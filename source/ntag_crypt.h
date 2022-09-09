#pragma once

#include <ntag.h>

#ifdef __cplusplus
extern "C" {
#endif

int NTAGDecrypt(NTAGData* data, NTAGRawData* raw);

int NTAGEncrypt(NTAGRawData* raw, NTAGData* data);

#ifdef __cplusplus
}
#endif
