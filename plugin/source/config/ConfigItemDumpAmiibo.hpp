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

WUPSConfigAPIStatus ConfigItemDumpAmiibo_Create(const char* configID, const char* displayName, const char* dumpFolder, WUPSConfigItemHandle* outHandle);


WUPSConfigAPIStatus ConfigItemDumpAmiibo_AddToCategory(WUPSConfigCategoryHandle cat, const char* configID, const char* displayName,
                                                       const char* dumpFolder);

class ConfigItemDumpAmiiboCPP : public WUPSConfigItem
{
public:
    static std::optional<ConfigItemDumpAmiiboCPP> Create(std::optional<std::string> identifier,
                                                         std::string_view displayName,
                                                         const char* dumpFolder,
                                                         WUPSConfigAPIStatus& err) noexcept;

    static ConfigItemDumpAmiiboCPP Create(std::optional<std::string> identifier,
                                          std::string_view displayName,
                                          const char* dumpFolder);

private:
    explicit ConfigItemDumpAmiiboCPP(WUPSConfigItemHandle itemHandle) : WUPSConfigItem(itemHandle)
    {
    }
};
