#include "ConfigItemSelectAmiibo.hpp"
#include "debug/logger.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>

#include <coreinit/screen.h>
#include <vpad/input.h>
#include <padscore/kpad.h>

#define MAX_AMIIBOS_PER_PAGE 14

#define OSScreenClearBuffer(color) \
    OSScreenClearBufferEx(SCREEN_TV, color); \
    OSScreenClearBufferEx(SCREEN_DRC, color)

#define OSScreenFlipBuffers() \
    OSScreenFlipBuffersEx(SCREEN_TV); \
    OSScreenFlipBuffersEx(SCREEN_DRC)

static void OSScreenPrint(uint32_t row, uint32_t column, const char* fmt, ...)
{
    char buffer[1024];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    
    OSScreenPutFontEx(SCREEN_TV, row, column, buffer);
    OSScreenPutFontEx(SCREEN_DRC, row, column, buffer);

    va_end(args);
}

static void ConfigItemSelectAmiibo_onDelete(void* context);
static bool ConfigItemSelectAmiibo_callCallback(void* context);

static void drawSelectionMenu(int index, int selected, int& start, int& end, uint32_t currentAmiibosPerPage, const std::vector<std::string>& names)
{
    OSScreenClearBuffer(0);

    int drawIndex = 0;
    OSScreenPrint(0, drawIndex, "Select an Amiibo - Use the DPAD to navigate");
    OSScreenPrint(0, ++drawIndex, "Press A to select, B to return");

    drawIndex++;

    if (index >= end) {
        end = index + 1;
        start = end - currentAmiibosPerPage;
    }
    else if (index < start) {
        start = index;
        end = start + currentAmiibosPerPage;
    }

    for (int i = start; i < end; i++) {
        if (i == index) {
            OSScreenPrint(1, drawIndex + 1, "--> %s", names[i].c_str());
        }
        else {
            OSScreenPrint(1, drawIndex + 1, "    %s", names[i].c_str());
        }

        if (i == selected) {
            OSScreenPrint(0, drawIndex + 1, "#");
        }

        drawIndex++;
    }

    OSScreenFlipBuffers();
}

static void enterSelectionMenu(ConfigItemSelectAmiibo* item)
{
    uint32_t currentAmiibosPerPage = (item->names.size() > MAX_AMIIBOS_PER_PAGE) ? MAX_AMIIBOS_PER_PAGE : item->names.size();

    int currentIndex = item->selected > 0 ? item->selected : 0;
    int start = 0;
    int end = currentAmiibosPerPage;
    int amount = (int) item->names.size();

    drawSelectionMenu(currentIndex, item->selected, start, end, currentAmiibosPerPage, item->names);

    VPADStatus vpad{};
    VPADReadError vpadError;
    KPADStatus kpad{};
    KPADError kpadError;

    while (true) {
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
                    if (kpad.trigger & WPAD_BUTTON_DOWN) {
                        buttonsTriggered |= VPAD_BUTTON_DOWN;
                    }
                    if (kpad.trigger & WPAD_BUTTON_UP) {
                        buttonsTriggered |= VPAD_BUTTON_UP;
                    }
                    if (kpad.trigger & WPAD_BUTTON_A) {
                        buttonsTriggered |= VPAD_BUTTON_A;
                    }
                    if (kpad.trigger & WPAD_BUTTON_B) {
                        buttonsTriggered |= VPAD_BUTTON_B;
                    }
                } else {
                    if (kpad.classic.trigger & WPAD_CLASSIC_BUTTON_DOWN) {
                        buttonsTriggered |= VPAD_BUTTON_DOWN;
                    }
                    if (kpad.classic.trigger & WPAD_CLASSIC_BUTTON_UP) {
                        buttonsTriggered |= VPAD_BUTTON_UP;
                    }
                    if (kpad.classic.trigger & WPAD_CLASSIC_BUTTON_A) {
                        buttonsTriggered |= VPAD_BUTTON_A;
                    }
                    if (kpad.classic.trigger & WPAD_CLASSIC_BUTTON_B) {
                        buttonsTriggered |= VPAD_BUTTON_B;
                    }
                }
            }
        }

        if (buttonsTriggered & VPAD_BUTTON_DOWN) {
            if (currentIndex < amount - 1) {
                drawSelectionMenu(++currentIndex, item->selected, start, end, currentAmiibosPerPage, item->names);
            }
        }

        if (buttonsTriggered & VPAD_BUTTON_UP) {
            if (currentIndex > 0) {
                drawSelectionMenu(--currentIndex, item->selected, start, end, currentAmiibosPerPage, item->names);
            }
        }

        if (buttonsTriggered & VPAD_BUTTON_A) {
            item->selected = currentIndex;
            drawSelectionMenu(currentIndex, item->selected, start, end, currentAmiibosPerPage, item->names);
        }

        if (buttonsTriggered & VPAD_BUTTON_B) {
            // calling this manually is bleh but that way the config entry gets updated immediately after returning
            ConfigItemSelectAmiibo_callCallback(item);

            OSScreenClearBuffer(0);
            break;
        }
    }
}

