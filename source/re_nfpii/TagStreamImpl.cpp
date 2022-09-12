#include "TagStreamImpl.hpp"
#include "re_nfpii.hpp"

#include <cstring>

namespace re::nfpii {

TagStreamImpl::TagStreamImpl()
{
    tag = nullptr;
    memset(&info, 0, sizeof(info));
    isOpened = false;
}

TagStreamImpl::~TagStreamImpl()
{
}

Result TagStreamImpl::Open(uint32_t id)
{
    if (isOpened) {
        return NFP_INVALID_STATE;
    }

    Tag::AppAreaInfo appInfo[16];
    uint32_t numAreas;
    tag->GetAppAreaInfo(appInfo, &numAreas);

    if (numAreas < 1) {
        return RESULT(0xa1b0c800);
    }

    for (uint32_t i = 0; i < numAreas; i++) {
        if (appInfo[i].id == id) {
            if (appInfo[i].size != 0xd8) {
                return RESULT(0xa1b0c800);
            }

            memcpy(&info, &appInfo[i], sizeof(info));
            isOpened = true;

            return NFP_SUCCESS;
        }
    }

    return RESULT(0xa1b0c800);
}

Result TagStreamImpl::Close()
{
    if (!isOpened) {
        return NFP_INVALID_STATE;
    }

    memset(&info, 0, sizeof(info));
    isOpened = false;

    return NFP_SUCCESS;
}

Result TagStreamImpl::Read(void* data, int32_t size)
{
    if (!isOpened) {
        return NFP_INVALID_STATE;
    }

    uint32_t* pSize = &info.size;
    if (size <= (int32_t) info.size) {
        pSize = (uint32_t*) &size;
    }

    return tag->ReadDataBuffer(data, info.offset, *pSize);
}

Result TagStreamImpl::Write(const void* data, uint32_t size)
{
    if (!isOpened) {
        return NFP_INVALID_STATE;
    }

    // nfp checks byte 0x10 of appareainfo here, not sure what that's for

    if (size > info.size) {
        return NFP_OUT_OF_RANGE;
    }

    Result res = tag->WriteDataBuffer(data, info.offset, size);
    if (res.IsSuccess() && size < info.size) {
        uint8_t randomness[0x21c];
        GetRandom(randomness, sizeof(randomness));

        res = tag->WriteDataBuffer(randomness, info.offset + size, info.size - size);
    }

    return res;
}

Result TagStreamImpl::Clear()
{
    if (!isOpened) {
        return NFP_INVALID_STATE;
    }

    // nfp checks byte 0x10 of appareainfo here, not sure what that's for

    uint8_t zeroes[0x21c];
    memset(zeroes, 0, sizeof(zeroes));

    return tag->WriteDataBuffer(zeroes, info.offset, info.size);
}

} // namespace re::nfpii
