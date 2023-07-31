#include "ConfigItemLog.hpp"
#include "utils/DrawUtils.hpp"
#include "utils/input.h"

#include <vector>
#include <string>
#include <cstdarg>
#include <cstring>
#include <algorithm>

#include <coreinit/mutex.h>
#include <vpad/input.h>
#include <padscore/kpad.h>

#define COLOR_BACKGROUND         Color(238, 238, 238, 255)
#define COLOR_TEXT               Color(51, 51, 51, 255)
#define COLOR_TEXT2              Color(72, 72, 72, 255)
#define COLOR_TEXT_WARN          Color(255, 204, 0, 255)
#define COLOR_TEXT_ERROR         Color(204, 51, 0, 255)
#define COLOR_WHITE              Color(0xFFFFFFFF)
#define COLOR_BLACK              Color(0, 0, 0, 255)

// max log entries which can be stored
#define MAX_LOG_ENTRIES 100

// max log entries displayed on page
#define MAX_ENTRIES_PER_PAGE 20

struct LogEntry {
    LogType type;
    std::string text;
};

static OSMutex logMutex;
static std::vector<LogEntry> logEntries;

void ConfigItemLog_Init(void)
{
    OSInitMutex(&logMutex);
}

void ConfigItemLog_PrintType(LogType type, const char* text)
{
    OSLockMutex(&logMutex);

    LogEntry entry;
    entry.type = type;
    entry.text = text;
    logEntries.push_back(entry);

    if (logEntries.size() > MAX_LOG_ENTRIES) {
        logEntries.erase(logEntries.begin(), logEntries.end() - MAX_LOG_ENTRIES);
        logEntries[0] = LogEntry(LOG_TYPE_NORMAL, "...");
    }

    OSUnlockMutex(&logMutex);
}

static std::string getLogStats(ConfigItemLog* item)
{
    uint32_t numErrors = 0;
    uint32_t numWarns = 0;
    for (LogEntry& e : logEntries) {
        if (e.type == LOG_TYPE_ERROR) {
            numErrors++;
        } else if (e.type == LOG_TYPE_WARN) {
            numWarns++;
        }
    }

    return std::to_string(numErrors) + " Error(s), " + std::to_string(numWarns) + " Warning(s)";
}

