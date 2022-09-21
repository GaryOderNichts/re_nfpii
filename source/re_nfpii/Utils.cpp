#include "Utils.hpp"
#include "re_nfpii.hpp"

#include <cstdlib>
#include <coreinit/time.h>
#include <coreinit/userconfig.h>

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

static uint8_t FindTagType(uint8_t p, uint8_t t)
{
    static uint8_t p0_lookup[] = { 1, 1, 2, 4 };
    static uint8_t p1_lookup[] = { 8, 1, 2, 4 };
    static uint8_t p2_lookup[] = { 16, 1, 2, 4 };

    switch (p) {
    case 0:
        if (t != 0 && t < 4) {
            return p0_lookup[t];
        }
        break;
    case 1:
        if (t != 0 && t < 4) {
            return p1_lookup[t];
        }
        break;
    case 2:
        if (t != 0 && t < 4) {
            return p2_lookup[t];
        }
        break;
    case 6:
    case 0x83:
        return 0x20;
    }

    return 0;
}

void ReadTagInfo(TagInfo* info, const NTAGData* data)
{
    assert(info);
    
    memcpy(info->id.uid, data->uid, data->uidSize);
    info->id.size = data->uidSize;
    info->protocol = data->protocol;
    info->tag_type = FindTagType(data->protocol, data->tagType);   
}

void ReadCommonInfo(CommonInfo* info, const NTAGData* data)
{
    assert(info);

    if (!(data->info.flags_hi & (uint8_t) AdminFlags::IsRegistered) && !(data->info.flags_hi & (uint8_t) AdminFlags::HasApplicationData)) {
        ConvertAmiiboDate(&info->lastWriteDate, 0);
        info->writes = data->info.writeCount;
        memcpy(&info->characterID, data->info.characterInfo, 3);
        info->numberingID = data->info.modelNumber;
        info->figureType = data->info.figureType;
        info->seriesID = data->info.series;
        info->applicationAreaSize = data->appData.size;
        info->figureVersion = data->info.zero;
        memset(info->reserved, 0, sizeof(info->reserved));
    } else {
        ConvertAmiiboDate(&info->lastWriteDate, data->info.lastWriteDate);
        info->writes = data->info.writeCount;
        memcpy(&info->characterID, data->info.characterInfo, 3);
        info->numberingID = data->info.modelNumber;
        info->figureType = data->info.figureType;
        info->seriesID = data->info.series;
        info->applicationAreaSize = data->appData.size;
        info->figureVersion = data->info.zero;
        memset(info->reserved, 0, sizeof(info->reserved));
    }
}

void ReadRegisterInfo(RegisterInfo* info, const NTAGData* data)
{
    assert(info);

    memcpy(&info->mii, &data->info.mii, sizeof(info->mii));
    memcpy(info->name, data->info.name, sizeof(data->info.name));
    // null-terminate name
    info->name[10] = 0;
    ConvertAmiiboDate(&info->registerDate, data->info.setupDate);
    info->flags = data->info.flags_lo;
    info->country = data->info.countryCode;
    memset(info->reserved, 0, sizeof(info->reserved));
}

void ReadReadOnlyInfo(ReadOnlyInfo* info, const NTAGData* data)
{
    assert(info);

    memcpy(&info->characterID, data->info.characterInfo, 3);
    info->numberingID = data->info.modelNumber;
    info->figureType = data->info.figureType;
    info->seriesID = data->info.series;
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

void ReadAdminInfo(AdminInfo* info, const NTAGData* data)
{
    assert(info);

    if (!(data->info.flags_hi & (uint8_t) AdminFlags::HasApplicationData)) {
        info->flags = (AdminFlags) data->info.flags_hi;
        info->titleID = 0;
        info->platform = 0xff;
        info->accessID = 0;
        info->applicationAreaWrites = data->info.appDataUpdateCount;
        info->formatVersion = data->formatVersion;
        memset(info->reserved, 0, sizeof(info->reserved));
    } else {
        info->titleID = data->info.titleId;
        info->accessID = data->info.appId;
        info->platform = GetPlatform(data->info.titleId);
        info->flags = (AdminFlags) data->info.flags_hi;
        info->applicationAreaWrites = data->info.appDataUpdateCount;
        info->formatVersion = data->formatVersion;
        memset(info->reserved, 0, sizeof(info->reserved));
    }
}

void ClearApplicationArea(NTAGData* data)
{
    GetRandom(&data->info.appId, sizeof(data->info.appId));
    GetRandom(&data->info.titleId, sizeof(data->info.titleId));
    GetRandom(&data->appData.data, sizeof(data->appData.data));
    // clear the "has application area" bit
    data->info.flags_hi &= ~(uint8_t) AdminFlags::HasApplicationData;
}

void ClearRegisterInfo(NTAGData* data)
{
    data->info.flags_lo = 0;
    data->info.countryCode = 0;
    data->info.setupDate = rand();
    GetRandom(data->info.name, sizeof(data->info.name));
    GetRandom(&data->info.mii, sizeof(data->info.mii));
    // clear the "has register info" bit
    data->info.flags_hi &= ~(uint8_t) AdminFlags::IsRegistered;
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

bool CheckAmiiboMagic(NTAGData* data)
{
    return data->info.magic == 0xa5;
}

bool CheckUuidCRC(NTAGInfo* info)
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
