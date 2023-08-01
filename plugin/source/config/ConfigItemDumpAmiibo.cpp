#include "ConfigItemDumpAmiibo.hpp"
#include "utils/DrawUtils.hpp"
#include "utils/input.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <ntag/ntag.h>
#include <vpad/input.h>
#include <padscore/kpad.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <coreinit/debug.h>

#define COLOR_BACKGROUND         Color(238, 238, 238, 255)
#define COLOR_TEXT               Color(51, 51, 51, 255)
#define COLOR_TEXT2              Color(72, 72, 72, 255)
#define COLOR_TEXT_WARN          Color(255, 204, 0, 255)
#define COLOR_TEXT_ERROR         Color(204, 51, 0, 255)
#define COLOR_WHITE              Color(0xFFFFFFFF)
#define COLOR_BLACK              Color(0, 0, 0, 255)

#define TIMEOUT_SECONDS 10

static void _mkdir(const char *dir)
{
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    strncpy(tmp, dir, sizeof(tmp) - 1);

    // remove trailing /
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = '\0';

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';

            // we can't create /vol and /vol/external01
            if (strcmp(tmp, "/vol") != 0 && strcmp(tmp, "/vol/external01") != 0) {
                mkdir(tmp, S_IRWXU);
            }

            *p = '/';
        }
    }

    mkdir(tmp, S_IRWXU);
}

static void ntagReadCallback(VPADChan chan, NTAGError error, NFCTagInfo *tagInfo, NTAGRawDataContainerT2T *rawData, void *userContext)
{
    ConfigItemDumpAmiibo* item = (ConfigItemDumpAmiibo*) userContext;

    if (error == 0) {
        char filePath[PATH_MAX];
        OSCalendarTime ct;

        OSTicksToCalendarTime(OSGetTime(), &ct);
        snprintf(filePath, PATH_MAX, "%s/%04d-%02d-%02d_%02d-%02d-%02d_%02X%02X%02X%02X%02X%02X%02X.bin", item->dumpFolder.c_str(),
            ct.tm_year, ct.tm_mon + 1, ct.tm_mday, ct.tm_hour, ct.tm_min, ct.tm_sec,
            tagInfo->uid[0], tagInfo->uid[1], tagInfo->uid[2], tagInfo->uid[3], tagInfo->uid[4], tagInfo->uid[5], tagInfo->uid[6]);

        item->lastDumpPath = filePath;

        FILE* f = fopen(filePath, "wb");
        if (!f) {
            item->state = DUMP_STATE_ERROR;
            return; 
        }

        fwrite(&rawData->data, 1, rawData->size, f);
        fclose(f);

        item->state = DUMP_STATE_COMPLETED;
    } else if (error == -0x3e5) {
        // timeout
        item->state = DUMP_STATE_INIT;
        return;
    } else {
        item->state = DUMP_STATE_ERROR;
        return;
    }
}

static void ntagAbortCallback(VPADChan chan, NTAGError error, void *userContext)
{
    ConfigItemDumpAmiibo* item = (ConfigItemDumpAmiibo*) userContext;

    if (error == 0) {
        item->state = DUMP_STATE_INIT;
    } else {
        item->state = DUMP_STATE_ERROR;
    }
}

