#include "re_nfpii.hpp"
#include "debug/logger.h"

#include <wums.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <mutex>

#include <coreinit/time.h>
#include <coreinit/memory.h>
#include <coreinit/dynload.h>
#include <sysapp/args.h>
#include <sysapp/switch.h>

namespace re::nfpii {

TagManager tagManager;

bool CheckZero(const void* data, uint32_t size)
{
    assert(data);

    for (uint32_t i = 0; i < size; i++) {
        if (((const uint8_t*) data)[i] != 0) {
            return false;
        }
    }

    return true;
}

void GetRandom(void* data, uint32_t size)
{
    assert(data);

    for (uint32_t i = 0; i < size; i++) {
        ((uint8_t*) data)[i] = (uint8_t) rand();
    }
}

uint16_t IncreaseCount(uint16_t count, bool overflow)
{
    if (count == 0xffff) {
        if (overflow) {
            return 0;
        }

        return 0xffff;
    }

    return count + 1;
}

static uint8_t FindTagType(uint8_t p, uint8_t t)
{
    static uint8_t p0_lookup[] = { 1, 1, 2, 4 };
    static uint8_t p1_lookup[] = { 8, 1, 2, 4 };
    static uint8_t p2_lookup[] = { 16, 1, 2, 4 };

    switch (p) {
    case 0:
        if (t != 0 && t < 4) {
            return p0_lookup[t];
        }
        break;
    case 1:
        if (t != 0 && t < 4) {
            return p1_lookup[t];
        }
        break;
    case 2:
        if (t != 0 && t < 4) {
            return p2_lookup[t];
        }
        break;
    case 6:
    case 0x83:
        return 0x20;
    }

    return 0;
}

void ReadTagInfo(TagInfo* info, const NTAGData* data)
{
    assert(info);
    
    memcpy(info->id.id, data->uid, data->uidSize);
    info->id.lenght = data->uidSize;
    info->protocol = data->protocol;
    info->tag_type = FindTagType(data->protocol, data->tagType);   
}

void ReadCommonInfo(CommonInfo* info, const NTAGData* data)
{
    assert(info);

    if (!(data->info.flags_hi & 1) && !(data->info.flags_hi & 2)) {
        ConvertAmiiboDate(&info->last_write_date, 0);
        info->write_counter = data->info.writeCount;
        memcpy(&info->game_character_id, data->info.characterInfo, 3);
        info->model_number = data->info.modelNumber;
        info->figure_type = data->info.figureType;
        info->series = data->info.series;
        info->application_area_size = data->appData.size;
        info->unk = data->info.zero;
        memset(info->reserved, 0, sizeof(info->reserved));
    } else {
        ConvertAmiiboDate(&info->last_write_date, data->info.lastWriteDate);
        info->write_counter = data->info.writeCount;
        memcpy(&info->game_character_id, data->info.characterInfo, 3);
        info->model_number = data->info.modelNumber;
        info->figure_type = data->info.figureType;
        info->series = data->info.series;
        info->application_area_size = data->appData.size;
        info->unk = data->info.zero;
        memset(info->reserved, 0, sizeof(info->reserved));
    }
}

void ReadRegisterInfo(RegisterInfo* info, const NTAGData* data)
{
    assert(info);

    memcpy(&info->mii, &data->info.mii, sizeof(info->mii));
    memcpy(info->amiibo_name, data->info.name, sizeof(data->info.name));
    // null-terminate name
    info->amiibo_name[10] = 0;
    ConvertAmiiboDate(&info->first_write_date, data->info.setupDate);
    info->flags = data->info.flags_lo;
    info->country_code = data->info.countryCode;
    memset(info->reserved, 0, sizeof(info->reserved));
}

void ReadReadOnlyInfo(ReadOnlyInfo* info, const NTAGData* data)
{
    assert(info);

    memcpy(&info->game_character_id, data->info.characterInfo, 3);
    info->model_number = data->info.modelNumber;
    info->figure_type = data->info.figureType;
    info->series = data->info.series;
    memset(info->reserved, 0, sizeof(info->reserved));
}

static uint8_t GetPlatform(uint64_t titleId) {
    static uint8_t lookup[] = { 0, 1, 0 };

    // This checks the first part of the low title id, I assume 0 = 3ds, 1 = U?
    // So this returns 1 if this is a Wii U title id?
    if ((titleId >> 0x1c & 0xf) < 3) {
        return lookup[titleId >> 0x1c & 0xf];
    }

    return 0xff;
}

void ReadAdminInfo(AdminInfo* info, const NTAGData* data)
{
    assert(info);

    if (data->info.flags_hi & 2) {
        info->flags = data->info.flags_hi;
        info->titleId = 0;
        info->platform = 0xff;
        info->appAreaId = 0;
        info->updateCount = data->info.appDataUpdateCount;
        info->version = data->formatVersion;
        memset(info->reserved, 0, sizeof(info->reserved));
    } else {
        info->titleId = data->info.titleId;
        info->appAreaId = data->info.appId;
        info->platform = GetPlatform(data->info.titleId);
        info->flags = data->info.flags_hi;
        info->updateCount = data->info.appDataUpdateCount;
        info->version = data->formatVersion;
        memset(info->reserved, 0, sizeof(info->reserved));
    }
}

void ClearApplicationArea(NTAGData* data)
{
    GetRandom(&data->info.appId, sizeof(data->info.appId));
    GetRandom(&data->info.titleId, sizeof(data->info.titleId));
    // clear the "has application area" bit
    data->info.flags_hi &= ~2;
}

void ClearRegisterInfo(NTAGData* data)
{
    data->info.flags_lo = 0;
    data->info.countryCode = 0;
    data->info.setupDate = rand();
    GetRandom(data->info.name, sizeof(data->info.name));
    GetRandom(&data->info.mii, sizeof(data->info.mii));
    // clear the "has register info" bit
    data->info.flags_hi &= ~1;
}

Result SetCountryRegion(uint8_t* outCountryCode)
{
    // TODO
    *outCountryCode = 0xff;
    return NFP_SUCCESS;
}

uint16_t OSTimeToAmiiboTime(OSTime time)
{
    OSCalendarTime ct;
    OSTicksToCalendarTime(time, &ct);

    return ((ct.tm_year << 9) - 0xfa000) | ((ct.tm_mon << 5) + 0x20) | (ct.tm_mday);
}

static bool VerifyDate(uint32_t y, uint32_t m, uint32_t d, uint32_t h, uint32_t min, uint32_t s, uint32_t ms)
{
    // N does proper ymd verification, too lazy to implement this right now
    if (h < 24 && min < 60 && s < 60 && ms < 1000) {
        return true;
    }

    return false;
}

void ConvertAmiiboDate(Date* date, uint16_t time)
{
    uint32_t day = time & 0x1f;
    uint32_t month = time >> 0x5 & 0xf;
    uint32_t year = (time >> 0x9 & 0x7f) + 0x7d0;
    if (!VerifyDate(year, month, day, 0, 0, 0, 0)) {
        // TODO what does nfp actually do here?
        date->year = 0;
        date->month = 0;
        date->day = 0;
        return;
    }

    date->day = day;
    date->month = month;
    date->year = year;
}

bool CheckAmiiboMagic(NTAGData* data)
{
    return data->info.magic == 0xa5;
}

bool CheckUuidCRC(NTAGInfo* info)
{
    // TODO
    return true;
}

void SetUuidCRC(uint32_t* crc)
{
    // TODO
}

static uint16_t crc16(uint16_t poly, const void* data, uint32_t size)
{
    uint16_t crc = 0;
    for (uint32_t byteIndex = 0; byteIndex < size; byteIndex++)  {
        for (int bitIndex = 7; bitIndex >= 0; bitIndex--) {
            crc = (((crc << 1) | ((((const uint8_t*) data)[byteIndex] >> bitIndex) & 0x1)) 
                ^ (((crc & 0x8000) != 0) ? poly : 0)); 
        }
    }

    for (int counter = 16; counter > 0; counter--)  {
        crc = ((crc << 1) ^ (((crc & 0x8000) != 0) ? poly : 0));
    }

    return crc;
}

Result UpdateMii(FFLStoreData* data)
{
    // Make sure the checksum matches
    if (crc16(0x1021, data, 0x5e) != data->checksum) {
        return NFP_OUT_OF_RANGE;
    }

    // Not sure what this bit is used for
    data->data.core.unk_0x18_b1 = 0;

    // Update checksum
    data->checksum = crc16(0x1021, data, 0x5e);
    return NFP_SUCCESS;
}

Result Initialize()
{
    DEBUG_FUNCTION_LINE("nn::nfp::Initialize");

    // TODO initialize act

    return tagManager.Initialize();
}

Result Finalize()
{
    DEBUG_FUNCTION_LINE("nn::nfp::Finalize");

    // TODO finalize act

    return tagManager.Finalize();
}

NfpState GetNfpState()
{
    NfpState state;
    tagManager.GetNfpState(state);

    //DEBUG_FUNCTION_LINE_WRITE("nn::nfp::GetNfpState: %u", state);
    return state;
}

Result SetActivateEvent(OSEvent* event)
{
    DEBUG_FUNCTION_LINE("nn::nfp::SetActivateEvent");

    return tagManager.SetActivateEvent(event);
}

Result SetDeactivateEvent(OSEvent* event)
{
    DEBUG_FUNCTION_LINE("nn::nfp::SetDeactivateEvent");

    return tagManager.SetDeactivateEvent(event);
}

Result StartDetection()
{
    DEBUG_FUNCTION_LINE("nn::nfp::StartDetection");

    return tagManager.StartDetection();
}

Result StopDetection()
{
    DEBUG_FUNCTION_LINE("nn::nfp::StopDetection");

    return tagManager.StopDetection();
}

Result Mount()
{
    DEBUG_FUNCTION_LINE("nn::nfp::Mount");

    return tagManager.Mount();
}

Result MountReadOnly()
{
    DEBUG_FUNCTION_LINE("nn::nfp::MountReadOnly");

    return tagManager.MountReadOnly();
}

Result MountRom()
{
    DEBUG_FUNCTION_LINE("nn::nfp::MountRom");

    return tagManager.MountRom();
}

Result Unmount()
{
    DEBUG_FUNCTION_LINE("nn::nfp::Unmount");

    return tagManager.Unmount();
}

Result Flush()
{
    DEBUG_FUNCTION_LINE("nn::nfp::Flush");

    return tagManager.Flush();
}

Result Format(const void* data, uint32_t size)
{
    DEBUG_FUNCTION_LINE("nn::nfp::Format: %p %u", data, size);

    return tagManager.Format(data, size);
}

Result Restore()
{
    DEBUG_FUNCTION_LINE("nn::nfp::Restore");

    return tagManager.Restore();
}

bool IsExistApplicationArea()
{
    DEBUG_FUNCTION_LINE("nn::nfp::IsExistApplicationArea");

    return tagManager.IsExistApplicationArea();
}

Result CreateApplicationArea(ApplicationAreaCreateInfo const& createInfo)
{
    DEBUG_FUNCTION_LINE("nn::nfp::CreateApplicationArea");

    return tagManager.CreateApplicationArea(createInfo);
}

Result WriteApplicationArea(const void* data, uint32_t size, const TagId* tagId)
{
    DEBUG_FUNCTION_LINE("nn::nfp::WriteApplicationArea size %u", size);

    return tagManager.WriteApplicationArea(data, size, tagId);
}

Result OpenApplicationArea(uint32_t id)
{
    DEBUG_FUNCTION_LINE("nn::nfp::OpenApplicationArea");

    return tagManager.OpenApplicationArea(id);
}

Result ReadApplicationArea(void* outData, uint32_t size)
{
    DEBUG_FUNCTION_LINE("nn::nfp::ReadApplicationArea size %d", size);

    return tagManager.ReadApplicationArea(outData, size);
}

Result DeleteApplicationArea()
{
    DEBUG_FUNCTION_LINE("nn::nfp::DeleteApplicationArea");

    return tagManager.DeleteApplicationArea();
}

Result GetTagInfo(TagInfo* outTagInfo)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetTagInfo");

