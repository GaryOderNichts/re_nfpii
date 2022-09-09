#include <wups.h>
#include <wups/config/WUPSConfigItemMultipleValues.h>
#include <string>
#include <map>

#include <whb/libmanager.h>
#include <whb/log_cafe.h>
#include <whb/log_module.h>
#include <whb/log_udp.h>

#include "debug/logger.h"
#include "nn_nfp/nn_nfp.hpp"
#include "config/ConfigItemSelectAmiibo.hpp"

WUPS_PLUGIN_NAME("re_nfpii");
WUPS_PLUGIN_DESCRIPTION("A nn_nfp reimplementation with support for Amiibo emulation");
WUPS_PLUGIN_VERSION("v0.0.1");
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
    nn::nfp::tagManager.SetEmulationState((nn::nfp::TagManager::EmulationState) index);
}

static void removeAfterChangedCallback(ConfigItemMultipleValues* values, uint32_t index)
{
    currentRemoveAfterOption = index;
    nn::nfp::tagManager.SetRemoveAfterSeconds(index / 2.0f);
}

static void uuidRandomizationChangedCallback(ConfigItemMultipleValues* values, uint32_t index)
{
    nn::nfp::tagManager.SetUUIDRandomizationState((nn::nfp::TagManager::UUIDRandomizationState) index);
}

static void amiiboSelectedCallback(ConfigItemSelectAmiibo* amiibos, const char* fileName)
{
    nn::nfp::tagManager.SetTagEmulationPath(TAG_EMULATION_PATH + fileName);
}

WUPS_GET_CONFIG()
{
    // if (WUPS_OpenStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
    //     DEBUG_FUNCTION_LINE("Failed to open storage");
    //     return 0;
    // }

    WUPSConfigHandle config;
    WUPSConfig_CreateHandled(&config, "Amiibo Spoofer");

    WUPSConfigCategoryHandle cat;
    WUPSConfig_AddCategoryByNameHandled(config, "Settings", &cat);

    ConfigItemMultipleValuesPair values[10];
    values[0].value = nn::nfp::TagManager::EMULATION_OFF;
    values[0].valueName = (char*) "Emulation Disabled";
    values[1].value = nn::nfp::TagManager::EMULATION_ON;
    values[1].valueName = (char*) "Emulation Enabled";
    WUPSConfigItemMultipleValues_AddToCategoryHandled(config, cat, "state", "Set State", nn::nfp::tagManager.GetEmulationState(), values, 2, stateChangedCallback);

    values[0].value = 0;
    values[0].valueName = (char*) "Never";
    for (int i = 1; i < 10; i++) {
        values[i].value = i;
        char* fmt = (char*) malloc(32);
        snprintf(fmt, 32, "%02fs", i / 2.0f);
        values[i].valueName = fmt;
    }
    WUPSConfigItemMultipleValues_AddToCategoryHandled(config, cat, "remove_after", "Remove after", currentRemoveAfterOption, values, 10, removeAfterChangedCallback);
    for (int i = 1; i < 10; i++) {
        free(values[i].valueName);
    }

#if 0 //TODO
    values[0].value = nn::nfp::TagManager::RANDOMIZATION_OFF;
    values[0].valueName = (char*) "Off";
    values[1].value = nn::nfp::TagManager::RANDOMIZATION_ONCE;
    values[1].valueName = (char*) "Once";
    values[2].value = nn::nfp::TagManager::RANDOMIZATION_EVERY_READ;
    values[2].valueName = (char*) "After reading";
    WUPSConfigItemMultipleValues_AddToCategoryHandled(config, cat, "random_uuid", "Randomize UUID", nn::nfp::tagManager.GetUUIDRandomizationState(), values, 3, uuidRandomizationChangedCallback);
#endif

    std::string currentPath = nn::nfp::tagManager.GetTagEmulationPath();
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
