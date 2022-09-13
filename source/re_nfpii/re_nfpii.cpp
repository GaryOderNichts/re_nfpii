#include "re_nfpii.hpp"
#include "Cabinet.hpp"
#include "Utils.hpp"
#include "debug/logger.h"

#include <wums.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <mutex>

namespace re::nfpii {

TagManager tagManager;

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

Result Format(const void* data, int32_t size)
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
    DEBUG_FUNCTION_LINE("nn::nfp::GetNfpAdminInfo");

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

Result GetAmiiboSettingsArgs(AmiiboSettingsArgs* args)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetAmiiboSettingsArgs");

    return Cabinet::GetArgs(args);
}

Result GetAmiiboSettingsResult(AmiiboSettingsResult* result, SYSArgDataBlock const& block)
{
    DEBUG_FUNCTION_LINE("nn::nfp::GetAmiiboSettingsResult");

    return Cabinet::GetResult(result, block);
}

Result InitializeAmiiboSettingsArgsIn(AmiiboSettingsArgsIn* args)
{
    DEBUG_FUNCTION_LINE("nn::nfp::InitializeAmiiboSettingsArgsIn");

    return Cabinet::InitializeArgsIn(args);
}

Result ReturnToCallerWithAmiiboSettingsResult(AmiiboSettingsResult const& result)
{
    DEBUG_FUNCTION_LINE("nn::nfp::ReturnToCallerWithAmiiboSettingsResult");

    return Cabinet::ReturnToCallerWithResult(result);
}

Result SwitchToAmiiboSettings(AmiiboSettingsArgsIn const& args, const char* standardArg, uint32_t standardArgSize)
{
    DEBUG_FUNCTION_LINE("nn::nfp::SwitchToAmiiboSettings");

    return Cabinet::SwitchToCabinet(args, standardArg, standardArgSize);
}

Result SwitchToAmiiboSettings(AmiiboSettingsArgsIn const& args)
{
    DEBUG_FUNCTION_LINE("nn::nfp::SwitchToAmiiboSettings");

    return Cabinet::SwitchToCabinet(args, nullptr, 0);
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
