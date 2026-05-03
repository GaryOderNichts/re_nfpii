#include "quick_select.h"

#include <forward_list>
#include <wups.h>
#include <nfpii.h>
#include <notifications/notifications.h>
#include <wups/button_combo/api.h>

#include "config/ConfigItemSelectAmiibo.hpp"
#include "debug/logger.h"

static uint32_t currentQuickSelectIndex = 0;

WUPSButtonCombo_ComboHandle sQuickSelectButtonComboHandle(nullptr);
WUPSButtonCombo_ComboHandle sToggleEmulationButtonComboHandle(nullptr);
static std::forward_list<WUPSButtonComboAPI::ButtonCombo> sButtonComboInstances;

extern WUPSButtonCombo_Buttons currentQuickSelectCombination;
extern WUPSButtonCombo_Buttons currentToggleEmulationCombination;


static uint32_t migrateButtonCombo(const uint32_t buttons)
{
    uint32_t conv_buttons = 0;

    if (buttons & VPAD_BUTTON_A) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_A;
    }
    if (buttons & VPAD_BUTTON_B) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_B;
    }
    if (buttons & VPAD_BUTTON_X) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_X;
    }
    if (buttons & VPAD_BUTTON_Y) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_Y;
    }

    if (buttons & VPAD_BUTTON_LEFT) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_LEFT;
    }
    if (buttons & VPAD_BUTTON_RIGHT) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_RIGHT;
    }
    if (buttons & VPAD_BUTTON_UP) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_UP;
    }
    if (buttons & VPAD_BUTTON_DOWN) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_DOWN;
    }

    if (buttons & VPAD_BUTTON_ZL) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_ZL;
    }
    if (buttons & VPAD_BUTTON_ZR) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_ZR;
    }

    if (buttons & VPAD_BUTTON_L) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_L;
    }
    if (buttons & VPAD_BUTTON_R) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_R;
    }

    if (buttons & VPAD_BUTTON_PLUS) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_PLUS;
    }
    if (buttons & VPAD_BUTTON_MINUS) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_MINUS;
    }

    if (buttons & VPAD_BUTTON_STICK_R) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_STICK_R;
    }
    if (buttons & VPAD_BUTTON_STICK_L) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_STICK_L;
    }

    if (buttons & VPAD_BUTTON_TV) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_TV;
    }

    return conv_buttons;
}


void migrateStorage()
{
    uint32_t oldButtonCombo = 0;
    if (WUPSStorageAPI::Get(BUTTON_COMBO_QUICK_SELECT_CONFIG_ID_DEPRECATED, oldButtonCombo) == WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Found deprecated config in storage. Storage will be migrated");
        currentQuickSelectCombination = static_cast<WUPSButtonCombo_Buttons>(migrateButtonCombo(oldButtonCombo));
        if (WUPSStorageAPI::DeleteItem(BUTTON_COMBO_QUICK_SELECT_CONFIG_ID_DEPRECATED) != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE("Failed to delete deprecated value: \"%s\" from storage", BUTTON_COMBO_QUICK_SELECT_CONFIG_ID_DEPRECATED);
        }
    }
    if (WUPSStorageAPI::Get(BUTTON_COMBO_TOGGLE_EMULATION_CONFIG_ID_DEPRECATED, oldButtonCombo) == WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Found deprecated config in storage. Storage will be migrated");
        currentToggleEmulationCombination = static_cast<WUPSButtonCombo_Buttons>(migrateButtonCombo(oldButtonCombo));
        if (WUPSStorageAPI::DeleteItem(BUTTON_COMBO_TOGGLE_EMULATION_CONFIG_ID_DEPRECATED) != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE("Failed to delete deprecated value: \"%s\" from storage", BUTTON_COMBO_TOGGLE_EMULATION_CONFIG_ID_DEPRECATED);
        }
    }
}

