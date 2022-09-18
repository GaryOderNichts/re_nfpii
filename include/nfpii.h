#pragma once
#include <stdint.h>
#include <nfc.h>

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

bool NfpiiIsInitialized(void);

void NfpiiSetEmulationState(NfpiiEmulationState state);

NfpiiEmulationState NfpiiGetEmulationState(void);

void NfpiiSetUUIDRandomizationState(NfpiiUUIDRandomizationState state);

NfpiiUUIDRandomizationState NfpiiGetUUIDRandomizationState(void);

void NfpiiSetRemoveAfterSeconds(float seconds);

void NfpiiSetTagEmulationPath(const char* path);

const char* NfpiiGetTagEmulationPath(void);

NFCError NfpiiQueueNFCGetTagInfo(NFCTagInfoCallback callback, void* arg);

#ifdef __cplusplus
}
#endif
