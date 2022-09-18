#pragma once
#include <wut.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NFCTagInfo NFCTagInfo;

typedef void (*NFCTagInfoCallback)(uint32_t index, int32_t result, NFCTagInfo *info, void *arg);

typedef enum NFCError
{
    NFC_ERR_OK              =  0x0,
    NFC_ERR_GET_TAG_INFO    = -0x1383,
    NFC_ERR_INVALID_COMMAND = -0x1385,
    NFC_ERR_NOT_INIT        = -0x1387,
} NFCError;

struct NFCTagInfo
{
    uint8_t uidSize;
    uint8_t uid[10];
    uint8_t protocol;
    uint8_t tag_type;
    WUT_PADDING_BYTES(0x20); // reserved
};
WUT_CHECK_SIZE(NFCTagInfo, 0x2d);

#ifdef __cplusplus
}
#endif
