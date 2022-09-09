#include "nn_nfp.hpp"
#include "debug/logger.h"

#include <wups.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <mutex>

#include <coreinit/time.h>

namespace nn::nfp {

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

DECL_FUNCTION(Result, Initialize)
{
    DEBUG_FUNCTION_LINE("nn::nfp::Initialize");

    // TODO initialize act

    return tagManager.Initialize();
}

DECL_FUNCTION(Result, Finalize)
{
    DEBUG_FUNCTION_LINE("nn::nfp::Finalize");

    // TODO finalize act

    return tagManager.Finalize();
}

DECL_FUNCTION(NfpState, GetNfpState)
{
    NfpState state;
    tagManager.GetNfpState(state);

    //DEBUG_FUNCTION_LINE_WRITE("nn::nfp::GetNfpState: %u", state);
    return state;
}

DECL_FUNCTION(Result, SetActivateEvent, OSEvent* event)
{
    DEBUG_FUNCTION_LINE("nn::nfp::SetActivateEvent");

    return tagManager.SetActivateEvent(event);
}

DECL_FUNCTION(Result, SetDeactivateEvent, OSEvent* event)
{
    DEBUG_FUNCTION_LINE("nn::nfp::SetDeactivateEvent");

    return tagManager.SetDeactivateEvent(event);
}

DECL_FUNCTION(Result, StartDetection)
{
    DEBUG_FUNCTION_LINE("nn::nfp::StartDetection");

    return tagManager.StartDetection();
}

DECL_FUNCTION(Result, StopDetection)
{
    DEBUG_FUNCTION_LINE("nn::nfp::StopDetection");

    return tagManager.StopDetection();
}

DECL_FUNCTION(Result, Mount)
{
    DEBUG_FUNCTION_LINE("nn::nfp::Mount");

    return tagManager.Mount();
}

DECL_FUNCTION(Result, MountReadOnly)
{
    DEBUG_FUNCTION_LINE("nn::nfp::MountReadOnly");

    return tagManager.MountReadOnly();
}

DECL_FUNCTION(Result, MountRom)
{
    DEBUG_FUNCTION_LINE("nn::nfp::MountRom");

    return tagManager.MountRom();
}

DECL_FUNCTION(Result, Unmount)
{
    DEBUG_FUNCTION_LINE("nn::nfp::Unmount");

    return tagManager.Unmount();
}

DECL_FUNCTION(Result, Flush)
{
    DEBUG_FUNCTION_LINE("nn::nfp::Flush");

    return tagManager.Flush();
}

DECL_FUNCTION(Result, Format, const void* data, uint32_t size)
{
    DEBUG_FUNCTION_LINE("nn::nfp::Format: %p %u", data, size);

    return tagManager.Format(data, size);
}

DECL_FUNCTION(Result, Restore)
{
    DEBUG_FUNCTION_LINE("nn::nfp::Restore");

    return tagManager.Restore();
}

DECL_FUNCTION(bool, IsExistApplicationArea)
{
    DEBUG_FUNCTION_LINE("nn::nfp::IsExistApplicationArea");

    return tagManager.IsExistApplicationArea();
}

DECL_FUNCTION(Result, CreateApplicationArea, ApplicationAreaCreateInfo const& createInfo)
{
    DEBUG_FUNCTION_LINE("nn::nfp::CreateApplicationArea");

    return tagManager.CreateApplicationArea(createInfo);
}

DECL_FUNCTION(Result, WriteApplicationArea, const void* data, uint32_t size, const TagId* tagId)
{
    DEBUG_FUNCTION_LINE("nn::nfp::WriteApplicationArea size %u", size);

    return tagManager.WriteApplicationArea(data, size, tagId);
}

DECL_FUNCTION(Result, OpenApplicationArea, uint32_t id)
{
    DEBUG_FUNCTION_LINE("nn::nfp::OpenApplicationArea");

    return tagManager.OpenApplicationArea(id);
}

DECL_FUNCTION(Result, ReadApplicationArea, void* outData, uint32_t size)
{
    DEBUG_FUNCTION_LINE("nn::nfp::ReadApplicationArea size %d", size);

    return tagManager.ReadApplicationArea(outData, size);
}

DECL_FUNCTION(Result, DeleteApplicationArea)
{
    DEBUG_FUNCTION_LINE("nn::nfp::DeleteApplicationArea");

    return tagManager.DeleteApplicationArea();
}

DECL_FUNCTION(Result, GetTagInfo, TagInfo* outTagInfo)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetTagInfo");