    return tagManager.GetTagInfo(outTagInfo);
}

Result GetNfpCommonInfo(CommonInfo* outCommonInfo)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetNfpCommonInfo");

    return tagManager.GetNfpCommonInfo(outCommonInfo);
}

Result GetNfpRegisterInfo(RegisterInfo* outRegisterInfo)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetNfpRegisterInfo");

    return tagManager.GetNfpRegisterInfo(outRegisterInfo);
}

Result DeleteNfpRegisterInfo()
{
    DEBUG_FUNCTION_LINE("nn::nfp::DeleteNfpRegisterInfo");

    return tagManager.DeleteNfpRegisterInfo();
}

Result GetNfpReadOnlyInfo(ReadOnlyInfo* outReadOnlyInfo)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetNfpReadOnlyInfo");
    
    return tagManager.GetNfpReadOnlyInfo(outReadOnlyInfo);
}

Result GetNfpRomInfo(RomInfo* outRomInfo)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetNfpRomInfo");

    return tagManager.GetNfpRomInfo(outRomInfo);
}

Result GetNfpAdminInfo(AdminInfo* outAdminInfo)
{
    DEBUG_FUNCTION_LINE("Warning: nn::nfp::GetNfpAdminInfo not implemented!");

    return tagManager.GetNfpAdminInfo(outAdminInfo);
}

