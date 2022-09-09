#pragma once

#include "Tag.hpp"

namespace nn::nfp {

class TagStreamImpl {
public:
    TagStreamImpl();
    virtual ~TagStreamImpl();

    Result Open(uint32_t id);
    Result Close();

    Result Read(void* data, int32_t size);
    Result Write(const void* data, uint32_t size);

    Result Clear();

public:
    // +0x0
    Tag* tag;
    Tag::AppAreaInfo info;
    bool isOpened;
};

} // namespace nn::nfp
