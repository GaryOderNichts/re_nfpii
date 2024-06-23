#include <wups.h>
#include <nfpii.h>
#include <vpad/input.h>
#include <padscore/wpad.h>
#include <notifications/notifications.h>

#include "config/ConfigItemSelectAmiibo.hpp"
#include "utils/input.h"
#include "debug/logger.h"

extern "C" uint32_t VPADGetButtonProcMode(VPADChan chan);

extern uint32_t currentQuickSelectCombination;
static uint32_t currentQuickSelectIndex = 0;

extern uint32_t currentToggleEmulationCombination;

static uint32_t sWPADLastButtonHold[4];
static uint32_t sWasHoldForXFrame[4];
static uint32_t sWasHoldForXFrameGamePad;

static void cycleQuickSelect()
{
    if (ConfigItemSelectAmiibo_GetFavorites().size() == 0) {
        return;
    }

    currentQuickSelectIndex++;
    if (currentQuickSelectIndex >= ConfigItemSelectAmiibo_GetFavorites().size()) {
        currentQuickSelectIndex = 0;
    }

    std::string path = ConfigItemSelectAmiibo_GetFavorites()[currentQuickSelectIndex];
    NfpiiSetTagEmulationPath(path.c_str());
    NfpiiSetEmulationState(NFPII_EMULATION_ON);

    std::string name = path.substr(path.find_last_of("/") + 1);
    std::string notifText = "re_nfpii: Selected \"" + name + "\"";

    if (NotificationModule_InitLibrary() == NOTIFICATION_MODULE_RESULT_SUCCESS) {
        NotificationModule_AddInfoNotification(notifText.c_str());
    }
}


static void toggleEmulation()
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

DECL_FUNCTION(int32_t, VPADRead, VPADChan chan, VPADStatus* buffer, uint32_t buffer_size, VPADReadError* error)
{
    VPADReadError real_error;
    int32_t result = real_VPADRead(chan, buffer, buffer_size, &real_error);

    if (result > 0 && real_error == VPAD_READ_SUCCESS) {
        uint32_t end = 1;
        // Fix games like TP HD
        if (VPADGetButtonProcMode(chan) == 1) {
            end = result;
        }
        bool found = false;
        bool foundTe = false;

        for (uint32_t i = 0; i < end; i++) {
            if (currentQuickSelectCombination != 0 && (((buffer[i].hold & 0x000FFFFF) & currentQuickSelectCombination) == currentQuickSelectCombination)) {
                found = true;
                break;
            } else if (currentToggleEmulationCombination != 0 && (((buffer[i].hold & 0x000FFFFF) & currentToggleEmulationCombination) == currentToggleEmulationCombination)) {
                foundTe = true;
                break;
            }
        }
        if (found) {
            if (sWasHoldForXFrameGamePad == 0) {
                cycleQuickSelect();
            }
            sWasHoldForXFrameGamePad++;
        } else if (foundTe) {
            if (sWasHoldForXFrameGamePad == 0) {
                toggleEmulation();
            }
            sWasHoldForXFrameGamePad++;
        } else {
            sWasHoldForXFrameGamePad = 0;
        }
    }

    if (error) {
        *error = real_error;
    }
    return result;
}

DECL_FUNCTION(void, WPADRead, WPADChan chan, WPADStatusProController* data)
{
    real_WPADRead(chan, data);

    if (chan >= 0 && chan < 4) {
        if (data[0].err == 0) {
            if (data[0].extensionType != 0xFF) {
                uint32_t curButtonHold = 0;
                if (data[0].extensionType == WPAD_EXT_CORE || data[0].extensionType == WPAD_EXT_NUNCHUK) {
                    // button data is in the first 2 bytes for wiimotes
                    curButtonHold = remapWiiMoteButtons(((uint16_t *) data)[0]);
                } else if (data[0].extensionType == WPAD_EXT_CLASSIC) {
                    curButtonHold = remapClassicButtons(((uint32_t *) data)[10] & 0xFFFF);
                } else if (data[0].extensionType == WPAD_EXT_PRO_CONTROLLER) {
                    curButtonHold = remapProButtons(data[0].buttons);
                }

                if ((currentQuickSelectCombination != 0 && (curButtonHold & currentQuickSelectCombination) == currentQuickSelectCombination)) {
                    if (sWasHoldForXFrame[chan] == 0) {
                        cycleQuickSelect();
                    }
                    sWasHoldForXFrame[chan]++;
                } else if ((currentToggleEmulationCombination != 0 && (curButtonHold & currentToggleEmulationCombination) == currentToggleEmulationCombination)) {
                    if (sWasHoldForXFrame[chan] == 0) {
                        toggleEmulation();
                    }
                    sWasHoldForXFrame[chan]++;
                } else {
                    sWasHoldForXFrame[chan] = 0;
                }

                sWPADLastButtonHold[chan] = curButtonHold;
            }
        }
    }
}

WUPS_MUST_REPLACE(VPADRead, WUPS_LOADER_LIBRARY_VPAD, VPADRead);
WUPS_MUST_REPLACE(WPADRead, WUPS_LOADER_LIBRARY_PADSCORE, WPADRead);