Result SetNfpRegisterInfo(RegisterInfoSet const& info)
{
    DEBUG_FUNCTION_LINE("nn::nfp::SetNfpRegisterInfo");

    return tagManager.SetNfpRegisterInfo(info);
}

Result InitializeCreateInfo(ApplicationAreaCreateInfo* createInfo)
{
    DEBUG_FUNCTION_LINE("nn::nfp::InitializeCreateInfo");

    if (!createInfo) {
        return NFP_INVALID_PARAM;
    }

    if (!tagManager.IsInitialized()) {
        return NFP_INVALID_STATE;
    }

    createInfo->data = nullptr;
    createInfo->id = 0;
    createInfo->size = 0;
    memset(createInfo->reserved, 0, sizeof(createInfo->reserved));

    return NFP_SUCCESS;
}

Result InitializeRegisterInfoSet(RegisterInfoSet* registerInfoSet)
{
    DEBUG_FUNCTION_LINE("nn::nfp::InitializeRegisterInfoSet");

    if (!registerInfoSet) {
        return NFP_INVALID_PARAM;
    }

    if (!tagManager.IsInitialized()) {
        return NFP_INVALID_STATE;
    }

    memset(registerInfoSet, 0, sizeof(RegisterInfoSet));

    return NFP_SUCCESS;
}

