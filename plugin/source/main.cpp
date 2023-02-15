#include <wups.h>
#include <wups/config/WUPSConfigItemMultipleValues.h>
#include <string>
#include <map>

#include <whb/libmanager.h>
#include <whb/log_cafe.h>
#include <whb/log_module.h>
#include <whb/log_udp.h>

#include <nfpii.h>
#include "debug/logger.h"
#include "config/ConfigItemSelectAmiibo.hpp"
#include "config/ConfigItemLog.hpp"

WUPS_PLUGIN_NAME("re_nfpii");
WUPS_PLUGIN_DESCRIPTION("A nn_nfp reimplementation with support for Amiibo emulation");
WUPS_PLUGIN_VERSION("v0.1.0");
WUPS_PLUGIN_AUTHOR("GaryOderNichts");
WUPS_PLUGIN_LICENSE("GPLv2");

//WUPS_USE_STORAGE("re_nfpii");
WUPS_USE_WUT_DEVOPTAB();

// This is in .5 steps, i.e. 9 would be 4.5s
#define MAX_REMOVE_AFTER_SECONDS 20

#define TAG_EMULATION_PATH std::string("/vol/external01/wiiu/re_nfpii/")

uint32_t currentRemoveAfterOption = 0;

static void nfpiiLogHandler(NfpiiLogVerbosity verb, const char* message)
{
    ConfigItemLog_PrintType((LogType) verb, message);
}

INITIALIZE_PLUGIN()
{
    if (!WHBLogModuleInit()) {
        WHBLogCafeInit();
        WHBLogUdpInit();
    }

    ConfigItemLog_Init();
    NfpiiSetLogHandler(nfpiiLogHandler);
}

DEINITIALIZE_PLUGIN()
{
    NfpiiSetLogHandler(nullptr);
}

ON_APPLICATION_START()
{
    if (!WHBLogModuleInit()) {
        WHBLogCafeInit();
        WHBLogUdpInit();
    }
}

static void stateChangedCallback(ConfigItemMultipleValues* values, uint32_t index)
{
    NfpiiSetEmulationState((NfpiiEmulationState) index);
}

static void removeAfterChangedCallback(ConfigItemMultipleValues* values, uint32_t index)
{
    currentRemoveAfterOption = index;
    NfpiiSetRemoveAfterSeconds(index / 2.0f);
}

static void uuidRandomizationChangedCallback(ConfigItemMultipleValues* values, uint32_t index)
{
    NfpiiSetUUIDRandomizationState((NfpiiUUIDRandomizationState) index);
}

static void amiiboSelectedCallback(ConfigItemSelectAmiibo* amiibos, const char* filePath)
{
    NfpiiSetTagEmulationPath(filePath);
}

WUPS_GET_CONFIG()
{
    // if (WUPS_OpenStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
    //     DEBUG_FUNCTION_LINE("Failed to open storage");
    //     return 0;
    // }

    WUPSConfigHandle config;
    WUPSConfig_CreateHandled(&config, "re_nfpii");

    WUPSConfigCategoryHandle cat;
    WUPSConfig_AddCategoryByNameHandled(config, "Settings", &cat);

    ConfigItemMultipleValuesPair emulationStateValues[2];
    emulationStateValues[0].value = EMULATION_OFF;
    emulationStateValues[0].valueName = (char*) "Emulation Disabled";
    emulationStateValues[1].value = EMULATION_ON;
    emulationStateValues[1].valueName = (char*) "Emulation Enabled";
    WUPSConfigItemMultipleValues_AddToCategoryHandled(config, cat, "state", "Set State", NfpiiGetEmulationState(), emulationStateValues, 2, stateChangedCallback);

    ConfigItemMultipleValuesPair removeAfterValues[MAX_REMOVE_AFTER_SECONDS + 1];
    removeAfterValues[0].value = 0;
    removeAfterValues[0].valueName = (char*) "Never";
    for (int i = 1; i < MAX_REMOVE_AFTER_SECONDS + 1; i++) {
        removeAfterValues[i].value = i;
        char* fmt = (char*) malloc(32);
        snprintf(fmt, 32, "%.1fs", i / 2.0f);
        removeAfterValues[i].valueName = fmt;
    }
    WUPSConfigItemMultipleValues_AddToCategoryHandled(config, cat, "remove_after", "Remove after", currentRemoveAfterOption, removeAfterValues, MAX_REMOVE_AFTER_SECONDS + 1, removeAfterChangedCallback);
    for (int i = 1; i < 10; i++) {
        free(removeAfterValues[i].valueName);
    }

#if 0 //TODO
    values[0].value = RANDOMIZATION_OFF;
    values[0].valueName = (char*) "Off";
    values[1].value = RANDOMIZATION_ONCE;
    values[1].valueName = (char*) "Once";
    values[2].value = RANDOMIZATION_EVERY_READ;
    values[2].valueName = (char*) "After reading";
    WUPSConfigItemMultipleValues_AddToCategoryHandled(config, cat, "random_uuid", "Randomize UUID", NfpiiGetUUIDRandomizationState(), values, 3, uuidRandomizationChangedCallback);
#endif

    std::string currentAmiiboPath = NfpiiGetTagEmulationPath();
    ConfigItemSelectAmiibo_AddToCategoryHandled(config, cat, "select_amiibo", "Select Amiibo", TAG_EMULATION_PATH.c_str(), currentAmiiboPath.c_str(), amiiboSelectedCallback);

    ConfigItemLog_AddToCategoryHandled(config, cat, "log", "Logs");

    return config;
}

WUPS_CONFIG_CLOSED()
{
    // if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
    //     DEBUG_FUNCTION_LINE("Failed to close storage");
    // }
}
