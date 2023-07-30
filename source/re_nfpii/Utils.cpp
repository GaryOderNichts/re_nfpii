#include "Utils.hpp"
#include "re_nfpii.hpp"

#include <cstdlib>
#include <coreinit/time.h>
#include <coreinit/userconfig.h>
#include <nfc/nfc.h>

namespace re::nfpii {

bool CheckZero(const void* data, uint32_t size)
{
    assert(data);

    for (uint32_t i = 0; i < size; i++) {
        if (((const uint8_t*) data)[i] != 0) {
            return false;
        }
    }

    return true;
}

void GetRandom(void* data, uint32_t size)
{
    assert(data);

    for (uint32_t i = 0; i < size; i++) {
        ((uint8_t*) data)[i] = (uint8_t) rand();
    }
}

uint16_t IncreaseCount(uint16_t count, bool overflow)
{
    if (count == 0xffff) {
        if (overflow) {
            return 0;
        }

        return 0xffff;
    }

    return count + 1;
}

static TagType FindTagType(NFCTechnology technology, NFCProtocol protocol)
{
    static TagType tt_lookup[] = { TagType::Type1Tag, TagType::Type2Tag, TagType::Type3Tag };

    switch (technology) {
    case NFC_TECHNOLOGY_A:
        if (protocol != NFC_PROTOCOL_UNKNOWN && protocol <= NFC_PROTOCOL_T3T) {
            return tt_lookup[protocol - 1];
        }
        break;
    case NFC_TECHNOLOGY_B:
        if (protocol != NFC_PROTOCOL_UNKNOWN && protocol <= NFC_PROTOCOL_T3T) {
            return tt_lookup[protocol - 1];
        }
        break;
    case NFC_TECHNOLOGY_F:
        if (protocol != NFC_PROTOCOL_UNKNOWN && protocol <= NFC_PROTOCOL_T3T) {
            return tt_lookup[protocol - 1];
        }
        break;
    case NFC_TECHNOLOGY_ISO15693:
        if (protocol == NFC_PROTOCOL_15693) {
            return TagType::Iso15693;
        }
        break;
    }

    return TagType::Unknown;
}

void ReadTagInfo(TagInfo* info, const NTAGDataT2T* data)
{
    assert(info);
    
    memcpy(info->id.uid, data->tagInfo.uid, data->tagInfo.uidSize);
    info->id.size = data->tagInfo.uidSize;
    info->technology = data->tagInfo.technology;
    info->tag_type = FindTagType(data->tagInfo.technology, data->tagInfo.protocol);   
}

void ReadCommonInfo(CommonInfo* info, const NTAGDataT2T* data)
{
    assert(info);

    if (!(data->info.flags & (uint8_t) AdminFlags::IsRegistered) && !(data->info.flags & (uint8_t) AdminFlags::HasApplicationData)) {
        ConvertAmiiboDate(&info->lastWriteDate, 0);
        info->writes = data->info.writes;
        memcpy(&info->characterID, data->info.characterID, 3);
        info->numberingID = data->info.numberingID;
        info->figureType = data->info.figureType;
        info->seriesID = data->info.seriesID;
        info->applicationAreaSize = data->appData.size;
        info->figureVersion = data->info.figureVersion;
        memset(info->reserved, 0, sizeof(info->reserved));
    } else {
        ConvertAmiiboDate(&info->lastWriteDate, data->info.lastWriteDate);
        info->writes = data->info.writes;
        memcpy(&info->characterID, data->info.characterID, 3);
        info->numberingID = data->info.numberingID;
        info->figureType = data->info.figureType;
        info->seriesID = data->info.seriesID;
        info->applicationAreaSize = data->appData.size;
        info->figureVersion = data->info.figureVersion;
        memset(info->reserved, 0, sizeof(info->reserved));
    }
}

void ReadRegisterInfo(RegisterInfo* info, const NTAGDataT2T* data)
{
    assert(info);

    memcpy(&info->mii, &data->info.mii, sizeof(info->mii));
    memcpy(info->name, data->info.name, sizeof(data->info.name));
    // null-terminate name
    info->name[10] = 0;
    ConvertAmiiboDate(&info->registerDate, data->info.setupDate);
    info->fontRegion = data->info.fontRegion;
    info->country = data->info.country;
    memset(info->reserved, 0, sizeof(info->reserved));
}

void ReadReadOnlyInfo(ReadOnlyInfo* info, const NTAGDataT2T* data)
{
    assert(info);

    memcpy(&info->characterID, data->info.characterID, 3);
    info->numberingID = data->info.numberingID;
    info->figureType = data->info.figureType;
    info->seriesID = data->info.seriesID;
    memset(info->reserved, 0, sizeof(info->reserved));
}

static uint8_t GetPlatform(uint64_t titleId) {
    static uint8_t lookup[] = { 0, 1, 0 };

    // This checks the first part of the low title id, I assume 0 = 3ds, 1 = U?
    // So this returns 1 if this is a Wii U title id?
    if ((titleId >> 0x1c & 0xf) < 3) {
        return lookup[titleId >> 0x1c & 0xf];
    }

    return 0xff;
}

void ReadAdminInfo(AdminInfo* info, const NTAGDataT2T* data)
{
    assert(info);

    if (!(data->info.flags & (uint8_t) AdminFlags::HasApplicationData)) {
        info->flags = (AdminFlags) data->info.flags;
        info->titleID = 0;
        info->platform = 0xff;
        info->accessID = 0;
        info->applicationAreaWrites = data->info.applicationAreaWrites;
        info->formatVersion = data->formatVersion;
        memset(info->reserved, 0, sizeof(info->reserved));
    } else {
        info->titleID = data->info.titleID;
        info->accessID = data->info.accessID;
        info->platform = GetPlatform(data->info.titleID);
        info->flags = (AdminFlags) data->info.flags;
        info->applicationAreaWrites = data->info.applicationAreaWrites;
        info->formatVersion = data->formatVersion;
        memset(info->reserved, 0, sizeof(info->reserved));
    }
}

void ClearApplicationArea(NTAGDataT2T* data)
{
    GetRandom(&data->info.accessID, sizeof(data->info.accessID));
    GetRandom(&data->info.titleID, sizeof(data->info.titleID));
    GetRandom(&data->appData.data, sizeof(data->appData.data));
    // clear the "has application area" bit
    data->info.flags &= ~(uint8_t) AdminFlags::HasApplicationData;
}

void ClearRegisterInfo(NTAGDataT2T* data)
{
    data->info.fontRegion = 0;
    data->info.country = 0;
    data->info.setupDate = rand();
    GetRandom(data->info.name, sizeof(data->info.name));
    GetRandom(&data->info.mii, sizeof(data->info.mii));
    // clear the "has register info" bit
    data->info.flags &= ~(uint8_t) AdminFlags::IsRegistered;
}

static bool ReadSysConfig(const char* name, UCDataType type, uint32_t size, void* data)
{
    UCHandle handle = UCOpen();
    if (handle < 0) {
        return false;
    }

    UCSysConfig config;
    strncpy(config.name, name, sizeof(config.name));
    config.access = 0x777;
    config.error = 0;
    config.dataType = type;
    config.dataSize = size;
    config.data = data;
    UCError error = UCReadSysConfig(handle, 1, &config);
    if (error == UC_ERROR_OK) {
        UCClose(handle);
        return true;
    }

    UCClose(handle);
    return false;
}

Result ReadCountryRegion(uint8_t* outCountryCode)
{
    uint32_t cntry_reg;
    if (!ReadSysConfig("cafe.cntry_reg", UC_DATATYPE_UNSIGNED_INT, sizeof(cntry_reg), &cntry_reg)) {
        *outCountryCode = 0xff;
        return NFP_SYSTEM_ERROR;
    }

    if (cntry_reg > 0xff) {
        cntry_reg = 0xff;
    }

    *outCountryCode = (uint8_t) cntry_reg;
    return NFP_SUCCESS;
}

uint16_t OSTimeToAmiiboTime(OSTime time)
{
    OSCalendarTime ct;
    OSTicksToCalendarTime(time, &ct);

    return ((ct.tm_year << 9) - 0xfa000) | ((ct.tm_mon << 5) + 0x20) | (ct.tm_mday);
}

static bool VerifyDate(uint32_t y, uint32_t m, uint32_t d, uint32_t h, uint32_t min, uint32_t s, uint32_t ms)
{
    // N does proper ymd verification, too lazy to implement this right now
    if (h < 24 && min < 60 && s < 60 && ms < 1000) {
        return true;
    }

    return false;
}

void ConvertAmiiboDate(Date* date, uint16_t time)
{
    uint32_t day = time & 0x1f;
    uint32_t month = time >> 0x5 & 0xf;
    uint32_t year = (time >> 0x9 & 0x7f) + 0x7d0;
    if (!VerifyDate(year, month, day, 0, 0, 0, 0)) {
        // TODO what does nfp actually do here?
        date->year = 0;
        date->month = 0;
        date->day = 0;
        return;
    }

    date->day = day;
    date->month = month;
    date->year = year;
}

bool CheckAmiiboMagic(NTAGDataT2T* data)
{
    return data->info.magic == 0xa5;
}

bool CheckUuidCRC(NTAGInfoT2T* info)
{
    // TODO
    return true;
}

void SetUuidCRC(uint32_t* crc)
{
    // TODO
}

static uint16_t crc16(uint16_t poly, const void* data, uint32_t size)
{
    uint16_t crc = 0;
    for (uint32_t byteIndex = 0; byteIndex < size; byteIndex++) {
        for (int bitIndex = 7; bitIndex >= 0; bitIndex--) {
            crc = (((crc << 1) | ((((const uint8_t*) data)[byteIndex] >> bitIndex) & 0x1)) 
                ^ (((crc & 0x8000) != 0) ? poly : 0)); 
        }
    }

    for (int counter = 16; counter > 0; counter--) {
        crc = ((crc << 1) ^ (((crc & 0x8000) != 0) ? poly : 0));
    }

    return crc;
}

Result UpdateMii(FFLStoreData* data)
{
    // Make sure the checksum matches
    if (crc16(0x1021, data, 0x5e) != data->checksum) {
        return NFP_OUT_OF_RANGE;
    }

    // Not sure what this bit is used for
    data->data.core.unk_0x18_b1 = 0;

    // Update checksum
    data->checksum = crc16(0x1021, data, 0x5e);
    return NFP_SUCCESS;
}

} // namespace re::nfpii
