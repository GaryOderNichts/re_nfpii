#include <stdint.h>
#include <vpad/input.h>
#include <wups.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ButtonComboState {
    BUTTON_COMBO_STATE_NONE,
    BUTTON_COMBO_STATE_PREPARE_FOR_HOLD,
    BUTTON_COMBO_STATE_WAIT_FOR_HOLD,
} ButtonComboState;

typedef struct ConfigItemButtonCombo {
    char *configId;
    WUPSConfigItemHandle handle;
    uint32_t defaultValue;
    uint32_t value;
    uint32_t holdDurationInMs;
    VPADButtons abortButton;
    uint32_t abortButtonHoldDurationInMs;
    void *callback;
    ButtonComboState state;
} ConfigItemButtonCombo;

typedef void (*ButtonComboValueChangedCallback)(ConfigItemButtonCombo *item, uint32_t buttonComboInVPADButtons);

bool WUPSConfigItemButtonComboAddToCategory(WUPSConfigCategoryHandle cat, const char *configId, const char *displayName, uint32_t defaultComboInVPADButtons, ButtonComboValueChangedCallback callback);

bool WUPSConfigItemButtonComboAddToCategoryEx(WUPSConfigCategoryHandle cat, const char *configId, const char *displayName, uint32_t defaultComboInVPADButtons, uint32_t holdDurationInMs, VPADButtons abortButton, uint32_t abortButtonHoldDurationInMs, ButtonComboValueChangedCallback callback);

#define WUPSConfigItemButtonCombo_AddToCategoryHandled(__config__, __cat__, __configID__, __displayName__, __defaultComboInVPADButtons__, __callback__) \
    do {                                                                                                                                                \
        if (!WUPSConfigItemButtonComboAddToCategory(__cat__, __configID__, __displayName__, __defaultComboInVPADButtons__, __callback__)) {             \
            WUPSConfig_Destroy(__config__);                                                                                                             \
            return 0;                                                                                                                                   \
        }                                                                                                                                               \
    } while (0)

#ifdef __cplusplus
}
#endif
