#include <wups.h>

#include <vector>
#include <string>

enum LogType {
    LOG_TYPE_NORMAL,
    LOG_TYPE_WARN,
    LOG_TYPE_ERROR,
};

struct ConfigItemLog {
    char* configID;
    WUPSConfigItemHandle handle;
};

void ConfigItemLog_Init(void);

void ConfigItemLog_PrintType(LogType type, const char* text);

bool ConfigItemLog_AddToCategory(WUPSConfigCategoryHandle cat, const char* configID, const char* displayName);

#define ConfigItemLog_AddToCategoryHandled(__config__, __cat__, __configID__, __displayName__)  \
    do {                                                                                        \
        if (!ConfigItemLog_AddToCategory(__cat__, __configID__, __displayName__)) {             \
            WUPSConfig_Destroy(__config__);                                                     \
            return 0;                                                                           \
        }                                                                                       \
    } while (0)