static void enterDumpMenu(ConfigItemDumpAmiibo* item)
{
    // Init DrawUtils
    DrawUtils::initBuffers();
    if (!DrawUtils::initFont()) {
        return;
    }

    item->state = DUMP_STATE_INIT;

    // create the dump folder
    _mkdir(item->dumpFolder.c_str());

    VPADStatus vpad{};
    VPADReadError vpadError;
    KPADStatus kpad{};
    KPADError kpadError;

    OSTick readStart = 0;

    while (true) {
        refresh: ;

        uint32_t buttonsTriggered = 0;

        VPADRead(VPAD_CHAN_0, &vpad, 1, &vpadError);
        if (vpadError == VPAD_READ_SUCCESS) {
            buttonsTriggered = vpad.trigger;
        }

        // read kpads and remap the buttons we need
        for (int i = 0; i < 4; i++) {
            if (KPADReadEx((KPADChan) i, &kpad, 1, &kpadError) > 0) {
                if (kpadError != KPAD_ERROR_OK) {
                    continue;
                }

                if (kpad.extensionType == WPAD_EXT_CORE || kpad.extensionType == WPAD_EXT_NUNCHUK ||
                    kpad.extensionType == WPAD_EXT_MPLUS || kpad.extensionType == WPAD_EXT_MPLUS_NUNCHUK) {
                    buttonsTriggered |= remapWiiMoteButtons(kpad.trigger);
                } else if (kpad.extensionType == WPAD_EXT_CLASSIC) {
                    buttonsTriggered |= remapClassicButtons(kpad.classic.trigger);
                } else if (kpad.extensionType == WPAD_EXT_PRO_CONTROLLER) {
                    buttonsTriggered |= remapProButtons(kpad.pro.trigger);
                }
            }
        }

        if (item->state != DUMP_STATE_WAITING) {
            if (buttonsTriggered & (VPAD_BUTTON_B | VPAD_BUTTON_HOME)) {
                break;
            }
        }

        if (item->state == DUMP_STATE_INIT) {
            if (buttonsTriggered & VPAD_BUTTON_A) {
                if (NTAGIsInit(VPAD_CHAN_0)) {
                    if (item->wasInit) {
                        // if we were the one initializing it, everything is fine
                        if (NTAGReadT2TRawData(VPAD_CHAN_0, TIMEOUT_SECONDS * 1000, nullptr, nullptr, ntagReadCallback, item) != 0) {
                            NTAGShutdown(VPAD_CHAN_0);
                            item->state = DUMP_STATE_ERROR;
                            goto refresh;
                        }

                        readStart = OSGetSystemTick();
                        item->state = DUMP_STATE_WAITING;
                        goto refresh;
                    }

                    // If NTAG is already initialized, we error out to not mess with the application using it
                    item->state = DUMP_STATE_ERROR;
                    goto refresh;
                }

                item->wasInit = true;

                if (NTAGInit(VPAD_CHAN_0) != 0) {
                    item->state = DUMP_STATE_ERROR;
                    goto refresh;
                }

                // Wait for NFC to be ready
                OSTick start = OSGetSystemTick();
                while (!NTAGIsInit(VPAD_CHAN_0) && (OSGetSystemTick() - start) < (OSTick) OSSecondsToTicks(2)) {
                    NTAGProc(VPAD_CHAN_0);
                    OSYieldThread();
                }
                NTAGProc(VPAD_CHAN_0);

                if (!NTAGIsInit(VPAD_CHAN_0)) {
                    NTAGShutdown(VPAD_CHAN_0);
                    item->state = DUMP_STATE_ERROR;
                    goto refresh;
                }

                if (NTAGReadT2TRawData(VPAD_CHAN_0, TIMEOUT_SECONDS * 1000, nullptr, nullptr, ntagReadCallback, item) != 0) {
                    NTAGShutdown(VPAD_CHAN_0);
                    item->state = DUMP_STATE_ERROR;
                    goto refresh;
                }

                readStart = OSGetSystemTick();
                item->state = DUMP_STATE_WAITING;
            }
        } else if (item->state == DUMP_STATE_WAITING) {
            NTAGProc(VPAD_CHAN_0);
            if (buttonsTriggered & VPAD_BUTTON_B) {
                NTAGAbort(VPAD_CHAN_0, ntagAbortCallback, item);
            }
        } else if (item->state == DUMP_STATE_COMPLETED || item->state == DUMP_STATE_ERROR) {
            if (buttonsTriggered & VPAD_BUTTON_A) {
                item->state = DUMP_STATE_INIT;
            }
        }

        {
            DrawUtils::beginDraw();
            DrawUtils::clear(COLOR_BACKGROUND);

            DrawUtils::setFontColor(COLOR_TEXT);

            DrawUtils::setFontSize(26);

            if (item->state == DUMP_STATE_INIT) {
                int yOff = SCREEN_HEIGHT / 2 - (6 * 26) / 2 - 8;
                const char* text = "Welcome to the Amiibo Dumper!";
                DrawUtils::print(SCREEN_WIDTH / 2 - DrawUtils::getTextWidth(text) / 2, yOff, text);
                yOff += 26;

                text = "After pressing \ue000 hold one of your Amiibo figures";
                DrawUtils::print(SCREEN_WIDTH / 2 - DrawUtils::getTextWidth(text) / 2, yOff, text);
                yOff += 26;

                text = "to the \ue099 symbol on your Gamepad.";
                DrawUtils::print(SCREEN_WIDTH / 2 - DrawUtils::getTextWidth(text) / 2, yOff, text);
                yOff += 26 + 16;

                text = "The dump will be saved to the";
                DrawUtils::print(SCREEN_WIDTH / 2 - DrawUtils::getTextWidth(text) / 2, yOff, text);
                yOff += 26;

                std::string folderText = item->dumpFolder.substr(sizeof("/vol/external01/") - 1);
                DrawUtils::print(SCREEN_WIDTH / 2 - DrawUtils::getTextWidth(folderText.c_str()) / 2, yOff, folderText.c_str());
                yOff += 26;

                text = "folder on your SD Card.";
                DrawUtils::print(SCREEN_WIDTH / 2 - DrawUtils::getTextWidth(text) / 2, yOff, text);
            } else if (item->state == DUMP_STATE_WAITING) {
                int yOff = SCREEN_HEIGHT / 2 - (2 * 26) / 2;
                const char* text = "Waiting for Amiibo...";
                DrawUtils::print(SCREEN_WIDTH / 2 - DrawUtils::getTextWidth(text) / 2, yOff, text);
                yOff += 26;

                std::string timeoutText = std::to_string(TIMEOUT_SECONDS - OSTicksToSeconds(OSGetSystemTick() - readStart)) + " seconds remaining.";
                DrawUtils::print(SCREEN_WIDTH / 2 - DrawUtils::getTextWidth(timeoutText.c_str()) / 2, yOff, timeoutText.c_str());
            } else if (item->state == DUMP_STATE_COMPLETED) {
                int yOff = SCREEN_HEIGHT / 2 - (26 + 22) / 2;
                const char* text = "Dump complete! Dump was saved to:";
                DrawUtils::print(SCREEN_WIDTH / 2 - DrawUtils::getTextWidth(text) / 2, yOff, text);
                yOff += 26;

                DrawUtils::setFontSize(22);
                std::string dumpPathText = item->lastDumpPath.substr(sizeof("/vol/external01/") - 1);
                DrawUtils::print(SCREEN_WIDTH / 2 - DrawUtils::getTextWidth(dumpPathText.c_str()) / 2, yOff, dumpPathText.c_str());
            } else if (item->state == DUMP_STATE_ERROR) {
                int yOff = SCREEN_HEIGHT / 2 - (3 * 26) / 2;
                const char* text = "An error has occured!";
                DrawUtils::print(SCREEN_WIDTH / 2 - DrawUtils::getTextWidth(text) / 2, yOff, text);
                yOff += 26;

                text = "Make sure the Gamepad is properly connected";
                DrawUtils::print(SCREEN_WIDTH / 2 - DrawUtils::getTextWidth(text) / 2, yOff, text);
                yOff += 26;

                text = "and no other title is using NFC.";
                DrawUtils::print(SCREEN_WIDTH / 2 - DrawUtils::getTextWidth(text) / 2, yOff, text);
            }

            // draw top bar
            DrawUtils::setFontSize(24);
            DrawUtils::print(16, 6 + 24, "re_nfpii - Dump Amiibo");
            DrawUtils::setFontSize(18);
            DrawUtils::drawRectFilled(8, 8 + 24 + 4, SCREEN_WIDTH - 8 * 2, 3, COLOR_BLACK);

            // draw bottom bar
            DrawUtils::drawRectFilled(8, SCREEN_HEIGHT - 24 - 8 - 4, SCREEN_WIDTH - 8 * 2, 3, COLOR_BLACK);
            DrawUtils::setFontSize(18);

            if (item->state == DUMP_STATE_WAITING) {
                DrawUtils::print(16, SCREEN_HEIGHT - 10, "\ue001 Abort");
            } else {
                DrawUtils::print(16, SCREEN_HEIGHT - 10, "\ue001 Back");
            }

            if (item->state == DUMP_STATE_COMPLETED) {
                DrawUtils::print(SCREEN_WIDTH - 16, SCREEN_HEIGHT - 10, "\ue000 Continue", true);
            } else if (item->state == DUMP_STATE_ERROR) {
                DrawUtils::print(SCREEN_WIDTH - 16, SCREEN_HEIGHT - 10, "\ue000 Try again", true);
            } else if (item->state == DUMP_STATE_INIT) {
                DrawUtils::print(SCREEN_WIDTH - 16, SCREEN_HEIGHT - 10, "\ue000 Start", true);
            }

            DrawUtils::endDraw();
        }
    }

    if (item->wasInit) {
        NTAGShutdown(VPAD_CHAN_0);
        item->wasInit = false;
    }
}