static bool ConfigItemSelectAmiibo_callCallback(void* context)
{
    ConfigItemSelectAmiibo* item = (ConfigItemSelectAmiibo*) context;

    if (item->callback && item->selected >= 0 && item->fileNames.size() > (uint32_t) item->selected) {
        item->callback(item, item->fileNames[item->selected].c_str());
        return true;
    }

    return false;
}

static void ConfigItemSelectAmiibo_onButtonPressed(void* context, WUPSConfigButtons buttons)
{
    ConfigItemSelectAmiibo* item = (ConfigItemSelectAmiibo*) context;

    if (buttons & WUPS_CONFIG_BUTTON_A) {
        enterSelectionMenu(item);
    }
}

static bool ConfigItemSelectAmiibo_isMovementAllowed(void* context)
{
    return true;
}

static int32_t ConfigItemSelectAmiibo_getCurrentValueDisplay(void* context, char* out_buf, int32_t out_size)
{
    ConfigItemSelectAmiibo* item = (ConfigItemSelectAmiibo*) context;

    if (item->selected >= 0 && item->names.size() > (uint32_t) item->selected) {
        strncpy(out_buf, item->names[item->selected].c_str(), out_size);
        return 0;
    }

    strncpy(out_buf, "None", out_size);
    return 0;
}

static void ConfigItemSelectAmiibo_restoreDefault(void* context)
{
    ConfigItemSelectAmiibo* item = (ConfigItemSelectAmiibo*) context;
    item->selected = -1;
}

static void ConfigItemSelectAmiibo_onSelected(void* context, bool isSelected)
{
}

bool ConfigItemSelectAmiibo_AddToCategory(WUPSConfigCategoryHandle cat, const char* configID, const char* displayName, const char* amiiboFolder, const char* currentName, AmiiboSelectedCallback callback)
{
    if (!cat || !displayName || !amiiboFolder || !currentName) {
        return false;
    }

    ConfigItemSelectAmiibo* item = new ConfigItemSelectAmiibo;
    if (!item) {
        return false;
    }

    item->callback = callback;
    item->selected = -1;

    struct dirent* ent;
    DIR* amiiboDir = opendir(amiiboFolder);
    if (amiiboDir) {
        while ((ent = readdir(amiiboDir)) != NULL) {
            if (ent->d_type & DT_REG) {
                item->fileNames.push_back(ent->d_name);

                // Add the name without extension
                std::string name = ent->d_name;
                item->names.push_back(name.substr(0, name.find_last_of(".")));

                // Check if this is the current item
                if (std::strcmp(ent->d_name, currentName) == 0) {
                    item->selected = item->names.size() - 1;
                }
            }
        }
    }
    else {
        DEBUG_FUNCTION_LINE("Cannot open amiibo folder");
    }

    if (configID) {
        item->configID = strdup(configID);
    } else {
        item->configID = nullptr;
    }

    WUPSConfigCallbacks_t callbacks = {
        .getCurrentValueDisplay         = &ConfigItemSelectAmiibo_getCurrentValueDisplay,
        .getCurrentValueSelectedDisplay = &ConfigItemSelectAmiibo_getCurrentValueDisplay,
        .onSelected                     = &ConfigItemSelectAmiibo_onSelected,
        .restoreDefault                 = &ConfigItemSelectAmiibo_restoreDefault,
        .isMovementAllowed              = &ConfigItemSelectAmiibo_isMovementAllowed,
        .callCallback                   = &ConfigItemSelectAmiibo_callCallback,
        .onButtonPressed                = &ConfigItemSelectAmiibo_onButtonPressed,
        .onDelete                       = &ConfigItemSelectAmiibo_onDelete
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

static void ConfigItemSelectAmiibo_onDelete(void* context)
{
    ConfigItemSelectAmiibo* item = (ConfigItemSelectAmiibo*) context;

    free(item->configID);
    
    delete item;
}
