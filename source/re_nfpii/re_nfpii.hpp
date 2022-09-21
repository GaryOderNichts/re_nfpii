#pragma once

#include <nn/nfp.h>
#include <ntag.h>

#include "TagManager.hpp"

#include "debug/logger.h"

namespace re::nfpii {
using nn::Result;
using namespace nn::nfp;

extern TagManager tagManager;

// creates a result directly from the value (should not use this unless lazy, or unsure)
#define RESULT(x)                       Result(NNResult{(int32_t)x})

#define NFP_SUCCESS                     Result(Result::LEVEL_SUCCESS, Result::RESULT_MODULE_COMMON, 0) // 0x0
#define NFP_FATAL_RESULT(desc)          Result(Result::LEVEL_FATAL, Result::RESULT_MODULE_NN_NFP, desc)
#define NFP_STATUS_RESULT(desc)         Result(Result::LEVEL_STATUS, Result::RESULT_MODULE_NN_NFP, desc)
#define NFP_USAGE_RESULT(desc)          Result(Result::LEVEL_USAGE, Result::RESULT_MODULE_NN_NFP, desc)

#define NFP_OUT_OF_RANGE                NFP_USAGE_RESULT(RESULT_OUT_OF_RANGE) // 0xc1b03700
#define NFP_INVALID_PARAM               NFP_USAGE_RESULT(RESULT_INVALID_PARAM) // 0xc1b03780
#define NFP_INVALID_ALIGNMENT           NFP_USAGE_RESULT(RESULT_INVALID_ALIGNMENT) // 0xc1b03800
#define NFP_INVALID_STATE               NFP_STATUS_RESULT(RESULT_INVALID_STATE) // 0xa1b06400
#define NFP_INVALID_TAG                 NFP_STATUS_RESULT(RESULT_INVALID_TAG) // 0xa1b0c800
#define NFP_INVALID_TAG_INFO            NFP_STATUS_RESULT(RESULT_INVALID_TAG_INFO) // 0xa1b0ca80
#define NFP_NO_BACKUPENTRY              NFP_STATUS_RESULT(RESULT_NO_BACKUPENTRY) // 0xa1b0e580
#define NFP_NO_REGISTER_INFO            NFP_STATUS_RESULT(RESULT_NO_REGISTER_INFO) // 0xa1b10900
#define NFP_APP_AREA_MISING             NFP_STATUS_RESULT(RESULT_APP_AREA_MISSING) // 0xa1b10400
#define NFP_APP_AREA_TAGID_MISMATCH     NFP_STATUS_RESULT(RESULT_APP_AREA_TAGID_MISMATCH) // 0xa1b11d00
#define NFP_APP_AREA_ALREADY_EXISTS     NFP_STATUS_RESULT(RESULT_APP_AREA_ALREADY_EXISTS) // 0xa1b10e00
#define NFP_APP_AREA_ACCESS_ID_MISMATCH NFP_STATUS_RESULT(RESULT_APP_AREA_ACCESS_ID_MISMATCH)  // 0xa1b11300
#define NFP_NO_BACKUP_SAVEDATA          NFP_STATUS_RESULT(RESULT_NO_BACKUP_SAVEDATA) // 0xa1b38880
#define NFP_SYSTEM_ERROR                NFP_STATUS_RESULT(RESULT_SYSTEM_ERROR) // 0xa1b3e880
#define NFP_FATAL                       NFP_FATAL_RESULT(RESULT_FATAL) // 0xe1b5db00

} // namespace re::nfpii
