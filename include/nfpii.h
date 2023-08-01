#pragma once
#include <stdint.h>
#include <nfc/nfc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NFPII_VERSION_MAJOR(v) ((v >> 16) & 0xff)
#define NFPII_VERSION_MINOR(v) ((v >> 8) & 0xff)
#define NFPII_VERSION_PATCH(v) (v & 0xff)
#define NFPII_VERSION(major, minor, patch) ((major << 16) | (minor << 8) | patch)

typedef enum NfpiiEmulationState {
    NFPII_EMULATION_OFF,
    NFPII_EMULATION_ON
} EmulationState;

typedef enum NfpiiUUIDRandomizationState {
    NFPII_RANDOMIZATION_OFF,
    NFPII_RANDOMIZATION_ONCE,
    NFPII_RANDOMIZATION_EVERY_READ
} UUIDRandomizationState;

typedef enum NfpiiLogVerbosity {
    NFPII_LOG_VERBOSITY_INFO,
    NFPII_LOG_VERBOSITY_WARN,
    NFPII_LOG_VERBOSITY_ERROR,
} NfpiiLogVerbosity;

typedef void (*NfpiiLogHandler)(NfpiiLogVerbosity verb, const char* message);

uint32_t NfpiiGetVersion(void);

bool NfpiiIsInitialized(void);

void NfpiiSetEmulationState(NfpiiEmulationState state);

NfpiiEmulationState NfpiiGetEmulationState(void);

void NfpiiSetUUIDRandomizationState(NfpiiUUIDRandomizationState state);

NfpiiUUIDRandomizationState NfpiiGetUUIDRandomizationState(void);

void NfpiiSetRemoveAfterSeconds(float seconds);

void NfpiiSetTagEmulationPath(const char* path);

const char* NfpiiGetTagEmulationPath(void);

NFCError NfpiiQueueNFCGetTagInfo(NFCGetTagInfoCallbackFn callback, void* arg);

void NfpiiSetLogHandler(NfpiiLogHandler handler);

#ifdef __cplusplus
}
#endif
