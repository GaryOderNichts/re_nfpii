#include "Tag.hpp"
#include "Utils.hpp"
#include "re_nfpii.hpp"
#include "ntag_crypt.h"
#include "debug/logger.h"
#include "utils/FSUtils.hpp"

#include <cstring>
#include <coreinit/title.h>

namespace re::nfpii {

Tag::Tag()
{
    numAppAreas = 0;
    memset(appAreaInfo, 0, sizeof(appAreaInfo));
    dataBufferCapacity = 0;
    updateTitleId = false;
    updateWriteCount = false;
    memset(&ntagData, 0, sizeof(ntagData));
}

Tag::~Tag()
{
}

Result Tag::SetData(NTAGData* data)
{
    memmove(&ntagData, data, sizeof(NTAGData));
    return NFP_SUCCESS;
}

NTAGData* Tag::GetData()
{
    return &ntagData;
}

Result Tag::Mount(bool backup)
{
    if (backup) {
        // We currently don't support writing the backup data
        // Probably unnecessary anyways?
    }

    if (InitializeDataBuffer(&ntagData).IsFailure()) {
        return RESULT(0xa1b0c880);
    }

    numAppAreas = 1;
    dataBufferCapacity = ntagData.appData.size;
    UpdateAppAreaInfo(false);

    updateTitleId = false;
    updateWriteCount = false;

    return NFP_SUCCESS;
}

Result Tag::Mount()
{
    return Mount(true);
}

Result Tag::Unmount()
{
    numAppAreas = 0;
    return NFP_SUCCESS;
}

Result Tag::GetAppAreaInfo(AppAreaInfo* info, uint32_t* outNumAreas)
{
    *outNumAreas = numAppAreas;
    memcpy(info, appAreaInfo, numAppAreas * sizeof(AppAreaInfo));
    return NFP_SUCCESS;
}

Result Tag::GetAppAreaInfo(AppAreaInfo* info, uint32_t* outNumAreas, int32_t maxAreas)
{
    // nfp implements this differently with another AppAreaInfo struct
    // I don't see the point in doing so though

    if (!info || !outNumAreas) {
        return NFP_INVALID_PARAM;
    }

    *outNumAreas = numAppAreas;

    if ((int32_t) numAppAreas > maxAreas || numAppAreas > 0x10) {
        return NFP_INVALID_PARAM;
    }

    memcpy(info, appAreaInfo, numAppAreas * sizeof(AppAreaInfo));
    return NFP_SUCCESS;
}

Result Tag::Write(TagInfo* info, bool update)
{
    memcpy(ntagData.appData.data, dataBuffer + 0x130, 0xd8);

    // Backup values we might restore if writing to tag fails
    uint64_t currentTitleId = ntagData.info.titleId;
    uint16_t currentCryptCount = ntagData.info.cryptCount;
    uint16_t currentWriteCount = ntagData.info.writeCount;
    uint16_t currentAppDataUpdateCount = ntagData.info.appDataUpdateCount;

    if (update) {
        ntagData.info.cryptCount = IncreaseCount(ntagData.info.cryptCount, true);

        if (updateWriteCount) {
            ntagData.info.writeCount = IncreaseCount(ntagData.info.writeCount, false);
        }

        if (updateTitleId) {
            ntagData.info.titleId = OSGetTitleID();
        }

        if (!CheckUuidCRC(&ntagData.info)) {
            ntagData.info.appDataUpdateCount = IncreaseCount(ntagData.info.appDataUpdateCount, false);
            SetUuidCRC(&ntagData.info.crc);
        }

        ntagData.info.lastWriteDate = OSTimeToAmiiboTime(OSGetTime());
    }

    Result res = WriteTag(true);
    if (res.IsFailure()) {
        // Restore values if writing failed
        ntagData.info.appDataUpdateCount = currentAppDataUpdateCount;
        ntagData.info.cryptCount = currentCryptCount;
        ntagData.info.writeCount = currentWriteCount;
        ntagData.info.titleId = currentTitleId;
        return res;
    }

    return NFP_SUCCESS;
}

Result Tag::InitializeDataBuffer(const NTAGData* data)
{
    memset(dataBuffer, 0, sizeof(dataBuffer));
    memcpy(dataBuffer + 0x130, data->appData.data, sizeof(data->appData.data));
    return NFP_SUCCESS;
}

Result Tag::WriteDataBuffer(const void* data, int32_t offset, uint32_t size)
{
    if (size < 0 || size > dataBufferCapacity) {
        return NFP_INVALID_PARAM;
    }

    memcpy(dataBuffer + offset, data, size);

    updateWriteCount = true;
    updateTitleId = true;

    return NFP_SUCCESS;
}

Result Tag::ReadDataBuffer(void* out, int32_t offset, uint32_t size)
{
    if (size < 0 || size > dataBufferCapacity) {
        return NFP_INVALID_PARAM;
    }

    memcpy(out, dataBuffer + offset, size);

    return NFP_SUCCESS;
}

void Tag::ClearTagData()
{
    memset(&ntagData, 0, sizeof(ntagData));
}

Result Tag::CreateApplicationArea(NTAGData* data, ApplicationAreaCreateInfo const& createInfo)
{
    TagInfo tagInfo;
    ReadTagInfo(&tagInfo, data);
    if (tagInfo.tag_type != 2) {
        return RESULT(0xa1b0c800);
    }

    if (!createInfo.data) {
        return NFP_INVALID_PARAM;
    }

    if (createInfo.size > ntagData.appData.size) {
        return NFP_OUT_OF_RANGE;
    }

    if (!CheckZero(createInfo.reserved, sizeof(createInfo.reserved))) {
        return NFP_OUT_OF_RANGE;
    }

    // Backup current working buffer in case anything fails, to not modify state
    NTAGData backupData;
    memcpy(&backupData, &ntagData, sizeof(NTAGData));

    // Move data into working buffer
    memmove(&ntagData, data, sizeof(NTAGData));

    updateTitleId = true;
    ntagData.info.appId = createInfo.id;

    memset(tagInfo.reserved1, 0, sizeof(tagInfo.reserved1));

    uint8_t applicationData[0xd8];
    memcpy(applicationData, createInfo.data, createInfo.size);
    
    // Rest will be padded with random bytes
    if (createInfo.size < ntagData.appData.size) {
        GetRandom(applicationData + createInfo.size, ntagData.appData.size - createInfo.size);
    }

    // Write the application data to the data buffer
    WriteDataBuffer(applicationData, 0x130, ntagData.appData.size);

    // Set the "has application area" flag
    ntagData.info.flags_hi |= TAG_HAS_APPLICATION_DATA;

    // Write the data to the tag
    Result res = Write(&tagInfo, true);
    if (res.IsFailure()) {
        // Restore the backed up state, if writing failed
        memcpy(&ntagData, &backupData, sizeof(NTAGData));
        return res;
    }

    // Re-mount the tag
    Mount();

    return NFP_SUCCESS;
}

Result Tag::SetRegisterInfo(RegisterInfoSet const& info)
{
    if (!CheckZero(info.reserved, sizeof(info.reserved))) {
        // TODO this rarely happens in smash?
        DumpHex(info.reserved, sizeof(info.reserved));
        return NFP_OUT_OF_RANGE;
    }

    ntagData.info.zero = 0;
    ReadCountryRegion(&ntagData.info.countryCode);
    memcpy(&ntagData.info.mii, &info.mii, sizeof(info.mii));
    UpdateMii(&ntagData.info.mii);
    memcpy(ntagData.info.name, info.amiibo_name, sizeof(ntagData.info.name));
    ntagData.info.flags_lo = info.flags;

    // If we don't have any register info yet, write setup date
    if (!HasRegisterInfo()) {
        ntagData.info.setupDate = OSTimeToAmiiboTime(OSGetTime());
        updateWriteCount = true;
    }

    // Set "has register info" bit
    ntagData.info.flags_hi |= TAG_HAS_REGISTER_INFO;

    return NFP_SUCCESS;
}

Result Tag::DeleteApplicationArea()
{
    TagInfo tagInfo;
    ReadTagInfo(&tagInfo, &ntagData);
    if (tagInfo.tag_type != 2) {
        return RESULT(0xa1b0c800);
    }

    // Backup current working buffer in case anything fails, to not modify state
    NTAGData backupData;
    memcpy(&backupData, &ntagData, sizeof(NTAGData));

    // Delete app area and increase encryption count
    ClearApplicationArea(&ntagData);
    ntagData.info.cryptCount = IncreaseCount(ntagData.info.cryptCount, true);

    // Write the data to the tag
    Result res = Write(&tagInfo, true);
    if (res.IsFailure()) {
        // Restore the backed up state, if writing failed
        memcpy(&ntagData, &backupData, sizeof(NTAGData));
        return res;
    }

    // Re-mount the tag
    Mount();

    return NFP_SUCCESS;
}

Result Tag::DeleteRegisterInfo()
{
    TagInfo tagInfo;
    ReadTagInfo(&tagInfo, &ntagData);
    if (tagInfo.tag_type != 2) {
        return RESULT(0xa1b0c800);
    }

    // Backup current working buffer in case anything fails, to not modify state
    NTAGData backupData;
    memcpy(&backupData, &ntagData, sizeof(NTAGData));

    // Delete register info and increase encryption count
    ClearRegisterInfo(&ntagData);
    ntagData.info.cryptCount = IncreaseCount(ntagData.info.cryptCount, true);

    // Write the data to the tag
    Result res = Write(&tagInfo, true);
    if (res.IsFailure()) {
        // Restore the backed up state, if writing failed
        memcpy(&ntagData, &backupData, sizeof(NTAGData));
        return res;
    }

    // Re-mount the tag
    Mount();

    return NFP_SUCCESS;
}

Result Tag::Format(NTAGData* data, const void* appData, int32_t appDataSize)
{
    if ((appDataSize > 1 && !appData) || appDataSize > 0xd8) {
        return NFP_INVALID_PARAM;
    }

    TagInfo tagInfo;
    ReadTagInfo(&tagInfo, data);
    if (tagInfo.tag_type != 2) {
        return RESULT(0xa1b0c800);
    }

    // Move data into working buffer
    memmove(&ntagData, data, sizeof(NTAGData));

    // Clear and randomize all data
    ntagData.info.magic = 0xa5;
    ntagData.info.flags_hi = 0;
    ntagData.info.zero = 0;
    ntagData.info.countryCode = 0;
    ntagData.info.cryptCount = rand() | 0x8000;
    ntagData.info.writeCount = 0;
    ntagData.info.appDataUpdateCount = 0;
    ntagData.info.flags_lo = 0;
    ntagData.info.setupDate = rand();
    ntagData.info.lastWriteDate = rand();
    GetRandom(&ntagData.info.appId, sizeof(ntagData.info.appId));
    GetRandom(&ntagData.info.titleId, sizeof(ntagData.info.titleId));
    GetRandom(&ntagData.info.crc, sizeof(ntagData.info.crc));
    GetRandom(&ntagData.info.name, sizeof(ntagData.info.name));
    GetRandom(&ntagData.info.mii, sizeof(ntagData.info.mii));
    if (appDataSize > 0) {
        memcpy(ntagData.appData.data, appData, appDataSize);
    }
    if (0xd8 - appDataSize > 0) {
        GetRandom(ntagData.appData.data + appDataSize, 0xd8 - appDataSize);
    }
    ntagData.appData.size = 0xd8;

    // nfp also updates some backup state here, but we don't emulate that yet
    return WriteTag(false);
}

bool Tag::HasRegisterInfo()
{
    return ntagData.info.flags_hi & TAG_HAS_REGISTER_INFO;
}

bool Tag::IsExistApplicationArea()
{
    return ntagData.info.flags_hi & TAG_HAS_APPLICATION_DATA;
}

Result Tag::UpdateAppAreaInfo(bool unk)
{
    if (unk) {
        return NFP_INVALID_PARAM;
    }

    appAreaInfo[0].offset = 0x130;
    appAreaInfo[0].size = ntagData.appData.size;
    appAreaInfo[0].id = ntagData.info.appId;

    return NFP_SUCCESS;
}

Result Tag::WriteTag(bool backup)
{
    // nfp usually writes to the tag using NTAG here
    // this code is mostly custom and writes the data to the SD instead

    NTAGRawData raw;
    if (NTAGEncrypt(&raw, GetData()) != 0) {
        return NFP_STATUS_RESULT(0x12345);
    }

    int res = FSUtils::WriteToFile(path.c_str(), &raw, sizeof(raw));
    if (res != sizeof(raw)) {
        DEBUG_FUNCTION_LINE("Failed to write tag data to %s: %x", path.c_str(), res);
        return NFP_STATUS_RESULT(0x12345);
    }

    // copy the new encrypted raw data to the raw part
    memcpy(&ntagData.rawData, &raw, sizeof(raw));

    return NFP_SUCCESS;
}

} // namespace re::nfpii