    return tagManager.GetTagInfo(outTagInfo);
}

DECL_FUNCTION(Result, GetNfpCommonInfo, CommonInfo* outCommonInfo)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetNfpCommonInfo");

    return tagManager.GetNfpCommonInfo(outCommonInfo);
}

DECL_FUNCTION(Result, GetNfpRegisterInfo, RegisterInfo* outRegisterInfo)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetNfpRegisterInfo");

    return tagManager.GetNfpRegisterInfo(outRegisterInfo);
}

DECL_FUNCTION(Result, DeleteNfpRegisterInfo)
{
    DEBUG_FUNCTION_LINE("nn::nfp::DeleteNfpRegisterInfo");

    return tagManager.DeleteNfpRegisterInfo();
}

DECL_FUNCTION(Result, GetNfpReadOnlyInfo, ReadOnlyInfo* outReadOnlyInfo)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetNfpReadOnlyInfo");
    
    return tagManager.GetNfpReadOnlyInfo(outReadOnlyInfo);
}

DECL_FUNCTION(Result, GetNfpRomInfo, RomInfo* outRomInfo)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetNfpRomInfo");

    return tagManager.GetNfpRomInfo(outRomInfo);
}

DECL_FUNCTION(Result, GetNfpAdminInfo, AdminInfo* outAdminInfo)
{
    DEBUG_FUNCTION_LINE("Warning: nn::nfp::GetNfpAdminInfo not implemented!");

    return tagManager.GetNfpAdminInfo(outAdminInfo);
}

DECL_FUNCTION(Result, SetNfpRegisterInfo, RegisterInfoSet const& info)
{
    DEBUG_FUNCTION_LINE("nn::nfp::SetNfpRegisterInfo");

    return tagManager.SetNfpRegisterInfo(info);
}

DECL_FUNCTION(Result, InitializeCreateInfo, ApplicationAreaCreateInfo* createInfo)
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

DECL_FUNCTION(Result, InitializeRegisterInfoSet, RegisterInfoSet* registerInfoSet)
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

DECL_FUNCTION(Result, GetBackupEntryFromMemory, void* info, uint16_t param_2, void* outData, int param_4)
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

DECL_FUNCTION(Result, GetBackupHeaderFromMemory, void* info, void* outData, int param_3)
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

DECL_FUNCTION(uint32_t, GetBackupSaveDataSize)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetBackupSaveDataSize");

    if (!tagManager.IsInitialized()) {
        return 0;
    }

    // Currently not implementing backups
    return 0;
}

DECL_FUNCTION(Result, ReadAllBackupSaveData, void* param_1, void* param_2)
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

DECL_FUNCTION(Result, AntennaCheck)
{
    DEBUG_FUNCTION_LINE("nn::nfp::AntennaCheck");

    if (!tagManager.IsInitialized()) {
        return NFP_INVALID_STATE;
    }

    // TODO re-initialize nn::nfp here

    return NFP_SUCCESS;
}

DECL_FUNCTION(Result, CheckMovable, void* moveInfo)
{
    DEBUG_FUNCTION_LINE("nn::nfp::CheckMovable");
    DEBUG_FUNCTION_LINE("NOT IMPLEMENTED!");

    return NFP_INVALID_STATE;
}

DECL_FUNCTION(Result, GetConnectionStatus, uint32_t* connectionStatus)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetConnectionStatus");
    DEBUG_FUNCTION_LINE("NOT IMPLEMENTED!");

    *connectionStatus = 0;

    return NFP_SUCCESS;
}

