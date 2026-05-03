#include <forward_list>
#include <wups.h>
#include <wups/config/WUPSConfigItemMultipleValues.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config/WUPSConfigItemButtonCombo.h>
#include <wups/config/WUPSConfigItemStub.h>
#include <wups/button_combo/api.h>
#include <string>
#include <map>

#include <whb/libmanager.h>
#include <whb/log_cafe.h>
#include <whb/log_module.h>
#include <whb/log_udp.h>

#include <nfpii.h>
#include <notifications/notifications.h>
#include <sys/stat.h>
#include <sys/syslimits.h>

#include "quick_select.h"
#include "debug/logger.h"
#include "config/ConfigItemSelectAmiibo.hpp"
#include "config/ConfigItemLog.hpp"
#include "config/ConfigItemDumpAmiibo.hpp"

#define STR_VALUE(arg) #arg
#define VERSION_STRING(x, y, z) "v" STR_VALUE(x) "." STR_VALUE(y) "." STR_VALUE(z)

WUPS_PLUGIN_NAME("re_nfpii");
WUPS_PLUGIN_DESCRIPTION("A nn_nfp reimplementation with support for Amiibo emulation");
WUPS_PLUGIN_VERSION(VERSION_STRING(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH));
WUPS_PLUGIN_AUTHOR("GaryOderNichts");
WUPS_PLUGIN_LICENSE("GPLv2");

WUPS_USE_STORAGE("re_nfpii");
WUPS_USE_WUT_DEVOPTAB();

// TODO make this dynamic again
// #define MAX_REMOVE_AFTER_SECONDS 20

#define TAG_EMULATION_PATH std::string("/vol/external01/wiiu/re_nfpii/")

uint32_t currentRemoveAfterOption = 0;

WUPSButtonCombo_Buttons currentQuickSelectCombination = QUICK_SELECT_BUTTON_COMBO_DEFAULT;
WUPSButtonCombo_Buttons currentToggleEmulationCombination = TOGGLE_EMULATION_BUTTON_COMBO_DEFAULT;

bool favoritesPerTitle = false;

static void nfpiiLogHandler(NfpiiLogVerbosity verb, const char* message)
{
    ConfigItemLog_PrintType((LogType) verb, message);
}

WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle);

void ConfigMenuClosedCallback();

