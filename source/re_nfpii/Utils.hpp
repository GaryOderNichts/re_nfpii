#pragma once

#include <nn/nfp.h>
#include <ntag.h>

namespace re::nfpii {
using nn::Result;
using namespace nn::nfp;

bool CheckZero(const void* data, uint32_t size);
void GetRandom(void* data, uint32_t size);
uint16_t IncreaseCount(uint16_t count, bool overflow);

void ReadTagInfo(TagInfo* info, const NTAGData* data);
void ReadCommonInfo(CommonInfo* info, const NTAGData* data);
void ReadRegisterInfo(RegisterInfo* info, const NTAGData* data);
void ReadReadOnlyInfo(ReadOnlyInfo* info, const NTAGData* data);
void ReadAdminInfo(AdminInfo* info, const NTAGData* data);

void ClearApplicationArea(NTAGData* data);
void ClearRegisterInfo(NTAGData* data);

Result ReadCountryRegion(uint8_t* outCountryCode);
uint16_t OSTimeToAmiiboTime(OSTime time);
void ConvertAmiiboDate(Date* date, uint16_t time);
bool CheckAmiiboMagic(NTAGData* data);

bool CheckUuidCRC(NTAGInfo* info);
void SetUuidCRC(uint32_t* crc);

Result UpdateMii(FFLStoreData* data);

} // namespace re::nfpii