Result GetBackupEntryFromMemory(void* info, uint16_t param_2, void* outData, int param_4)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetBackupEntryFromMemory");

    if (!tagManager.IsInitialized()) {
        return NFP_INVALID_STATE;
    }

    if (!info || !outData) {
        return NFP_INVALID_PARAM;
    }

    return NFP_NO_BACKUPENTRY;
}

Result GetBackupHeaderFromMemory(void* info, void* outData, int param_3)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetBackupHeaderFromMemory");

    if (!tagManager.IsInitialized()) {
        return NFP_INVALID_STATE;
    }

    if (info == NULL || outData == NULL) {
        return NFP_INVALID_PARAM;
    }

    return NFP_INVALID_PARAM; // is there a NO_BACKUPHEADER?
}

uint32_t GetBackupSaveDataSize()
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetBackupSaveDataSize");

    if (!tagManager.IsInitialized()) {
        return 0;
    }

    // Currently not implementing backups
    return 0;
}

Result ReadAllBackupSaveData(void* param_1, void* param_2)
{
    DEBUG_FUNCTION_LINE("nn::nfp::ReadAllBackupSaveData");

    if (!tagManager.IsInitialized()) {
        return NFP_INVALID_STATE;
    }

    if (!param_1) {
        return NFP_INVALID_PARAM;
    }

    if (((uint32_t)param_1 & 63) != 0) {
        return NFP_INVALID_ALIGNMENT;
    }

    if (((uint32_t)param_2 & 63) != 0) {
        return NFP_INVALID_ALIGNMENT;
    }

    return NFP_NO_BACKUP_SAVEDATA;
}

Result AntennaCheck()
{
    DEBUG_FUNCTION_LINE("nn::nfp::AntennaCheck");

    if (!tagManager.IsInitialized()) {
        return NFP_INVALID_STATE;
    }

    // TODO re-initialize nn::nfp here

    return NFP_SUCCESS;
}

Result CheckMovable(void* moveInfo)
{
    DEBUG_FUNCTION_LINE("nn::nfp::CheckMovable");

    // TODO

    return NFP_INVALID_STATE;
}

Result GetConnectionStatus(uint32_t* connectionStatus)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetConnectionStatus");

    // TODO

    *connectionStatus = 0;

    return NFP_SUCCESS;
}

Result FindCharacterImage(uint32_t*, uint32_t)
{
    DEBUG_FUNCTION_LINE("nn::nfp::FindCharacterImage");

    if (!tagManager.IsInitialized()) {
        return NFP_INVALID_STATE;
    }

    return NFP_INVALID_PARAM;
}

Result GetCharacterImage(void* buffer, uint32_t size, uint32_t)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetCharacterImage");

    if (!tagManager.IsInitialized()) {
        return NFP_INVALID_STATE;
    }

    if (!buffer) {
        return NFP_INVALID_PARAM;
    }

    if (((uint32_t)buffer & 63) != 0) {
        return NFP_INVALID_ALIGNMENT;
    }

    if (((uint32_t)size & 63) != 0) {
        return NFP_INVALID_ALIGNMENT;
    }

    return NFP_INVALID_PARAM;
}

