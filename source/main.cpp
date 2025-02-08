#include <wums.h>

#include <whb/libmanager.h>
#include <whb/log_cafe.h>
#include <whb/log_module.h>
#include <whb/log_udp.h>

#include <nfpii.h>
#include <re_nfpii/re_nfpii.hpp>
#include <notifications/notifications.h>

#include "utils/LogHandler.hpp"

#include <string_view>

#define STR_VALUE(arg) #arg
#define VERSION_STRING(x, y, z) "v" STR_VALUE(x) "." STR_VALUE(y) "." STR_VALUE(z)

WUMS_MODULE_EXPORT_NAME("nn_nfp");
WUMS_MODULE_DESCRIPTION("A nn_nfp reimplementation with support for Amiibo emulation");
WUMS_MODULE_AUTHOR("GaryOderNichts");
WUMS_MODULE_VERSION(VERSION_STRING(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH));
WUMS_MODULE_LICENSE("GPLv2");

namespace  {
    static bool sIsPluginLoaded = false;
    static bool sIsPluginLoadedInfoShown = false;

    void ShowNotification(std::string_view notification) {
        auto err1 = NotificationModule_SetDefaultValue(NOTIFICATION_MODULE_NOTIFICATION_TYPE_INFO,
                                                       NOTIFICATION_MODULE_DEFAULT_OPTION_KEEP_UNTIL_SHOWN, true);
        auto err2 = NotificationModule_SetDefaultValue(NOTIFICATION_MODULE_NOTIFICATION_TYPE_INFO,
                                                       NOTIFICATION_MODULE_DEFAULT_OPTION_DURATION_BEFORE_FADE_OUT,
                                                       15.0f);

        if (err1 != NOTIFICATION_MODULE_RESULT_SUCCESS || err2 != NOTIFICATION_MODULE_RESULT_SUCCESS) return;

        NotificationModule_AddInfoNotification(notification.data());
    }

}

WUMS_INITIALIZE(myargs)
{
    if (!WHBLogModuleInit()) {
        WHBLogCafeInit();
        WHBLogUdpInit();
    }

    if (NotificationModule_InitLibrary() != NOTIFICATION_MODULE_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("NotificationModule_InitLibrary failed");
    }

    LogHandler::Init();
}

WUMS_APPLICATION_STARTS()
{
    if (!WHBLogModuleInit()) {
        WHBLogCafeInit();
        WHBLogUdpInit();
    }
    sIsPluginLoaded = false;
}

WUMS_APPLICATION_ENDS()
{
    // Call finalize in case the application doesn't
    re::nfpii::tagManager.Finalize();
}

WUMS_ALL_APPLICATION_STARTS_DONE() {
    if(!sIsPluginLoadedInfoShown && !sIsPluginLoaded) {
        ShowNotification("re_nfpii module is loaded but the companion plugin is not loaded");
        sIsPluginLoadedInfoShown = true;
    }
}

uint32_t NfpiiGetVersion(void)
{
    return NFPII_VERSION(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
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

NFCError NfpiiQueueNFCGetTagInfo(NFCGetTagInfoCallbackFn callback, void* arg)
{
    LogHandler::Info("Module: Queued NFCGetTagInfo");

    return re::nfpii::tagManager.QueueNFCGetTagInfo(callback, arg);
}

void NfpiiSetPluginloaded()
{
    sIsPluginLoaded = true;
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
WUMS_EXPORT_FUNCTION(NfpiiSetPluginloaded);
