#pragma once

#include <ntag/ntag.h>

#ifdef __cplusplus
extern "C" {
#endif

int NTAGDecrypt(NTAGDataT2T* data, NTAGRawDataT2T* raw);

int NTAGEncrypt(NTAGRawDataT2T* raw, NTAGDataT2T* data);

#ifdef __cplusplus
}
#endif
