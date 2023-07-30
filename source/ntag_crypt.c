#include "ntag_crypt.h"
#include "debug/logger.h"

#include <string.h>
#include <coreinit/ios.h>

/*  This file handles converting nfc data and encrypting/decrypting the 
    data using /dev/ccr_nfc. It's mainly based on what ntag.rpl does after
    reading or before writing tags.
    This should probably be replaced with a custom format without encryption
    at some point. */

typedef struct {
    uint32_t version;
    uint32_t offsets[10];
    uint8_t data[0x21c];
} NFCCryptData;
WUT_CHECK_SIZE(NFCCryptData, 0x248);

static void rawDataToCryptData(NFCCryptData* raw, NFCCryptData* crypt)
{
    memcpy(crypt, raw, sizeof(NFCCryptData));
    crypt->version = raw->version;
    crypt->offsets[0] = 0x208;
    crypt->offsets[1] = 0x29;
    crypt->offsets[2] = 0x1e8;
    crypt->offsets[3] = 0x1d4;
    crypt->offsets[4] = 0x2c;
    crypt->offsets[5] = 0x188;
    crypt->offsets[6] = 0x1dc;
    crypt->offsets[7] = 0x0;
    crypt->offsets[8] = 0x8;
    crypt->offsets[9] = 0x1b4;

    uint8_t* src = (uint8_t*) raw->data;
    uint8_t* dst = (uint8_t*) crypt->data;
    memcpy(dst + 0x1d4, src, 0x8);
    memcpy(dst, src + 0x8, 0x8);
    memcpy(dst + 0x28, src + 0x10, 0x4);
    memcpy(dst + crypt->offsets[4], src + 0x14, 0x20);
    memcpy(dst + crypt->offsets[9], src + 0x34, 0x20);
    memcpy(dst + crypt->offsets[6], src + 0x54, 0xc);
    memcpy(dst + crypt->offsets[2], src + 0x60, 0x20);
    memcpy(dst + crypt->offsets[8], src + 0x80, 0x20);
    memcpy(dst + crypt->offsets[4] + 0x20, src + 0xa0, 0x168);
    memcpy(dst + 0x208, src + 0x208, 0x14);
}

static void cryptDataToRawData(NFCCryptData* crypt, NFCCryptData* raw)
{
    memcpy(raw, crypt, sizeof(NFCCryptData));
    raw->version = crypt->version;
    raw->offsets[0] = 0x208;
    raw->offsets[1] = 0x11;
    raw->offsets[2] = 0x60;
    raw->offsets[3] = 0x0;
    raw->offsets[4] = 0x14;
    raw->offsets[5] = 0x188;
    raw->offsets[6] = 0x54;
    raw->offsets[7] = 0xc;
    raw->offsets[8] = 0x80;
    raw->offsets[9] = 0x34;

    uint8_t* src = (uint8_t*) crypt->data;
    uint8_t* dst = (uint8_t*) raw->data;
    memcpy(dst + 0x8, src, 0x8);
    memcpy(dst + 0x10, src + 0x28, 0x4);
    memcpy(dst + 0xa0, src + 0x4c, 0x168);
    memcpy(dst + raw->offsets[8], src + 0x8, 0x20);
    memcpy(dst + raw->offsets[4], src + 0x2c, 0x20);
    memcpy(dst + raw->offsets[9], src + 0x1b4, 0x20);
    memcpy(dst + raw->offsets[3], src + 0x1d4, 0x8);
    memcpy(dst + raw->offsets[6], src + 0x1dc, 0xc);
    memcpy(dst + raw->offsets[2], src + 0x1e8, 0x20);
    memcpy(dst + 0x208, src + 0x208, 0x14);
}

static int decryptGameData(NTAGRawDataT2T* data)
{
    // Only support version 2
    if (data->section1.formatVersion != 2) {
        return -1;
    }

    int ccrNfcHandle = IOS_Open("/dev/ccr_nfc", (IOSOpenMode) 0);
    if (ccrNfcHandle < 0) {
        return ccrNfcHandle;
    }

    NFCCryptData rawData;
    rawData.version = data->section1.formatVersion;
    memcpy(rawData.data, data, sizeof(NTAGRawDataT2T));

    NFCCryptData inData;
    rawDataToCryptData(&rawData, &inData);

    // Let /dev/ccr_nfc do the actual decryption
    NFCCryptData outData;
    int res = IOS_Ioctl(ccrNfcHandle, 2, &inData, sizeof(inData), &outData, sizeof(outData));
    IOS_Close(ccrNfcHandle);
    if (res < 0) {
        return res;
    }

    memset(&rawData, 0, sizeof(rawData));
    cryptDataToRawData(&outData, &rawData);

    if (rawData.version != 2) {
        return -1;
    }

    memcpy(data, rawData.data, sizeof(NTAGRawDataT2T));

    return 0;
}

