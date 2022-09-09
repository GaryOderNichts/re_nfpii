#pragma once
#include <wut.h>
#include <nn/ffl/miidata.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NTAGInfo NTAGInfo;
typedef struct NTAGApplicationData NTAGApplicationData;
typedef struct NTAGRawData NTAGRawData;
typedef struct NTAGData NTAGData;

struct WUT_PACKED NTAGInfo
{
    uint8_t magic;
    uint16_t cryptCount;
    uint8_t zero;
    uint8_t flags_hi;
    uint8_t characterInfo[3];
    uint16_t modelNumber;
    uint8_t figureType;
    uint8_t series;
    uint8_t unk0[4];
    uint16_t writeCount;
    uint16_t appDataUpdateCount;
    uint32_t crc;
    uint8_t flags_lo;
    uint16_t name[10];
    FFLStoreData mii;
    uint8_t countryCode;
    uint16_t setupDate;
    uint16_t lastWriteDate;
    uint32_t appId;
    uint64_t titleId;
    uint8_t unk1[0x1FC];
};
WUT_CHECK_SIZE(NTAGInfo, 0x29A);

struct WUT_PACKED NTAGApplicationData
{
    uint16_t size;
    uint8_t data[0xD8];
};
WUT_CHECK_SIZE(NTAGApplicationData, 0xDA);

struct WUT_PACKED NTAGRawData
{
    uint8_t serial[9];
    uint8_t internal;
    uint8_t lock_bytes[2];
    uint8_t capabilityContainer[4];
    struct WUT_PACKED {
        uint8_t magic;
        uint16_t cryptCount;
        uint8_t zero;
        uint8_t flags;
        uint8_t countryCode;
        uint16_t appDataUpdateCount;
        uint16_t setupDate;
        uint16_t lastWriteDate;
        uint32_t crc;
        uint16_t name[10];
    } section0;
    struct WUT_PACKED {
        uint8_t hmac0[0x20];
        uint8_t characterInfo[3];
        uint8_t figureType;
        uint16_t modelNumber;
        uint8_t series;
        uint8_t version;
        uint32_t unk;
        uint8_t hmac1[0x20];
        uint8_t hmac2[0x20];
    } section1;
    struct WUT_PACKED {
        FFLStoreData mii;
        uint64_t titleId;
        uint16_t writeCount;
        uint32_t appId;
        uint8_t unk[0x22];
    } section2;
    uint8_t applicationData[0xd8];
    uint8_t dynamicLock[4];
    uint8_t cfg0[4];
    uint8_t cfg1[4];
    WUT_UNKNOWN_BYTES(8);
};
WUT_CHECK_OFFSET(NTAGRawData, 0x10, section0);
WUT_CHECK_OFFSET(NTAGRawData, 0x34, section1);
WUT_CHECK_OFFSET(NTAGRawData, 0xa0, section2);
WUT_CHECK_SIZE(NTAGRawData, 0x21C);

struct WUT_PACKED NTAGData {
    uint8_t uidSize;
    uint8_t uid[10];
    uint8_t protocol;
    uint8_t tagType;
    WUT_UNKNOWN_BYTES(0x20);
    uint8_t formatVersion;
    NTAGInfo info;
    NTAGApplicationData appData;
    uint16_t rawSize;
    NTAGRawData rawData;
    WUT_UNKNOWN_BYTES(0x20);
};
WUT_CHECK_SIZE(NTAGData, 0x5e0);

int NTAGConvertT2T(NTAGData* out, NTAGData* in);

#ifdef __cplusplus
}
#endif
