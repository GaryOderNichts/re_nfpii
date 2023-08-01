#include <wups.h>

#include <string>

typedef enum {
    DUMP_STATE_INIT,
    DUMP_STATE_ERROR,
    DUMP_STATE_WAITING,
    DUMP_STATE_COMPLETED,
} DumpState;

struct ConfigItemDumpAmiibo {
    char* configID;
    WUPSConfigItemHandle handle;
    DumpState state;
    bool wasInit;
    std::string dumpFolder;
    std::string lastDumpPath;
};

bool ConfigItemDumpAmiibo_AddToCategory(WUPSConfigCategoryHandle cat, const char* configID, const char* displayName, const char* dumpFolder);

#define ConfigItemDumpAmiibo_AddToCategoryHandled(__config__, __cat__, __configID__, __displayName__, __dumpFolder__)  \
    do {                                                                                                               \
        if (!ConfigItemDumpAmiibo_AddToCategory(__cat__, __configID__, __displayName__, __dumpFolder__)) {             \
            WUPSConfig_Destroy(__config__);                                                                            \
            return 0;                                                                                                  \
        }                                                                                                              \
    } while (0)