static int encryptGameData(NTAGRawDataT2T* data)
{
    // Only support version 2
    if (data->section1.formatVersion != 2) {
        return -1;
    }

    int ccrNfcHandle = IOS_Open("/dev/ccr_nfc", (IOSOpenMode) 0);
    if (ccrNfcHandle < 0) {
        return ccrNfcHandle;
    }

    NFCCryptData rawData;
    rawData.version = data->section1.formatVersion;
    memcpy(rawData.data, data, sizeof(NTAGRawDataT2T));

    NFCCryptData inData;
    rawDataToCryptData(&rawData, &inData);

    // Let /dev/ccr_nfc do the actual encryption
    NFCCryptData outData;
    int res = IOS_Ioctl(ccrNfcHandle, 1, &inData, sizeof(inData), &outData, sizeof(outData));
    IOS_Close(ccrNfcHandle);
    if (res < 0) {
        return res;
    }

    memset(&rawData, 0, sizeof(rawData));
    cryptDataToRawData(&outData, &rawData);

    if (rawData.version != 2) {
        return -1;
    }

    memcpy(data, rawData.data, sizeof(NTAGRawDataT2T));

    return 0;
}

int NTAGDecrypt(NTAGDataT2T* data, NTAGRawDataT2T* raw)
{
    // Verify tag version
    if (raw->section1.formatVersion != 2) {
        DEBUG_FUNCTION_LINE("Invalid format version: %u", raw->section1.formatVersion);
        return -1;
    }

    // Copy UID, set proto and tag type
    data->tagInfo.uidSize = 0x7;
    memcpy(data->tagInfo.uid, raw->uid, data->tagInfo.uidSize);
    data->tagInfo.technology = NFC_TECHNOLOGY_A;
    data->tagInfo.protocol = NFC_PROTOCOL_T2T;

    // Verify lock and cfg bytes
    if (raw->lockBytes[0] != 0x0f || raw->lockBytes[1] != 0xe0 || raw->dynamicLock[0] != 0x01
       || raw->dynamicLock[1] != 0x00 || raw->dynamicLock[2] != 0x0f || raw->cfg0[0] != 0x00 
       || raw->cfg0[1] != 0x00 || raw->cfg0[2] != 0x00 || raw->cfg0[3] != 0x04 || raw->cfg1[0] != 0x5f
       || raw->cfg1[2] != 0x00 || raw->cfg1[3] != 0x00) {
        DEBUG_FUNCTION_LINE("Invalid lock/cfg bit !");
        return -1;
    }

    // Store raw data
    data->raw.size = sizeof(NTAGRawDataT2T);
    memcpy(&data->raw.data, raw, sizeof(NTAGRawDataT2T));

    // Decrypt
    int res = decryptGameData(raw);
    if (res != 0) {
        DEBUG_FUNCTION_LINE("Failed to decrypt data");
        return res;
    }

    // Convert
    data->info.figureVersion = raw->section0.figureVersion;
    data->info.setupDate = raw->section0.setupDate;
    data->info.magic = raw->section0.magic;
    data->info.writes = raw->section0.writes;
    data->info.flags = raw->section0.flags >> 4;
    data->info.country = raw->section0.country;
    data->info.fontRegion = raw->section0.flags & 0xf;
    data->info.crcCounter = raw->section0.crcCounter;
    data->info.lastWriteDate = raw->section0.lastWriteDate;
    memcpy(&data->info.crc, &raw->section0.crc, sizeof(raw->section0.crc));
    memcpy(data->info.name, raw->section0.name, sizeof(raw->section0.name));
    data->info.accessID = raw->section2.accessID;
    memcpy(data->info.characterID, raw->section1.characterID, sizeof(raw->section1.characterID));
    data->info.figureType = raw->section1.figureType;
    data->info.numberingID = raw->section1.numberingID;
    data->info.seriesID = raw->section1.seriesID;
    memcpy(&data->info.unknown, &raw->section1.unknown, sizeof(raw->section1.unknown));
    memcpy(&data->info.mii, &raw->section2.mii, sizeof(raw->section2.mii));
    data->info.titleID = raw->section2.titleID;
    data->info.applicationAreaWrites = raw->section2.applicationAreaWrites;
    memset(data->info.reserved, 0, sizeof(data->info.reserved));
    memcpy(data->info.reserved, raw->section2.reserved, sizeof(raw->section2.reserved));
    memset(data->appData.data, 0, sizeof(data->appData.data));
    data->appData.size = 0xd8;
    memcpy(data->appData.data, raw->applicationData, data->appData.size);
    data->formatVersion = raw->section1.formatVersion;

    return 0;
}