WUPS_MUST_REPLACE(Initialize,                       WUPS_LOADER_LIBRARY_NN_NFP, Initialize__Q2_2nn3nfpFv);
WUPS_MUST_REPLACE(Finalize,                         WUPS_LOADER_LIBRARY_NN_NFP, Finalize__Q2_2nn3nfpFv);
WUPS_MUST_REPLACE(GetNfpState,                      WUPS_LOADER_LIBRARY_NN_NFP, GetNfpState__Q2_2nn3nfpFv);
WUPS_MUST_REPLACE(SetActivateEvent,                 WUPS_LOADER_LIBRARY_NN_NFP, SetActivateEvent__Q2_2nn3nfpFP7OSEvent);
WUPS_MUST_REPLACE(SetDeactivateEvent,               WUPS_LOADER_LIBRARY_NN_NFP, SetDeactivateEvent__Q2_2nn3nfpFP7OSEvent);
WUPS_MUST_REPLACE(StartDetection,                   WUPS_LOADER_LIBRARY_NN_NFP, StartDetection__Q2_2nn3nfpFv);
WUPS_MUST_REPLACE(StopDetection,                    WUPS_LOADER_LIBRARY_NN_NFP, StopDetection__Q2_2nn3nfpFv);
WUPS_MUST_REPLACE(Mount,                            WUPS_LOADER_LIBRARY_NN_NFP, Mount__Q2_2nn3nfpFv);
WUPS_MUST_REPLACE(MountReadOnly,                    WUPS_LOADER_LIBRARY_NN_NFP, MountReadOnly__Q2_2nn3nfpFv);
WUPS_MUST_REPLACE(MountRom,                         WUPS_LOADER_LIBRARY_NN_NFP, MountRom__Q2_2nn3nfpFv);
WUPS_MUST_REPLACE(Unmount,                          WUPS_LOADER_LIBRARY_NN_NFP, Unmount__Q2_2nn3nfpFv);
WUPS_MUST_REPLACE(Flush,                            WUPS_LOADER_LIBRARY_NN_NFP, Flush__Q2_2nn3nfpFv);
WUPS_MUST_REPLACE(Format,                           WUPS_LOADER_LIBRARY_NN_NFP, Format__Q2_2nn3nfpFPCUci);
WUPS_MUST_REPLACE(Restore,                          WUPS_LOADER_LIBRARY_NN_NFP, Restore__Q2_2nn3nfpFv);
WUPS_MUST_REPLACE(IsExistApplicationArea,           WUPS_LOADER_LIBRARY_NN_NFP, IsExistApplicationArea__Q2_2nn3nfpFv);
WUPS_MUST_REPLACE(CreateApplicationArea,            WUPS_LOADER_LIBRARY_NN_NFP, CreateApplicationArea__Q2_2nn3nfpFRCQ3_2nn3nfp25ApplicationAreaCreateInfo);
WUPS_MUST_REPLACE(WriteApplicationArea,             WUPS_LOADER_LIBRARY_NN_NFP, WriteApplicationArea__Q2_2nn3nfpFPCvUiRCQ3_2nn3nfp5TagId);
WUPS_MUST_REPLACE(OpenApplicationArea,              WUPS_LOADER_LIBRARY_NN_NFP, OpenApplicationArea__Q2_2nn3nfpFUi);
WUPS_MUST_REPLACE(ReadApplicationArea,              WUPS_LOADER_LIBRARY_NN_NFP, ReadApplicationArea__Q2_2nn3nfpFPvUi);
WUPS_MUST_REPLACE(DeleteApplicationArea,            WUPS_LOADER_LIBRARY_NN_NFP, DeleteApplicationArea__Q2_2nn3nfpFv);
WUPS_MUST_REPLACE(GetTagInfo,                       WUPS_LOADER_LIBRARY_NN_NFP, GetTagInfo__Q2_2nn3nfpFPQ3_2nn3nfp7TagInfo);
WUPS_MUST_REPLACE(GetNfpCommonInfo,                 WUPS_LOADER_LIBRARY_NN_NFP, GetNfpCommonInfo__Q2_2nn3nfpFPQ3_2nn3nfp10CommonInfo);
WUPS_MUST_REPLACE(GetNfpRegisterInfo,               WUPS_LOADER_LIBRARY_NN_NFP, GetNfpRegisterInfo__Q2_2nn3nfpFPQ3_2nn3nfp12RegisterInfo);
WUPS_MUST_REPLACE(DeleteNfpRegisterInfo,            WUPS_LOADER_LIBRARY_NN_NFP, DeleteNfpRegisterInfo__Q2_2nn3nfpFv);
WUPS_MUST_REPLACE(GetNfpReadOnlyInfo,               WUPS_LOADER_LIBRARY_NN_NFP, GetNfpReadOnlyInfo__Q2_2nn3nfpFPQ3_2nn3nfp12ReadOnlyInfo);
WUPS_MUST_REPLACE(GetNfpRomInfo,                    WUPS_LOADER_LIBRARY_NN_NFP, GetNfpRomInfo__Q2_2nn3nfpFPQ3_2nn3nfp7RomInfo);
WUPS_MUST_REPLACE(GetNfpAdminInfo,                  WUPS_LOADER_LIBRARY_NN_NFP, GetNfpAdminInfo__Q2_2nn3nfpFPQ3_2nn3nfp9AdminInfo);
WUPS_MUST_REPLACE(SetNfpRegisterInfo,               WUPS_LOADER_LIBRARY_NN_NFP, SetNfpRegisterInfo__Q2_2nn3nfpFRCQ3_2nn3nfp15RegisterInfoSet);
WUPS_MUST_REPLACE(InitializeCreateInfo,             WUPS_LOADER_LIBRARY_NN_NFP, InitializeCreateInfo__Q2_2nn3nfpFPQ3_2nn3nfp25ApplicationAreaCreateInfo);
WUPS_MUST_REPLACE(InitializeRegisterInfoSet,        WUPS_LOADER_LIBRARY_NN_NFP, InitializeRegisterInfoSet__Q2_2nn3nfpFPQ3_2nn3nfp15RegisterInfoSet);
WUPS_MUST_REPLACE(GetBackupEntryFromMemory,         WUPS_LOADER_LIBRARY_NN_NFP, GetBackupEntryFromMemory__Q2_2nn3nfpFPQ3_2nn3nfp15BackupEntryInfoUsPCvUi);
WUPS_MUST_REPLACE(GetBackupHeaderFromMemory,        WUPS_LOADER_LIBRARY_NN_NFP, GetBackupHeaderFromMemory__Q2_2nn3nfpFPQ3_2nn3nfp16BackupHeaderInfoPCvUi);
WUPS_MUST_REPLACE(GetBackupSaveDataSize,            WUPS_LOADER_LIBRARY_NN_NFP, GetBackupSaveDataSize__Q2_2nn3nfpFv);
WUPS_MUST_REPLACE(ReadAllBackupSaveData,            WUPS_LOADER_LIBRARY_NN_NFP, ReadAllBackupSaveData__Q2_2nn3nfpFPvUi);
WUPS_MUST_REPLACE(AntennaCheck,                     WUPS_LOADER_LIBRARY_NN_NFP, AntennaCheck__Q2_2nn3nfpFv);
WUPS_MUST_REPLACE(CheckMovable,                     WUPS_LOADER_LIBRARY_NN_NFP, CheckMovable__Q2_2nn3nfpFRCQ3_2nn3nfp8MoveInfo);
WUPS_MUST_REPLACE(GetConnectionStatus,              WUPS_LOADER_LIBRARY_NN_NFP, GetConnectionStatus__Q2_2nn3nfpFPQ3_2nn3nfp16ConnectionStatus);
// FindCharacterImage__Q2_2nn3nfpFPUiUs
// GetCharacterImage__Q2_2nn3nfpFPvUiUs
// GetCharacterName__Q2_2nn3nfpFPUsUiPCUcUc
// GetNfpInfoForMove__Q2_2nn3nfpFPQ3_2nn3nfp8MoveInfo
// FormatForMove__Q2_2nn3nfpFRCQ3_2nn3nfp8MoveInfoT1
// Move__Q2_2nn3nfpFv

} // namespace nn::nfp