static void cycleQuickSelect(WUPSButtonCombo_ControllerTypes, WUPSButtonCombo_ComboHandle, void*)
{
    if (ConfigItemSelectAmiibo_GetFavorites().empty()) {
        return;
    }

    currentQuickSelectIndex++;
    if (currentQuickSelectIndex >= ConfigItemSelectAmiibo_GetFavorites().size()) {
        currentQuickSelectIndex = 0;
    }

    std::string path = ConfigItemSelectAmiibo_GetFavorites()[currentQuickSelectIndex];
    NfpiiSetTagEmulationPath(path.c_str());
    NfpiiSetEmulationState(NFPII_EMULATION_ON);

    std::string name = path.substr(path.find_last_of('/') + 1);
    std::string notifText = "re_nfpii: Selected \"" + name + "\"";

    if (NotificationModule_InitLibrary() == NOTIFICATION_MODULE_RESULT_SUCCESS) {
        NotificationModule_AddInfoNotification(notifText.c_str());
    }
}


static void toggleEmulation(WUPSButtonCombo_ControllerTypes, WUPSButtonCombo_ComboHandle, void*)
{
    NfpiiEmulationState state = NfpiiGetEmulationState();
    std::string notifText;
    if (state == NFPII_EMULATION_ON) {
        NfpiiSetEmulationState(NFPII_EMULATION_OFF);
        notifText = "re_nfpii: Disabled emulation";
    } else {
        NfpiiSetEmulationState(NFPII_EMULATION_ON);
        notifText = "re_nfpii: Enabled emulation";
    };

    if (NotificationModule_InitLibrary() == NOTIFICATION_MODULE_RESULT_SUCCESS) {
        NotificationModule_AddInfoNotification(notifText.c_str());
    }
}


template <typename... Args>
std::string string_format(const std::string& format, Args... args)
{
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    auto size = static_cast<size_t>(size_s);
    auto buf = std::make_unique<char[]>(size);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

WUPSButtonCombo_ComboHandle RegisterButtonCombo(const std::string_view label, const WUPSButtonCombo_Buttons buttonCombo,
    const WUPSButtonCombo_ComboCallback callback)
{
    const auto buttonComboLabel = string_format("re_nfpii: %s", label.data());
    WUPSButtonCombo_ComboStatus status = WUPS_BUTTON_COMBO_COMBO_STATUS_INVALID_STATUS;
    WUPSButtonCombo_Error err = WUPS_BUTTON_COMBO_ERROR_UNKNOWN_ERROR;
    auto res = WUPSButtonComboAPI::CreateComboPressDown(buttonComboLabel,
                                                        buttonCombo,
                                                        callback,
                                                        nullptr,
                                                        status,
                                                        err);
    if (!res || err != WUPS_BUTTON_COMBO_ERROR_SUCCESS) {
        if (buttonCombo != 0) { // Old button combo module doesn't support empty button combos
            const std::string errorMsg = string_format("re_nfpii: Failed to register button combo \"%s\"", label.data());
            DEBUG_FUNCTION_LINE("%s", errorMsg.c_str());
            NotificationModule_AddErrorNotification(errorMsg.c_str());
        }
    } else {
        if (status == WUPS_BUTTON_COMBO_COMBO_STATUS_CONFLICT) {
            const auto conflictMsg = string_format("re_nfpii: \"%s\"-combo was disabled due to a conflict. Please assign a different combo", label.data());
            DEBUG_FUNCTION_LINE("%s", conflictMsg.c_str());

            NotificationModule_AddInfoNotification(conflictMsg.c_str());
        } else if (status != WUPS_BUTTON_COMBO_COMBO_STATUS_VALID) {
            const auto conflictMsg = string_format("re_nfpii: Unknown error happened while registering button combo \"%s\"", label.data());
            DEBUG_FUNCTION_LINE("%s", conflictMsg.c_str());

            NotificationModule_AddInfoNotification(conflictMsg.c_str());
        }
        const auto handle = res->getHandle();
        sButtonComboInstances.emplace_front(std::move(*res));
        return handle;
    }
    return WUPSButtonCombo_ComboHandle(nullptr);
}

void RegisterButtonCombos()
{
    sQuickSelectButtonComboHandle = RegisterButtonCombo("Quick Select", currentQuickSelectCombination, cycleQuickSelect);
    sToggleEmulationButtonComboHandle = RegisterButtonCombo("Toggle Emulation", currentToggleEmulationCombination, toggleEmulation);
}
