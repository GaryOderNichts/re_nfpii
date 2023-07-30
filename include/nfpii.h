#pragma once
#include <stdint.h>
#include <nfc/nfc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum NfpiiEmulationState {
    EMULATION_OFF,
    EMULATION_ON
} EmulationState;

typedef enum NfpiiUUIDRandomizationState {
    RANDOMIZATION_OFF,
    RANDOMIZATION_ONCE,
    RANDOMIZATION_EVERY_READ
} UUIDRandomizationState;

typedef enum NfpiiLogVerbosity {
    LOG_VERBOSITY_INFO,
    LOG_VERBOSITY_WARN,
    LOG_VERBOSITY_ERROR,
} NfpiiLogVerbosity;

typedef void (*NfpiiLogHandler)(NfpiiLogVerbosity verb, const char* message);

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
