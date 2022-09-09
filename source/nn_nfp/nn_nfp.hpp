#pragma once

#include <coreinit/mutex.h>
#include <nn/nfp.h>
#include <ntag.h>

#include "TagManager.hpp"

#include "debug/logger.h"

namespace nn::nfp {

extern TagManager tagManager;

// creates a result directly from the value (should not use this unless lazy, or unsure)
#define RESULT(x)                       Result(NNResult{(int32_t)x})

#define NFP_SUCCESS                     Result(Result::LEVEL_SUCCESS, Result::RESULT_MODULE_COMMON, 0) // 0x0
#define NFP_STATUS_RESULT(desc)         Result(Result::LEVEL_STATUS, Result::RESULT_MODULE_NN_NFP, desc)
#define NFP_USAGE_RESULT(desc)          Result(Result::LEVEL_USAGE, Result::RESULT_MODULE_NN_NFP, desc)

#define NFP_INVALID_STATE               NFP_STATUS_RESULT(INVALID_STATE) // 0xa1b06400
#define NFP_NO_REGISTER_INFO            NFP_STATUS_RESULT(NO_REGISTER_INFO) // 0xa1b10900
#define NFP_APP_AREA_TAGID_MISMATCH     NFP_STATUS_RESULT(APP_AREA_TAGID_MISMATCH) // 0xa1b11d00
#define NFP_NO_BACKUPENTRY              NFP_STATUS_RESULT(NO_BACKUPENTRY) // 0xa1b0e580
#define NFP_NO_BACKUP_SAVEDATA          NFP_STATUS_RESULT(NO_BACKUP_SAVEDATA) // 0xa1b38880
#define NFP_APP_AREA_ALREADY_EXISTS     NFP_STATUS_RESULT(APP_AREA_ALREADY_EXISTS) // 0xa1b10e00
#define NFP_APP_AREA_ID_MISMATCH        NFP_STATUS_RESULT(APP_AREA_ID_MISMATCH)  // 0xa1b11300
#define NFP_APP_AREA_MISING             NFP_STATUS_RESULT(0x10400) // 0xa1b10400
#define NFP_INVALID_TAG                 NFP_STATUS_RESULT(0x0ca80) // 0xa1b0ca80
#define NFP_INVALID_PARAM               NFP_USAGE_RESULT(INVALID_PARAM) // 0xc1b03780
#define NFP_INVALID_ALIGNMENT           NFP_USAGE_RESULT(INVALID_ALIGNMENT) // 0xc1b03800
#define NFP_OUT_OF_RANGE                NFP_USAGE_RESULT(0x03700) // 0xc1b03700

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

Result SetCountryRegion(uint8_t* outCountryCode);
uint16_t OSTimeToAmiiboTime(OSTime time);
void ConvertAmiiboDate(Date* date, uint16_t time);
bool CheckAmiiboMagic(NTAGData* data);

bool CheckUuidCRC(NTAGInfo* info);
void SetUuidCRC(uint32_t* crc);

Result UpdateMii(FFLStoreData* data);

} // namespace nn::nfp
