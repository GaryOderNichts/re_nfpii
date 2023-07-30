#pragma once

#include <ntag/ntag.h>
#include <nn/nfp.h>

#include <string>

namespace re::nfpii {
using nn::Result;
using namespace nn::nfp;

class Tag {
public:
    Tag();
    virtual ~Tag();

    struct State {
        // +0x0
        int32_t state;
        int32_t result;
        Tag* tag;    
    };

    struct AppAreaInfo {
        // +0x0
        uint32_t id;
        // +0x8
        uint32_t offset;
        uint32_t size;
    };

    Result SetData(NTAGDataT2T* data);
    NTAGDataT2T* GetData();

    Result Mount(bool backup);
    Result Mount();

    Result Unmount();

    Result GetAppAreaInfo(AppAreaInfo* info, uint32_t* outNumAreas);
    Result GetAppAreaInfo(AppAreaInfo* info, uint32_t* outNumAreas, int32_t maxAreas);

    Result Write(TagInfo* info, bool update);

    Result InitializeDataBuffer(const NTAGDataT2T* data);
    Result WriteDataBuffer(const void* data, int32_t offset, uint32_t size);
    Result ReadDataBuffer(void* out, int32_t offset, uint32_t size);

    void ClearTagData();

    Result CreateApplicationArea(NTAGDataT2T* data, ApplicationAreaCreateInfo const& createInfo);
    Result SetRegisterInfo(RegisterInfoSet const& info);

    Result DeleteApplicationArea();
    Result DeleteRegisterInfo();
    Result Format(NTAGDataT2T* data, const void* appData, int32_t appDataSize);
    
    bool HasRegisterInfo();
    bool IsExistApplicationArea();

    Result UpdateAppAreaInfo(bool unk);

    Result WriteTag(bool backup);

public: // custom
    void SetPath(std::string& path) {
        this->path = path;
    }

private:
    // +0x4
    uint8_t dataBuffer[0x800];
    NTAGDataT2T ntagData;
    uint32_t numAppAreas;

    // +0xdec
    AppAreaInfo appAreaInfo[16];
    uint32_t dataBufferCapacity;

    // +0xf58
    bool updateAppWriteCount;
    bool updateTitleId;

private: // custom
    std::string path;
};

} // namespace re::nfpii
