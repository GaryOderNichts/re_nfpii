#pragma once

#include <nn/nfp.h>

namespace nn::nfp {

// What the actual point of having this many different classes is, is beyond me
// For rw to the tag that's now TagManager -> TagStream -> TagManager -> TagStreamImpl -> Tag
class TagStream {
public:
    TagStream();
    virtual ~TagStream();

    bool IsOpened();
    bool CheckOpened(uint32_t id);

    Result Close();
    Result Open(uint32_t id);

    Result Read(void* data, uint32_t size);
    Result Write(const void* data, uint32_t size);

private:
    // +0x0
    uint32_t appId;

    // +0x64
    bool isOpened;
};

} // namespace nn::nfp