Result GetCharacterName(void*, uint32_t, uint32_t, uint32_t)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetCharacterName");

    if (!tagManager.IsInitialized()) {
        return NFP_INVALID_STATE;
    }

    return NFP_INVALID_PARAM;
}

Result GetNfpInfoForMove(void*)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetNfpInfoForMove");

    // TODO

    if (!tagManager.IsInitialized()) {
        return NFP_INVALID_STATE;
    }

    return NFP_INVALID_PARAM;
}

Result FormatForMove(const void* data, uint32_t size)
{
    DEBUG_FUNCTION_LINE("nn::nfp::FormatForMove");

    // TODO

    if (!tagManager.IsInitialized()) {
        return NFP_INVALID_STATE;
    }

    return NFP_INVALID_PARAM;
}

Result Move()
{
    DEBUG_FUNCTION_LINE("nn::nfp::Move");

    // TODO

    if (!tagManager.IsInitialized()) {
        return NFP_INVALID_STATE;
    }

    return NFP_INVALID_PARAM;
}

// The imported functions for dynamic modules won't be correct while in an applet,
// so we dynload them with dynload as a workaround.
int32_t (*dyn_SYSDeserializeSysArgs)(SYSDeserializeCallback callback, void *userArg);
int32_t (*dyn__SYSDirectlyReturnToCaller)(SYSStandardArgsOut *arg);
BOOL (*dyn__SYSDeserializeStandardArg)(SYSDeserializeArg *deserializeArg, SYSStandardArg *standardArg);

static void loadSysappFunctionPointers()
{
    OSDynLoad_Module sysappModule;
    OSDynLoad_Acquire("sysapp.rpl", &sysappModule);

    OSDynLoad_FindExport(sysappModule, FALSE, "SYSDeserializeSysArgs", (void**) &dyn_SYSDeserializeSysArgs);
    OSDynLoad_FindExport(sysappModule, FALSE, "_SYSDirectlyReturnToCaller", (void**) &dyn__SYSDirectlyReturnToCaller);
    OSDynLoad_FindExport(sysappModule, FALSE, "_SYSDeserializeStandardArg", (void**) &dyn__SYSDeserializeStandardArg);
}

// We need to re-init the devoptab when switching between applet and game
extern "C" {
    void __init_wut_devoptab();
    void __fini_wut_devoptab();
}

void OnCabinetEnter()
{
    // Need to dynload sysapp pointers while in an applet
    loadSysappFunctionPointers();

    __init_wut_devoptab();

    tagManager.SetAmiiboSettings(true);
}

void OnCabinetLeave()
{
    tagManager.SetAmiiboSettings(false);

    __fini_wut_devoptab();
}

void OnGameEnter()
{
    __init_wut_devoptab();
}

void OnGameLeave()
{
    __fini_wut_devoptab();
}

static void AmiiboSettingsDeserializationCallback(SYSDeserializeArg* arg, void* usrarg)
{
    AmiiboSettingsArgs* amiiboArgs = (AmiiboSettingsArgs*) usrarg;
    if (!amiiboArgs) {
        return;
    }

    if (dyn__SYSDeserializeStandardArg(arg, &amiiboArgs->standardArg)) {
        return;
    }

    if (strcmp(arg->argName, "amb:mode") == 0) {
        OSBlockMove(&amiiboArgs->argsIn.mode, arg->data, sizeof(amiiboArgs->argsIn.mode), TRUE);
        return;
    }

    if (strcmp(arg->argName, "amb:tag_info") == 0) {
        OSBlockMove(&amiiboArgs->argsIn.tag_info, arg->data, sizeof(amiiboArgs->argsIn.tag_info), TRUE);
        return;
    }

    if (strcmp(arg->argName, "amb:is_registered") == 0) {
        OSBlockMove(&amiiboArgs->argsIn.is_registered, arg->data, sizeof(amiiboArgs->argsIn.is_registered), TRUE);
        return;
    }

    if (strcmp(arg->argName, "amb:register_info") == 0) {
        OSBlockMove(&amiiboArgs->argsIn.register_info, arg->data, sizeof(amiiboArgs->argsIn.register_info), TRUE);
        return;
    }

    if (strcmp(arg->argName, "amb:common_info") == 0) {
        OSBlockMove(&amiiboArgs->argsIn.common_info, arg->data, sizeof(amiiboArgs->argsIn.common_info), TRUE);
        return;
    }
}

