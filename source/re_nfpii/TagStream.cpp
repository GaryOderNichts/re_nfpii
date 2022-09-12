#include "TagStream.hpp"
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

} // namespace re::nfpii
