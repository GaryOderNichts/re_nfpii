#include <wups.h>

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

WUPSConfigAPIStatus ConfigItemLog_Create(const char* configID, const char* displayName, WUPSConfigItemHandle* outHandle);

WUPSConfigAPIStatus ConfigItemLog_AddToCategory(WUPSConfigCategoryHandle cat, const char* configID, const char* displayName);

class ConfigItemLogCPP : public WUPSConfigItem
{
public:
    static std::optional<ConfigItemLogCPP> Create(std::optional<std::string> identifier,
                                                  std::string_view displayName,
                                                  WUPSConfigAPIStatus& err) noexcept;

    static ConfigItemLogCPP Create(std::optional<std::string> identifier,
                                   std::string_view displayName);

private:
    explicit ConfigItemLogCPP(WUPSConfigItemHandle itemHandle) : WUPSConfigItem(itemHandle)
    {
    }
};