Result GetAmiiboSettingsArgs(AmiiboSettingsArgs* args)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetAmiiboSettingsArgs");

    OnCabinetEnter();

    if (!args) {
        return NFP_INVALID_PARAM;
    }

    memset(args, 0, sizeof(*args));
    if (dyn_SYSDeserializeSysArgs(AmiiboSettingsDeserializationCallback, args) != 0) {
        return RESULT(0xa1b3e880);
    }

    return NFP_SUCCESS;
}

static bool VerifyResultValueBlock(SYSArgDataBlock const& block)
{
    if (block.unk1 != 3) {
        return false;
    } 
    
    if (block.size != sizeof(AmiiboSettingsResult) + sizeof("ambResultValue:")) {
        return false;
    }

    if (memcmp(block.data, "ambResultValue:", sizeof("ambResultValue:")) != 0) {
        return false;
    }

    return true;
}

Result GetAmiiboSettingsResult(AmiiboSettingsResult* result, SYSArgDataBlock const& block)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetAmiiboSettingsResult");

    if (!result) {
        return NFP_INVALID_PARAM;
    }

    OnGameEnter();

    Initialize();

    if (!VerifyResultValueBlock(block)) {
        return RESULT(0xa1b12c00);
    }

    OSBlockMove(result, (uint8_t*) block.data + sizeof("ambResultValue:"), sizeof(*result), TRUE);
    return NFP_SUCCESS;
}

Result InitializeAmiiboSettingsArgsIn(AmiiboSettingsArgsIn* args)
{
    DEBUG_FUNCTION_LINE("nn::nfp::InitializeAmiiboSettingsArgsIn");

    if (!args) {
        return NFP_INVALID_PARAM;
    }

    args->mode = 0;
    memset(&args->tag_info, 0, sizeof(args->tag_info));
    args->is_registered = false;
    memset(&args->padding, 0, sizeof(args->padding));
    memset(&args->register_info, 0, sizeof(args->register_info));
    memset(&args->common_info, 0, sizeof(args->common_info));
    memset(args->reserved, 0, sizeof(args->reserved));

    return NFP_SUCCESS;
}

Result ReturnToCallerWithAmiiboSettingsResult(AmiiboSettingsResult const& result)
{
    DEBUG_FUNCTION_LINE("nn::nfp::ReturnToCallerWithAmiiboSettingsResult");

    struct {
        char argName[0x10];
        AmiiboSettingsResult result;
    } returnArg;
    OSBlockMove(returnArg.argName, "ambResultValue:", sizeof("ambResultValue:"), TRUE);
    OSBlockMove(&returnArg.result, &result, sizeof(result), TRUE);
    memset(returnArg.result.reserved, 0, sizeof(returnArg.result.reserved));

    SYSStandardArgsOut args;
    args.data = &returnArg;
    args.size = sizeof(returnArg);

    Finalize();

    OnCabinetLeave();

    if (dyn__SYSDirectlyReturnToCaller(&args) != 0) {
        return RESULT(0xa1b3e880);
    }

    return NFP_SUCCESS;
}

bool VerifyAmiiboSettingsArgs(AmiiboSettingsArgsIn const& args)
{
    if (args.mode != 0x0 && args.mode != 0x1 && args.mode != 0x2 && args.mode != 0x64) {
        return false;
    }

    if (!CheckZero(&args.tag_info.reserved1, sizeof(args.tag_info.reserved1))) {
        return false;
    }
    if (!CheckZero(&args.padding, sizeof(args.padding))) {
        return false;
    }
    if (!CheckZero(&args.common_info.reserved, sizeof(args.common_info.reserved))) {
        return false;
    }
    if (!CheckZero(&args.register_info.reserved, sizeof(args.register_info.reserved))) {
        return false;
    }
    if (!CheckZero(&args.reserved, sizeof(args.reserved))) {
        return false;
    }

    return true;
}

