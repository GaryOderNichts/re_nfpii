#include "TagStream.hpp"
#include "Utils.hpp"
#include "re_nfpii.hpp"

namespace re::nfpii {

TagStream::TagStream()
{
    appId = 0;
    isOpened = false;
}

TagStream::~TagStream()
{
    Close();
}

bool TagStream::IsOpened()
{
    return isOpened;
}

bool TagStream::CheckOpened(uint32_t id)
{
    if (IsOpened()) {
        // Not sure what nintendo is actually doing here
        // this is much simpler and does the same
        return id == appId;
    }

    return false;
}

Result TagStream::Close()
{
    if (!IsOpened()) {
        return NFP_INVALID_STATE;
    }

    Result res = tagManager.CloseStream();
    if (res.IsFailure()) {
        return res;
    }

    isOpened = false;
    return NFP_SUCCESS;
}

Result TagStream::Open(uint32_t id)
{
    if (IsOpened()) {
        return NFP_INVALID_STATE;
    }

    Result res = tagManager.OpenStream(id);
    if (res.IsFailure()) {
        return res;
    }

    isOpened = true;
    appId = id;

    return res;
}

Result TagStream::Read(void* data, uint32_t size)
{
    if (!IsOpened()) {
        return NFP_INVALID_STATE;
    }

    return tagManager.ReadStream(data, size);
}

Result TagStream::Write(const void* data, uint32_t size)
{
    if (!IsOpened()) {
        return NFP_INVALID_STATE;
    }

    return tagManager.WriteStream(data, size);
}

Result TagStream::TagStreamImpl::Open(uint32_t id)
{
    if (isOpened) {
        return NFP_INVALID_STATE;
    }

    Tag::AppAreaInfo appInfo[16];
    uint32_t numAreas;
    tag->GetAppAreaInfo(appInfo, &numAreas);

    if (numAreas < 1) {
        return NFP_INVALID_TAG;
    }

    for (uint32_t i = 0; i < numAreas; i++) {
        if (appInfo[i].id == id) {
            if (appInfo[i].size != 0xd8) {
                return NFP_INVALID_TAG;
            }

            memcpy(&info, &appInfo[i], sizeof(info));
            isOpened = true;

            return NFP_SUCCESS;
        }
    }

    return NFP_INVALID_TAG;
}

Result TagStream::TagStreamImpl::Close()
{
    if (!isOpened) {
        return NFP_INVALID_STATE;
    }

    memset(&info, 0, sizeof(info));
    isOpened = false;

    return NFP_SUCCESS;
}

Result TagStream::TagStreamImpl::Read(void* data, int32_t size)
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

Result TagStream::TagStreamImpl::Write(const void* data, uint32_t size)
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

Result TagStream::TagStreamImpl::Clear()
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