static bool ConfigItemDumpAmiibo_callCallback(void* context)
{
    return true;
}

static void ConfigItemDumpAmiibo_onButtonPressed(void* context, WUPSConfigButtons buttons)
{
    ConfigItemDumpAmiibo* item = (ConfigItemDumpAmiibo*) context;

    if (buttons & WUPS_CONFIG_BUTTON_A) {
        enterDumpMenu(item);
    }
}

static bool ConfigItemDumpAmiibo_isMovementAllowed(void* context)
{
    return true;
}

static int32_t ConfigItemDumpAmiibo_getCurrentValueDisplay(void* context, char* out_buf, int32_t out_size)
{
    *out_buf = '\0';
    return 0;
}

static void ConfigItemDumpAmiibo_restoreDefault(void* context)
{
}

static void ConfigItemDumpAmiibo_onSelected(void* context, bool isSelected)
{
}


static void ConfigItemDumpAmiibo_onDelete(void* context)
{
    ConfigItemDumpAmiibo* item = (ConfigItemDumpAmiibo*) context;

    free(item->configID);
    
    delete item;
}

bool ConfigItemDumpAmiibo_AddToCategory(WUPSConfigCategoryHandle cat, const char* configID, const char* displayName, const char* dumpFolder)
{
    if (!cat) {
        return false;
    }

    ConfigItemDumpAmiibo* item = new ConfigItemDumpAmiibo;
    if (!item) {
        return false;
    }

    if (configID) {
        item->configID = strdup(configID);
    } else {
        item->configID = nullptr;
    }

    item->state = DUMP_STATE_INIT;
    item->wasInit = false;
    item->dumpFolder = dumpFolder;

    WUPSConfigCallbacks_t callbacks = {
        .getCurrentValueDisplay         = &ConfigItemDumpAmiibo_getCurrentValueDisplay,
        .getCurrentValueSelectedDisplay = &ConfigItemDumpAmiibo_getCurrentValueDisplay,
        .onSelected                     = &ConfigItemDumpAmiibo_onSelected,
        .restoreDefault                 = &ConfigItemDumpAmiibo_restoreDefault,
        .isMovementAllowed              = &ConfigItemDumpAmiibo_isMovementAllowed,
        .callCallback                   = &ConfigItemDumpAmiibo_callCallback,
        .onButtonPressed                = &ConfigItemDumpAmiibo_onButtonPressed,
        .onDelete                       = &ConfigItemDumpAmiibo_onDelete
    };

    if (WUPSConfigItem_Create(&item->handle, configID, displayName, callbacks, item) < 0) {
        delete item;
        return false;
    }

    if (WUPSConfigCategory_AddItem(cat, item->handle) < 0) {
        return false;
    }

    return true;
}