Result SwitchToAmiiboSettings(AmiiboSettingsArgsIn const& args, const char* standardArg, uint32_t standardArgSize)
{
    DEBUG_FUNCTION_LINE("nn::nfp::SwitchToAmiiboSettings");

    if (!VerifyAmiiboSettingsArgs(args)) {
        return NFP_INVALID_PARAM;
    }

    SYSClearSysArgs();
    SYSStandardArgsIn standardArgs;
    standardArgs.argString = standardArg;
    standardArgs.size = standardArgSize;
    if (_SYSSerializeStandardArgsIn(&standardArgs) < 0) {
        return NFP_INVALID_PARAM;
    }

    if (SYSSerializeSysArgs("amb:mode", &args.mode, sizeof(args.mode)) < 0) {
        return NFP_INVALID_PARAM;
    }

    if (SYSSerializeSysArgs("amb:tag_info", &args.tag_info, sizeof(args.tag_info)) < 0) {
        return NFP_INVALID_PARAM;
    }

    if (SYSSerializeSysArgs("amb:is_registered", &args.is_registered, sizeof(args.is_registered)) < 0) {
        return NFP_INVALID_PARAM;
    }

    if (SYSSerializeSysArgs("amb:register_info", &args.register_info, sizeof(args.register_info)) < 0) {
        return NFP_INVALID_PARAM;
    }

    if (SYSSerializeSysArgs("amb:common_info", &args.common_info, sizeof(args.common_info)) < 0) {
        return NFP_INVALID_PARAM;
    }

    Finalize();

    OnGameLeave();

    // Switch to amiibo settings
    if (_SYSDirectlySwitchTo(SYSAPP_PFID_CABINETU) != 0) {
        return RESULT(0xa1b3e880);
    }

    return NFP_SUCCESS;
}

Result SwitchToAmiiboSettings(AmiiboSettingsArgsIn const& args)
{
    return SwitchToAmiiboSettings(args, nullptr, 0);
}

uint32_t GetErrorCode(nn::Result const& res)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetErrorCode");

    if (res.GetModule() == Result::RESULT_MODULE_NN_NFP && res.IsFailure()) {
        return (res.GetDescription() >> 0x7) + 0x19a280;
    }

    return 0x19a280;
}

} // namespace re::nfpii

