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

WUPS_PLUGIN_NAME("re_nfpii");
WUPS_PLUGIN_DESCRIPTION("A nn_nfp reimplementation with support for Amiibo emulation");
WUPS_PLUGIN_VERSION("v0.1.0");
WUPS_PLUGIN_AUTHOR("GaryOderNichts");
WUPS_PLUGIN_LICENSE("GPLv2");

//WUPS_USE_STORAGE("re_nfpii");
WUPS_USE_WUT_DEVOPTAB();

#define TAG_EMULATION_PATH std::string("/vol/external01/wiiu/re_nfpii/")

uint32_t currentRemoveAfterOption = 0;

INITIALIZE_PLUGIN()
{
    if (!WHBLogModuleInit()) {
        WHBLogCafeInit();
        WHBLogUdpInit();
    }
}

DEINITIALIZE_PLUGIN()
{
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

static void amiiboSelectedCallback(ConfigItemSelectAmiibo* amiibos, const char* fileName)
{
    NfpiiSetTagEmulationPath((TAG_EMULATION_PATH + fileName).c_str());
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

    ConfigItemMultipleValuesPair values[10];
    values[0].value = EMULATION_OFF;
    values[0].valueName = (char*) "Emulation Disabled";
    values[1].value = EMULATION_ON;
    values[1].valueName = (char*) "Emulation Enabled";
    WUPSConfigItemMultipleValues_AddToCategoryHandled(config, cat, "state", "Set State", NfpiiGetEmulationState(), values, 2, stateChangedCallback);

    values[0].value = 0;
    values[0].valueName = (char*) "Never";
    for (int i = 1; i < 10; i++) {
        values[i].value = i;
        char* fmt = (char*) malloc(32);
        snprintf(fmt, 32, "%.1fs", i / 2.0f);
        values[i].valueName = fmt;
    }
    WUPSConfigItemMultipleValues_AddToCategoryHandled(config, cat, "remove_after", "Remove after", currentRemoveAfterOption, values, 10, removeAfterChangedCallback);
    for (int i = 1; i < 10; i++) {
        free(values[i].valueName);
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

    std::string currentPath = NfpiiGetTagEmulationPath();
    if (currentPath.starts_with(TAG_EMULATION_PATH)) {
        currentPath.erase(0, TAG_EMULATION_PATH.length());
    }

    ConfigItemSelectAmiibo_AddToCategoryHandled(config, cat, "select_amiibo", "Select Amiibo", TAG_EMULATION_PATH.c_str(), currentPath.c_str(), amiiboSelectedCallback);

    return config;
}

WUPS_CONFIG_CLOSED()
{
    // if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
    //     DEBUG_FUNCTION_LINE("Failed to close storage");
    // }
}
