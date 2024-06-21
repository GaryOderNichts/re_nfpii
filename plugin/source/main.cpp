#include <wups.h>
#include <wups/config/WUPSConfigItemMultipleValues.h>
#include <wups/config/WUPSConfigItemBoolean.h>
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
#include "config/ConfigItemDumpAmiibo.hpp"
#include "config/WUPSConfigItemButtonCombo.h"

#define STR_VALUE(arg) #arg
#define VERSION_STRING(x, y, z) "v" STR_VALUE(x) "." STR_VALUE(y) "." STR_VALUE(z)

WUPS_PLUGIN_NAME("re_nfpii");
WUPS_PLUGIN_DESCRIPTION("A nn_nfp reimplementation with support for Amiibo emulation");
WUPS_PLUGIN_VERSION(VERSION_STRING(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH));
WUPS_PLUGIN_AUTHOR("GaryOderNichts");
WUPS_PLUGIN_LICENSE("GPLv2");

WUPS_USE_STORAGE("re_nfpii");
WUPS_USE_WUT_DEVOPTAB();

// This is in .5 steps, i.e. 9 would be 4.5s
#define MAX_REMOVE_AFTER_SECONDS 20

#define TAG_EMULATION_PATH std::string("/vol/external01/wiiu/re_nfpii/")

uint32_t currentRemoveAfterOption = 0;

uint32_t currentQuickSelectCombination = 0;
uint32_t currentQuickRemoveCombination = 0;

bool favoritesPerTitle = false;

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

    // Read values from config
    WUPSStorageError err = WUPS_OpenStorage();
    if (err == WUPS_STORAGE_ERROR_SUCCESS) {
        int32_t emulationState = (int32_t) NfpiiGetEmulationState();
        if ((err = WUPS_GetInt(nullptr, "emulationState", &emulationState)) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            WUPS_StoreInt(nullptr, "emulationState", emulationState);
        } else if (err == WUPS_STORAGE_ERROR_SUCCESS) {
            NfpiiSetEmulationState((NfpiiEmulationState) emulationState);
        }

        if ((err = WUPS_GetInt(nullptr, "removeAfter", (int32_t*) &currentRemoveAfterOption)) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            WUPS_StoreInt(nullptr, "removeAfter", currentRemoveAfterOption);
        } else if (err == WUPS_STORAGE_ERROR_SUCCESS) {
            NfpiiSetRemoveAfterSeconds(currentRemoveAfterOption / 2.0f);
        }

        char path[PATH_MAX];
        strncpy(path, NfpiiGetTagEmulationPath(), PATH_MAX - 1);
        if ((err = WUPS_GetString(nullptr, "currentPath", path, PATH_MAX)) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            WUPS_StoreString(nullptr, "currentPath", path);
        } else if (err == WUPS_STORAGE_ERROR_SUCCESS) {
            // check that the stored path actually exists
            struct stat sb;
            if (stat(path, &sb) == 0 && (sb.st_mode & S_IFMT) == S_IFREG) {
                NfpiiSetTagEmulationPath(path);
            }
        }

        if ((err = WUPS_GetBool(nullptr, "favoritesPerTitle", &favoritesPerTitle)) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            WUPS_StoreBool(nullptr, "favoritesPerTitle", favoritesPerTitle);
        }
        ConfigItemSelectAmiibo_Init(TAG_EMULATION_PATH, favoritesPerTitle);

        if ((err = WUPS_GetInt(nullptr, "quickSelectCombo", (int32_t*) &currentQuickSelectCombination)) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            WUPS_StoreInt(nullptr, "quickSelectCombo", currentQuickSelectCombination);
        }

        if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE("Failed to close storage");
        }
    } else {
        DEBUG_FUNCTION_LINE("Failed to open storage");
    }
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

    // Make sure favorites are refreshed for the new title
    if (WUPS_OpenStorage() == WUPS_STORAGE_ERROR_SUCCESS) {
        ConfigItemSelectAmiibo_Init(TAG_EMULATION_PATH, favoritesPerTitle);

        if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE("Failed to close storage");
        } 
    } else {
        DEBUG_FUNCTION_LINE("Failed to open storage");
    }
}

static void stateChangedCallback(ConfigItemMultipleValues* values, uint32_t index)
{
    WUPS_StoreInt(nullptr, "emulationState", (int32_t) index);
    NfpiiSetEmulationState((NfpiiEmulationState) index);
}

static void removeAfterChangedCallback(ConfigItemMultipleValues* values, uint32_t index)
{
    currentRemoveAfterOption = index;
    WUPS_StoreInt(nullptr, "removeAfter", (int32_t) currentRemoveAfterOption);
    NfpiiSetRemoveAfterSeconds(index / 2.0f);
}

static void uuidRandomizationChangedCallback(ConfigItemMultipleValues* values, uint32_t index)
{
    NfpiiSetUUIDRandomizationState((NfpiiUUIDRandomizationState) index);
}

static void amiiboSelectedCallback(ConfigItemSelectAmiibo* amiibos, const char* filePath)
{
    WUPS_StoreString(nullptr, "currentPath", filePath);
    NfpiiSetTagEmulationPath(filePath);
}

static void favoritesPerTitleCallback(ConfigItemBoolean* item, bool enable)
{
    favoritesPerTitle = enable;
    WUPS_StoreBool(nullptr, "favoritesPerTitle", favoritesPerTitle);

    // refresh favorites
    ConfigItemSelectAmiibo_Init(TAG_EMULATION_PATH, favoritesPerTitle);
}

static void quickSelectComboCallback(ConfigItemButtonCombo* item, uint32_t newValue)
{
    currentQuickSelectCombination = newValue;
    WUPS_StoreInt(nullptr, "quickSelectCombo", (int32_t) currentQuickSelectCombination);
}

static void quickRemoveComboCallback(ConfigItemButtonCombo* item, uint32_t newValue)
{
    currentQuickRemoveCombination = newValue;
    WUPS_StoreInt(nullptr, "quickRemoveCombo", (int32_t) currentQuickRemoteCombination);
}

WUPS_GET_CONFIG()
{
    if (WUPS_OpenStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to open storage");
        return 0;
    }

    WUPSConfigHandle config;
    WUPSConfig_CreateHandled(&config, "re_nfpii");

    WUPSConfigCategoryHandle cat;
    WUPSConfig_AddCategoryByNameHandled(config, "Settings", &cat);

    ConfigItemMultipleValuesPair emulationStateValues[2];
    emulationStateValues[0].value = NFPII_EMULATION_OFF;
    emulationStateValues[0].valueName = (char*) "Emulation Disabled";
    emulationStateValues[1].value = NFPII_EMULATION_ON;
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

    WUPSConfigItemBoolean_AddToCategoryHandled(config, cat, "favorites_per_title", "Per-Title Favorites", favoritesPerTitle, favoritesPerTitleCallback);

    WUPSConfigItemButtonCombo_AddToCategoryHandled(config, cat, "quick_select_combination", "Quick Select Combo", currentQuickSelectCombination, quickSelectComboCallback);

    WUPSConfigItemButtonCombo_AddToCategoryHandled(config, cat, "quick_remove_combination", "Quick Remove Combo", currentQuickRemoveCombination, quickRemoveComboCallback);

    ConfigItemDumpAmiibo_AddToCategoryHandled(config, cat, "dump_amiibo", "Dump Amiibo", (TAG_EMULATION_PATH + "dumps").c_str());

    ConfigItemLog_AddToCategoryHandled(config, cat, "log", "Logs");

    return config;
}

WUPS_CONFIG_CLOSED()
{
    if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to close storage");
    }
}
