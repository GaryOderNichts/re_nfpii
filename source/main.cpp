#include <wums.h>

#include <whb/libmanager.h>
#include <whb/log_cafe.h>
#include <whb/log_module.h>
#include <whb/log_udp.h>

#include <nfpii.h>
#include <re_nfpii/re_nfpii.hpp>

#include "utils/LogHandler.hpp"

WUMS_MODULE_EXPORT_NAME("nn_nfp");
WUMS_MODULE_DESCRIPTION("A nn_nfp reimplementation with support for Amiibo emulation");
WUMS_MODULE_AUTHOR("GaryOderNichts");
WUMS_MODULE_VERSION("v0.2.2");
WUMS_MODULE_LICENSE("GPLv2");

WUMS_INITIALIZE(myargs)
{
    if (!WHBLogModuleInit()) {
        WHBLogCafeInit();
        WHBLogUdpInit();
    }

    LogHandler::Init();
}

WUMS_APPLICATION_STARTS()
{
    if (!WHBLogModuleInit()) {
        WHBLogCafeInit();
        WHBLogUdpInit();
    }
}

WUMS_APPLICATION_ENDS()
{
    // Call finalize in case the application doesn't
    re::nfpii::tagManager.Finalize();
}

// custom exports for the configuration plugin
bool NfpiiIsInitialized(void)
{
    return re::nfpii::tagManager.IsInitialized();
}

void NfpiiSetEmulationState(NfpiiEmulationState state)
{
    LogHandler::Info("Module: Updated emulation state to: %d", state);

    re::nfpii::tagManager.SetEmulationState(state);
}

NfpiiEmulationState NfpiiGetEmulationState(void)
{
    return re::nfpii::tagManager.GetEmulationState();
}

void NfpiiSetUUIDRandomizationState(NfpiiUUIDRandomizationState state)
{
    LogHandler::Info("Module: Updated uuid random state to: %d", state);

    re::nfpii::tagManager.SetUUIDRandomizationState(state);
}

NfpiiUUIDRandomizationState NfpiiGetUUIDRandomizationState(void)
{
    return re::nfpii::tagManager.GetUUIDRandomizationState();
}

void NfpiiSetRemoveAfterSeconds(float seconds)
{
    LogHandler::Info("Module: Updated remote after seconds to: %.1fs", seconds);

    re::nfpii::tagManager.SetRemoveAfterSeconds(seconds);
}

void NfpiiSetTagEmulationPath(const char* path)
{
    LogHandler::Info("Module: Update tag emulation path to: '%s'", path);

    re::nfpii::tagManager.SetTagEmulationPath(path);
}

const char* NfpiiGetTagEmulationPath(void)
{
    return re::nfpii::tagManager.GetTagEmulationPath().c_str();
}

NFCError NfpiiQueueNFCGetTagInfo(NFCTagInfoCallback callback, void* arg)
{
    LogHandler::Info("Module: Queued NFCGetTagInfo");

    return re::nfpii::tagManager.QueueNFCGetTagInfo(callback, arg);
}

WUMS_EXPORT_FUNCTION(NfpiiIsInitialized);
WUMS_EXPORT_FUNCTION(NfpiiSetEmulationState);
WUMS_EXPORT_FUNCTION(NfpiiGetEmulationState);
WUMS_EXPORT_FUNCTION(NfpiiSetUUIDRandomizationState);
WUMS_EXPORT_FUNCTION(NfpiiGetUUIDRandomizationState);
WUMS_EXPORT_FUNCTION(NfpiiSetRemoveAfterSeconds);
WUMS_EXPORT_FUNCTION(NfpiiSetTagEmulationPath);
WUMS_EXPORT_FUNCTION(NfpiiGetTagEmulationPath);
WUMS_EXPORT_FUNCTION(NfpiiQueueNFCGetTagInfo);