WUMS_EXPORT(WUMS_FUNCTION_EXPORT, Initialize__Q2_2nn3nfpFv,                                                                 re::nfpii::Initialize);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, Finalize__Q2_2nn3nfpFv,                                                                   re::nfpii::Finalize);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, GetNfpState__Q2_2nn3nfpFv,                                                                re::nfpii::GetNfpState);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, SetActivateEvent__Q2_2nn3nfpFP7OSEvent,                                                   re::nfpii::SetActivateEvent);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, SetDeactivateEvent__Q2_2nn3nfpFP7OSEvent,                                                 re::nfpii::SetDeactivateEvent);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, StartDetection__Q2_2nn3nfpFv,                                                             re::nfpii::StartDetection);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, StopDetection__Q2_2nn3nfpFv,                                                              re::nfpii::StopDetection);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, Mount__Q2_2nn3nfpFv,                                                                      re::nfpii::Mount);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, MountReadOnly__Q2_2nn3nfpFv,                                                              re::nfpii::MountReadOnly);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, MountRom__Q2_2nn3nfpFv,                                                                   re::nfpii::MountRom);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, Unmount__Q2_2nn3nfpFv,                                                                    re::nfpii::Unmount);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, Flush__Q2_2nn3nfpFv,                                                                      re::nfpii::Flush);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, Format__Q2_2nn3nfpFPCUci,                                                                 re::nfpii::Format);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, Restore__Q2_2nn3nfpFv,                                                                    re::nfpii::Restore);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, IsExistApplicationArea__Q2_2nn3nfpFv,                                                     re::nfpii::IsExistApplicationArea);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, CreateApplicationArea__Q2_2nn3nfpFRCQ3_2nn3nfp25ApplicationAreaCreateInfo,                re::nfpii::CreateApplicationArea);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, WriteApplicationArea__Q2_2nn3nfpFPCvUiRCQ3_2nn3nfp5TagId,                                 re::nfpii::WriteApplicationArea);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, OpenApplicationArea__Q2_2nn3nfpFUi,                                                       re::nfpii::OpenApplicationArea);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, ReadApplicationArea__Q2_2nn3nfpFPvUi,                                                     re::nfpii::ReadApplicationArea);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, DeleteApplicationArea__Q2_2nn3nfpFv,                                                      re::nfpii::DeleteApplicationArea);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, GetTagInfo__Q2_2nn3nfpFPQ3_2nn3nfp7TagInfo,                                               re::nfpii::GetTagInfo);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, GetNfpCommonInfo__Q2_2nn3nfpFPQ3_2nn3nfp10CommonInfo,                                     re::nfpii::GetNfpCommonInfo);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, GetNfpRegisterInfo__Q2_2nn3nfpFPQ3_2nn3nfp12RegisterInfo,                                 re::nfpii::GetNfpRegisterInfo);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, DeleteNfpRegisterInfo__Q2_2nn3nfpFv,                                                      re::nfpii::DeleteNfpRegisterInfo);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, GetNfpReadOnlyInfo__Q2_2nn3nfpFPQ3_2nn3nfp12ReadOnlyInfo,                                 re::nfpii::GetNfpReadOnlyInfo);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, GetNfpRomInfo__Q2_2nn3nfpFPQ3_2nn3nfp7RomInfo,                                            re::nfpii::GetNfpRomInfo);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, GetNfpAdminInfo__Q2_2nn3nfpFPQ3_2nn3nfp9AdminInfo,                                        re::nfpii::GetNfpAdminInfo);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, SetNfpRegisterInfo__Q2_2nn3nfpFRCQ3_2nn3nfp15RegisterInfoSet,                             re::nfpii::SetNfpRegisterInfo);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, InitializeCreateInfo__Q2_2nn3nfpFPQ3_2nn3nfp25ApplicationAreaCreateInfo,                  re::nfpii::InitializeCreateInfo);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, InitializeRegisterInfoSet__Q2_2nn3nfpFPQ3_2nn3nfp15RegisterInfoSet,                       re::nfpii::InitializeRegisterInfoSet);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, GetBackupEntryFromMemory__Q2_2nn3nfpFPQ3_2nn3nfp15BackupEntryInfoUsPCvUi,                 re::nfpii::GetBackupEntryFromMemory);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, GetBackupHeaderFromMemory__Q2_2nn3nfpFPQ3_2nn3nfp16BackupHeaderInfoPCvUi,                 re::nfpii::GetBackupHeaderFromMemory);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, GetBackupSaveDataSize__Q2_2nn3nfpFv,                                                      re::nfpii::GetBackupSaveDataSize);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, ReadAllBackupSaveData__Q2_2nn3nfpFPvUi,                                                   re::nfpii::ReadAllBackupSaveData);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, AntennaCheck__Q2_2nn3nfpFv,                                                               re::nfpii::AntennaCheck);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, CheckMovable__Q2_2nn3nfpFRCQ3_2nn3nfp8MoveInfo,                                           re::nfpii::CheckMovable);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, GetConnectionStatus__Q2_2nn3nfpFPQ3_2nn3nfp16ConnectionStatus,                            re::nfpii::GetConnectionStatus);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, FindCharacterImage__Q2_2nn3nfpFPUiUs,                                                     re::nfpii::FindCharacterImage);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, GetCharacterImage__Q2_2nn3nfpFPvUiUs,                                                     re::nfpii::GetCharacterImage);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, GetCharacterName__Q2_2nn3nfpFPUsUiPCUcUc,                                                 re::nfpii::GetCharacterName);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, GetNfpInfoForMove__Q2_2nn3nfpFPQ3_2nn3nfp8MoveInfo,                                       re::nfpii::GetNfpInfoForMove);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, FormatForMove__Q2_2nn3nfpFRCQ3_2nn3nfp8MoveInfoT1,                                        re::nfpii::FormatForMove);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, Move__Q2_2nn3nfpFv,                                                                       re::nfpii::Move);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, GetAmiiboSettingsArgs__Q2_2nn3nfpFPQ3_2nn3nfp18AmiiboSettingsArgs,                        re::nfpii::GetAmiiboSettingsArgs);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, GetAmiiboSettingsResult__Q2_2nn3nfpFPQ3_2nn3nfp20AmiiboSettingsResultRC15SysArgDataBlock, re::nfpii::GetAmiiboSettingsResult);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, InitializeAmiiboSettingsArgsIn__Q2_2nn3nfpFPQ3_2nn3nfp20AmiiboSettingsArgsIn,             re::nfpii::InitializeAmiiboSettingsArgsIn);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, ReturnToCallerWithAmiiboSettingsResult__Q2_2nn3nfpFRCQ3_2nn3nfp20AmiiboSettingsResult,    re::nfpii::ReturnToCallerWithAmiiboSettingsResult);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, SwitchToAmiiboSettings__Q2_2nn3nfpFRCQ3_2nn3nfp20AmiiboSettingsArgsInPCcUi,               (nn::Result (*)(nn::nfp::AmiiboSettingsArgsIn const&, const char*, uint32_t)) re::nfpii::SwitchToAmiiboSettings);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, SwitchToAmiiboSettings__Q2_2nn3nfpFRCQ3_2nn3nfp20AmiiboSettingsArgsIn,                    (nn::Result (*)(nn::nfp::AmiiboSettingsArgsIn const&)) re::nfpii::SwitchToAmiiboSettings);
WUMS_EXPORT(WUMS_FUNCTION_EXPORT, GetErrorCode__Q2_2nn3nfpFRCQ2_2nn6Result,                                                 re::nfpii::GetErrorCode);
