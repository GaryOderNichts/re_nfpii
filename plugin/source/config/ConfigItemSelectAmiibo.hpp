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

std::vector<std::string>& ConfigItemSelectAmiibo_GetFavorites();

void ConfigItemSelectAmiibo_Init(std::string rootPath, bool favoritesPerTitle);


WUPSConfigAPIStatus ConfigItemSelectAmiibo_Create(const char* configID, const char* displayName,
                                                  const char* amiiboFolder, const char* currentAmiibo,
                                                  AmiiboSelectedCallback callback,
                                                  WUPSConfigItemHandle* outHandle);

WUPSConfigAPIStatus ConfigItemSelectAmiibo_AddToCategory(WUPSConfigCategoryHandle cat, const char* configID, const char* displayName,
                                                         const char* amiiboFolder, const char* currentAmiibo,
                                                         AmiiboSelectedCallback callback);

class ConfigItemSelectAmiiboCPP : public WUPSConfigItem
{
public:
    static std::optional<ConfigItemSelectAmiiboCPP> Create(std::optional<std::string> identifier,
                                                           std::string_view displayName,
                                                           const char* amiiboFolder, const char* currentAmiibo,
                                                           AmiiboSelectedCallback callback,
                                                           WUPSConfigAPIStatus& err) noexcept;

    static ConfigItemSelectAmiiboCPP Create(std::optional<std::string> identifier,
                                            std::string_view displayName,
                                            const char* amiiboFolder, const char* currentAmiibo,
                                            AmiiboSelectedCallback callback);

private:
    explicit ConfigItemSelectAmiiboCPP(WUPSConfigItemHandle itemHandle) : WUPSConfigItem(itemHandle)
    {
    }
};
