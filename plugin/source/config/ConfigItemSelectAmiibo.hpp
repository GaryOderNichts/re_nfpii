#include <wups.h>

#include <vector>
#include <string>

typedef void (*AmiiboSelectedCallback)(struct ConfigItemSelectAmiibo*, const char* fileName);

struct ConfigItemSelectAmiibo {
    char* configID;
    WUPSConfigItemHandle handle;

    AmiiboSelectedCallback callback;

    std::string rootPath;
    std::string currentPath;
    std::string selectedAmiibo;
};

bool ConfigItemSelectAmiibo_AddToCategory(WUPSConfigCategoryHandle cat, const char* configID, const char* displayName, const char* amiiboFolder, const char* currentAmiibo, AmiiboSelectedCallback callback);

#define ConfigItemSelectAmiibo_AddToCategoryHandled(__config__, __cat__, __configID__, __displayName__, __amiiboFolder__, __currentAmiibo__, __callback__)  \
    do {                                                                                                                                                    \
        if (!ConfigItemSelectAmiibo_AddToCategory(__cat__, __configID__, __displayName__, __amiiboFolder__, __currentAmiibo__, __callback__)) {             \
            WUPSConfig_Destroy(__config__);                                                                                                                 \
            return 0;                                                                                                                                       \
        }                                                                                                                                                   \
    } while (0)
