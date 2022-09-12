#include <wups.h>

#include <vector>
#include <string>

typedef void (*AmiiboSelectedCallback)(struct ConfigItemSelectAmiibo*, const char* fileName);

struct ConfigItemSelectAmiibo {
    char* configID;
    WUPSConfigItemHandle handle;

    AmiiboSelectedCallback callback;

    std::vector<std::string> fileNames;
    std::vector<std::string> names;

    int selected;
};

bool ConfigItemSelectAmiibo_AddToCategory(WUPSConfigCategoryHandle cat, const char* configID, const char* displayName, const char* amiiboFolder, const char* currentName, AmiiboSelectedCallback callback);

#define ConfigItemSelectAmiibo_AddToCategoryHandled(__config__, __cat__, __configID__, __displayName__, __amiiboFolder__, __currentName__, __callback__) \
    do {                                                                                                                                                                              \
        if (!ConfigItemSelectAmiibo_AddToCategory(__cat__, __configID__, __displayName__, __amiiboFolder__, __currentName__, __callback__)) {            \
            WUPSConfig_Destroy(__config__);                                                                                                                                           \
            return 0;                                                                                                                                                                 \
        }                                                                                                                                                                             \
    } while (0)