INITIALIZE_PLUGIN()
{
    if (!WHBLogModuleInit()) {
        WHBLogCafeInit();
        WHBLogUdpInit();
    }

    if (NotificationModule_InitLibrary() != NOTIFICATION_MODULE_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to init notifications");
    }

    WUPSConfigAPIOptionsV1 configOptions = {.name = "re_nfpii"};
    if (WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback, ConfigMenuClosedCallback) != WUPSCONFIG_API_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to init config api");
    }

    ConfigItemLog_Init();
    NfpiiSetLogHandler(nfpiiLogHandler);

    migrateStorage();
    // Read values from config
    {
        auto emulationState = static_cast<int32_t>(NfpiiGetEmulationState());
        WUPSStorageError err;
        if ((err = WUPSStorageAPI::Get("emulationState", emulationState)) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            WUPSStorageAPI::Store("emulationState", emulationState);
        } else if (err == WUPS_STORAGE_ERROR_SUCCESS) {
            NfpiiSetEmulationState((NfpiiEmulationState)emulationState);
        }

        if ((err = WUPSStorageAPI::Get("removeAfter", currentRemoveAfterOption)) ==
            WUPS_STORAGE_ERROR_NOT_FOUND) {
            WUPSStorageAPI::Store("removeAfter", currentRemoveAfterOption);
        } else if (err == WUPS_STORAGE_ERROR_SUCCESS) {
            NfpiiSetRemoveAfterSeconds(currentRemoveAfterOption / 2.0f);
        }

        std::string path = NfpiiGetTagEmulationPath();
        if ((err = WUPSStorageAPI::Get<std::string>("currentPath", path, WUPSStorageAPI::RESIZE_EXISTING_BUFFER)) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            WUPSStorageAPI::Store("currentPath", path);
        } else if (err == WUPS_STORAGE_ERROR_SUCCESS) {
            // check that the stored path actually exists
            struct stat sb{};
            if (stat(path.c_str(), &sb) == 0 && (sb.st_mode & S_IFMT) == S_IFREG) {
                NfpiiSetTagEmulationPath(path.c_str());
            }
        }

        if ((err = WUPSStorageAPI::Get("favoritesPerTitle", favoritesPerTitle)) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            WUPSStorageAPI::Store("favoritesPerTitle", favoritesPerTitle);
        }
        ConfigItemSelectAmiibo_Init(TAG_EMULATION_PATH, favoritesPerTitle);

        // Todo check default value?
        WUPSStorageAPI::GetOrStoreDefault(BUTTON_COMBO_QUICK_SELECT_CONFIG_ID, currentQuickSelectCombination, QUICK_SELECT_BUTTON_COMBO_DEFAULT);
        WUPSStorageAPI::GetOrStoreDefault(BUTTON_COMBO_TOGGLE_EMULATION_CONFIG_ID, currentToggleEmulationCombination, TOGGLE_EMULATION_BUTTON_COMBO_DEFAULT);

        if (currentQuickSelectCombination == 0) {
            currentQuickSelectCombination = QUICK_SELECT_BUTTON_COMBO_DEFAULT;
            WUPSStorageAPI::Store(BUTTON_COMBO_QUICK_SELECT_CONFIG_ID, currentQuickSelectCombination);
        }

        // Make sure the button combo is not empty.
        if (currentToggleEmulationCombination == 0) {
            currentToggleEmulationCombination = TOGGLE_EMULATION_BUTTON_COMBO_DEFAULT;
            WUPSStorageAPI::Store(BUTTON_COMBO_TOGGLE_EMULATION_CONFIG_ID, currentToggleEmulationCombination);
        }

        if (WUPSStorageAPI::SaveStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE("Failed to save storage");
        }
    }

    // Make sure to always show notifications
    NotificationModule_SetDefaultValue(NOTIFICATION_MODULE_NOTIFICATION_TYPE_INFO, NOTIFICATION_MODULE_DEFAULT_OPTION_KEEP_UNTIL_SHOWN, true);
    NotificationModule_SetDefaultValue(NOTIFICATION_MODULE_NOTIFICATION_TYPE_ERROR, NOTIFICATION_MODULE_DEFAULT_OPTION_KEEP_UNTIL_SHOWN, true);
    RegisterButtonCombos();
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
    ConfigItemSelectAmiibo_Init(TAG_EMULATION_PATH, favoritesPerTitle);

    NfpiiSetPluginLoaded();
}

static void stateChangedCallback(ConfigItemMultipleValues* values, uint32_t index)
{
    WUPSStorageAPI::Store("emulationState", index);
    NfpiiSetEmulationState((NfpiiEmulationState) index);
}

static void removeAfterChangedCallback(ConfigItemMultipleValues* values, uint32_t index)
{
    currentRemoveAfterOption = index;
    WUPSStorageAPI::Store("removeAfter", (int32_t) currentRemoveAfterOption);
    NfpiiSetRemoveAfterSeconds(index / 2.0f);
}

static void uuidRandomizationChangedCallback(ConfigItemMultipleValues* values, uint32_t index)
{
    NfpiiSetUUIDRandomizationState((NfpiiUUIDRandomizationState) index);
}

static void amiiboSelectedCallback(ConfigItemSelectAmiibo* amiibos, const char* filePath)
{
    std::string filePathStr = filePath;
    WUPSStorageAPI::Store("currentPath", filePathStr);
    NfpiiSetTagEmulationPath(filePath);
}

static void favoritesPerTitleCallback(ConfigItemBoolean* item, bool enable)
{
    favoritesPerTitle = enable;
    WUPSStorageAPI::Store("favoritesPerTitle", favoritesPerTitle);

    // refresh favorites
    ConfigItemSelectAmiibo_Init(TAG_EMULATION_PATH, favoritesPerTitle);
}

static void quickSelectComboCallback(ConfigItemButtonCombo* item, uint32_t newValue)
{
    currentQuickSelectCombination = static_cast<WUPSButtonCombo_Buttons>(newValue);
    WUPSStorageAPI::Store(BUTTON_COMBO_QUICK_SELECT_CONFIG_ID, currentQuickSelectCombination);
}

static void toggleEmulationComboCallback(ConfigItemButtonCombo* item, uint32_t newValue)
{
    currentToggleEmulationCombination = static_cast<WUPSButtonCombo_Buttons>(newValue);
    WUPSStorageAPI::Store(BUTTON_COMBO_TOGGLE_EMULATION_CONFIG_ID, currentToggleEmulationCombination);
}


WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle)
{
    WUPSConfigCategory root = WUPSConfigCategory(rootHandle);
    try {
        auto settingsCat = WUPSConfigCategory::Create("Settings");

        constexpr WUPSConfigItemMultipleValues::ValuePair possibleValues[] = {
            {NFPII_EMULATION_OFF, "Emulation Disabled"},
            {NFPII_EMULATION_ON, "Emulation Enabled"},
        };

        // TODO: Double check if `NFPII_EMULATION_OFF`is the correct default value.
        settingsCat.add(WUPSConfigItemMultipleValues::CreateFromValue("state", "Set State",
                                                                      NFPII_EMULATION_OFF, NfpiiGetEmulationState(),
                                                                      possibleValues,
                                                                      stateChangedCallback));


        constexpr WUPSConfigItemMultipleValues::ValuePair removeAfterValues[] = {
            {0, "Never"},
            {1, "0.5s"},
            {2, "1.0s"},
            {3, "1.5s"},
            {4, "2.0s"},
            {5, "2.5s"},
            {6, "3.0s"},
            {7, "3.5s"},
            {8, "4.0s"},
            {9, "4.5s"},
            {10, "5.0s"},
            {11, "5.5s"},
            {12, "6.0s"},
            {13, "6.5s"},
            {14, "7.0s"},
            {15, "7.5s"},
            {16, "8.0s"},
            {17, "8.5s"},
            {18, "9.0s"},
            {19, "9.5s"},
            {20, "10.0s"}
        };

        settingsCat.add(WUPSConfigItemMultipleValues::CreateFromValue("remove_after", "Remove after",
                                                                      0, NfpiiGetEmulationState(),
                                                                      removeAfterValues,
                                                                      removeAfterChangedCallback));

#if 0 //TODO
        values[0].value = RANDOMIZATION_OFF;
        values[0].valueName = (char*)"Off";
        values[1].value = RANDOMIZATION_ONCE;
        values[1].valueName = (char*)"Once";
        values[2].value = RANDOMIZATION_EVERY_READ;
        values[2].valueName = (char*)"After reading";
        WUPSConfigItemMultipleValues_AddToCategoryHandled(config, cat, "random_uuid", "Randomize UUID",
                                                          NfpiiGetUUIDRandomizationState(), values, 3,
                                                          uuidRandomizationChangedCallback);
#endif


        std::string currentAmiiboPath = NfpiiGetTagEmulationPath();
        settingsCat.add(ConfigItemSelectAmiiboCPP::Create("select_amiibo", "Select Amiibo",
                                                          TAG_EMULATION_PATH.c_str(), currentAmiiboPath.c_str(),
                                                          amiiboSelectedCallback));

        settingsCat.add(WUPSConfigItemBoolean::Create("favorites_per_title", "Per-Title Favorites", false, favoritesPerTitle, favoritesPerTitleCallback));

        bool buttonCombosSupported = false;
        if (sQuickSelectButtonComboHandle != nullptr && sToggleEmulationButtonComboHandle != nullptr) {
            buttonCombosSupported = true;
            settingsCat.add(WUPSConfigItemButtonCombo::Create("quick_select_combination", "Quick Select Combo",
                                                              static_cast<WUPSButtonCombo_Buttons>(0),
                                                              sQuickSelectButtonComboHandle,
                                                              quickSelectComboCallback));

            settingsCat.add(WUPSConfigItemButtonCombo::Create("quick_remove_combination", "Toggle Emulation Combo",
                                                              static_cast<WUPSButtonCombo_Buttons>(0),
                                                              sToggleEmulationButtonComboHandle,
                                                              toggleEmulationComboCallback));
        }

        settingsCat.add(ConfigItemDumpAmiiboCPP::Create("dump_amiibo", "Dump Amiibo",
                                                        (TAG_EMULATION_PATH + "dumps").c_str()));

        settingsCat.add(ConfigItemLogCPP::Create("log", "Logs"));
        if (!buttonCombosSupported) {
            settingsCat.add(WUPSConfigItemStub::Create("Please update to latest Aroma to be able to use button combos"));
        }
        root.add(std::move(settingsCat));
    } catch (std::exception& e) {
        DEBUG_FUNCTION_LINE("Creating config menu failed: %s", e.what());
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }

    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

void ConfigMenuClosedCallback()
{
    WUPSStorageAPI::SaveStorage();
}