int NTAGEncrypt(NTAGRawDataT2T* raw, NTAGDataT2T* data)
{
#if 0 // not doing this anymore since NTAGConvertT2T does no error handling and also corrupts data sometimes?
    // To encrypt we can simply call NTAGConvertT2T
    NTAGDataT2T tmp;
    memset(&tmp, 0, sizeof(tmp));

    int res = NTAGConvertT2T(&tmp, data);
    if (res != 0) {
        DEBUG_FUNCTION_LINE("NTAGConvertT2T failed");
        return res;
    }

    if (tmp.section1.version != 2) {
        DEBUG_FUNCTION_LINE("Invalid format version");
        return -1;
    }

    memcpy(raw, &data->rawData, sizeof(NTAGRawDataT2T));

    // Copy over the individual modified sections
    memcpy(&raw->section0, &tmp.rawData.section0, sizeof(raw->section0));
    memcpy(&raw->section1, &tmp.rawData.section1, sizeof(raw->section1));
    memcpy(&raw->section2, &tmp.rawData.section2, sizeof(raw->section2));
    memcpy(&raw->applicationData, &tmp.rawData.applicationData, sizeof(raw->applicationData));
#endif

    // Convert
    memcpy(raw, &data->raw.data, sizeof(data->raw.data));
    raw->section0.crcCounter = data->info.crcCounter;
    raw->section0.magic = data->info.magic;
    raw->section0.writes = data->info.writes;
    raw->section0.country = data->info.country;
    raw->section0.flags = (data->info.fontRegion & 0xf) | (data->info.flags << 4);
    raw->section0.setupDate = data->info.setupDate;
    raw->section0.figureVersion = data->info.figureVersion;
    raw->section0.lastWriteDate = data->info.lastWriteDate;
    memcpy(&raw->section0.crc, &data->info.crc, sizeof(data->info.crc));
    raw->section2.accessID = data->info.accessID;
    memcpy(raw->section0.name, data->info.name, sizeof(data->info.name));
    memcpy(raw->section1.tagHmac, data->raw.data.section1.tagHmac, sizeof(data->raw.data.section1.tagHmac));
    memcpy(raw->section1.characterID, data->info.characterID, sizeof(data->info.characterID));
    raw->section1.numberingID = data->info.numberingID;
    raw->section1.seriesID = data->info.seriesID;
    raw->section1.figureType = data->info.figureType;
    raw->section1.formatVersion = data->formatVersion;
    memcpy(&raw->section1.unknown, &data->info.unknown, sizeof(data->info.unknown));
    memcpy(raw->section1.keygenSalt, data->raw.data.section1.keygenSalt, sizeof(data->raw.data.section1.keygenSalt));
    memcpy(raw->section1.dataHmac, data->raw.data.section1.dataHmac, sizeof(data->raw.data.section1.dataHmac));
    memcpy(&raw->section2.mii, &data->info.mii, sizeof(data->info.mii));
    raw->section2.titleID = data->info.titleID;
    raw->section2.applicationAreaWrites = data->info.applicationAreaWrites;
    memcpy(raw->section2.reserved, data->info.reserved, sizeof(raw->section2.reserved));
    memcpy(raw, &data->raw.data, 0x10);
    if (data->appData.size > 0xd8) {
        return -1;
    }
    memcpy(raw->applicationData, data->appData.data, data->appData.size);

    // Encrypt
    int res = encryptGameData(raw);
    if (res != 0) {
        DEBUG_FUNCTION_LINE("Failed to encrypt data");
        return res;
    }

    // Copy back ntag uid and lock bytes from original rawData
    //memcpy(raw, &data->rawData, 0x10);
    //memcpy(raw->dynamicLock, data->rawData.dynamicLock, 0x14);

    return 0;
}