static void enterLogViewer(ConfigItemLog* item)
{
    // Init DrawUtils
    DrawUtils::initBuffers();
    if (!DrawUtils::initFont()) {
        return;
    }

    while (true) {
        uint32_t start = 0;
        uint32_t end = std::min(logEntries.size(), (uint32_t) MAX_ENTRIES_PER_PAGE);

        bool redraw = true;

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
                        buttonsTriggered |= remapWiiMoteButtons(kpad.trigger);
                    } else if (kpad.extensionType == WPAD_EXT_CLASSIC) {
                        buttonsTriggered |= remapClassicButtons(kpad.classic.trigger);
                    } else if (kpad.extensionType == WPAD_EXT_PRO_CONTROLLER) {
                        buttonsTriggered |= remapProButtons(kpad.pro.trigger);
                    }
                }
            }

            if (buttonsTriggered & VPAD_BUTTON_DOWN) {
                end = std::min(end + MAX_ENTRIES_PER_PAGE, logEntries.size());
                start = std::max((int) end - MAX_ENTRIES_PER_PAGE, 0);
                redraw = true;
            }

            if (buttonsTriggered & VPAD_BUTTON_UP) {
                start = std::max(0, (int) start - MAX_ENTRIES_PER_PAGE);
                end = std::min(start + MAX_ENTRIES_PER_PAGE, logEntries.size());
                redraw = true;
            }

            if (buttonsTriggered & (VPAD_BUTTON_B | VPAD_BUTTON_HOME)) {
                return;
            }

            if (redraw) {
                DrawUtils::beginDraw();
                DrawUtils::clear(COLOR_BACKGROUND);

                // draw entries
                uint32_t index = 8 + 24 + 8 + 4;
                for (uint32_t i = start; i < end; i++) {
                    LogEntry& entry = logEntries[i];

                    if (entry.type == LOG_TYPE_WARN) {
                        DrawUtils::setFontColor(COLOR_TEXT_WARN);
                    } else if (entry.type == LOG_TYPE_ERROR) {
                        DrawUtils::setFontColor(COLOR_TEXT_ERROR);
                    } else {
                        DrawUtils::setFontColor(COLOR_TEXT2);
                    }

                    DrawUtils::setFontSize(16);
                    DrawUtils::print(16, index + 16, entry.text.c_str());

                    index += 16 + 4;
                }

                DrawUtils::setFontColor(COLOR_TEXT);

                // draw top bar
                DrawUtils::setFontSize(24);
                DrawUtils::print(16, 6 + 24, "re_nfpii - Log viewer");
                DrawUtils::setFontSize(18);
                DrawUtils::print(SCREEN_WIDTH - 16, 8 + 24, getLogStats(item).c_str(), true);
                DrawUtils::drawRectFilled(8, 8 + 24 + 4, SCREEN_WIDTH - 8 * 2, 3, COLOR_BLACK);

                // draw bottom bar
                DrawUtils::drawRectFilled(8, SCREEN_HEIGHT - 24 - 8 - 4, SCREEN_WIDTH - 8 * 2, 3, COLOR_BLACK);
                DrawUtils::setFontSize(18);
                DrawUtils::print(16, SCREEN_HEIGHT - 10, "\ue07d Scroll ");
                DrawUtils::print(SCREEN_WIDTH - 16, SCREEN_HEIGHT - 10, "\ue001 Back", true);

                // draw scroll indicators
                DrawUtils::setFontSize(24);
                if (end < logEntries.size()) {
                    DrawUtils::print(SCREEN_WIDTH / 2 + 12, SCREEN_HEIGHT - 32, "\ufe3e", true);
                }
                if (start > 0) {
                    DrawUtils::print(SCREEN_WIDTH / 2 + 12, 32 + 20, "\ufe3d", true);
                }

                DrawUtils::endDraw();
                redraw = false;
            }
        }
    }
}

static bool ConfigItemLog_callCallback(void* context)
{
    return true;
}

static void ConfigItemLog_onButtonPressed(void* context, WUPSConfigButtons buttons)
{
    ConfigItemLog* item = (ConfigItemLog*) context;

    if (buttons & WUPS_CONFIG_BUTTON_A) {
        enterLogViewer(item);
    }
}

static bool ConfigItemLog_isMovementAllowed(void* context)
{
    return true;
}

static int32_t ConfigItemLog_getCurrentValueDisplay(void* context, char* out_buf, int32_t out_size)
{
    ConfigItemLog* item = (ConfigItemLog*) context;

    strncpy(out_buf, getLogStats(item).c_str(), out_size);
    return 0;
}

static void ConfigItemLog_restoreDefault(void* context)
{
}

static void ConfigItemLog_onSelected(void* context, bool isSelected)
{
}

static void ConfigItemLog_onDelete(void* context)
{
    ConfigItemLog* item = (ConfigItemLog*) context;

    free(item->configID);
    
    delete item;
}

bool ConfigItemLog_AddToCategory(WUPSConfigCategoryHandle cat, const char* configID, const char* displayName)
{
    if (!cat) {
        return false;
    }

    ConfigItemLog* item = new ConfigItemLog;
    if (!item) {
        return false;
    }

    if (configID) {
        item->configID = strdup(configID);
    } else {
        item->configID = nullptr;
    }

    WUPSConfigCallbacks_t callbacks = {
        .getCurrentValueDisplay         = &ConfigItemLog_getCurrentValueDisplay,
        .getCurrentValueSelectedDisplay = &ConfigItemLog_getCurrentValueDisplay,
        .onSelected                     = &ConfigItemLog_onSelected,
        .restoreDefault                 = &ConfigItemLog_restoreDefault,
        .isMovementAllowed              = &ConfigItemLog_isMovementAllowed,
        .callCallback                   = &ConfigItemLog_callCallback,
        .onButtonPressed                = &ConfigItemLog_onButtonPressed,
        .onDelete                       = &ConfigItemLog_onDelete
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
