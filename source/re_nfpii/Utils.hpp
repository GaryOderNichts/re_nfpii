#pragma once

#include <nn/nfp.h>
#include <ntag/ntag.h>

namespace re::nfpii {
using nn::Result;
using namespace nn::nfp;

bool CheckZero(const void* data, uint32_t size);
void GetRandom(void* data, uint32_t size);
uint16_t IncreaseCount(uint16_t count, bool overflow);

void ReadTagInfo(TagInfo* info, const NTAGDataT2T* data);
void ReadCommonInfo(CommonInfo* info, const NTAGDataT2T* data);
void ReadRegisterInfo(RegisterInfo* info, const NTAGDataT2T* data);
void ReadReadOnlyInfo(ReadOnlyInfo* info, const NTAGDataT2T* data);
void ReadAdminInfo(AdminInfo* info, const NTAGDataT2T* data);

void ClearApplicationArea(NTAGDataT2T* data);
void ClearRegisterInfo(NTAGDataT2T* data);

Result ReadCountryRegion(uint8_t* outCountryCode);
uint16_t OSTimeToAmiiboTime(OSTime time);
void ConvertAmiiboDate(Date* date, uint16_t time);
bool CheckAmiiboMagic(NTAGDataT2T* data);

bool CheckUuidCRC(NTAGInfoT2T* info);
void SetUuidCRC(uint32_t* crc);

Result UpdateMii(FFLStoreData* data);

} // namespace re::nfpii
