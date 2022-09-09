#pragma once

#include <ntag.h>
#include <nn/nfp.h>

#include <string>

namespace nn::nfp {

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

    Result SetData(NTAGData* data);
    NTAGData* GetData();

    Result Mount(bool backup);
    Result Mount();

    Result Unmount();

    Result GetAppAreaInfo(AppAreaInfo* info, uint32_t* outNumAreas);
    Result GetAppAreaInfo(AppAreaInfo* info, uint32_t* outNumAreas, int32_t maxAreas);

    Result Write(TagInfo* info, bool update);

    Result InitializeDataBuffer(const NTAGData* data);
    Result WriteDataBuffer(const void* data, int32_t offset, uint32_t size);
    Result ReadDataBuffer(void* out, int32_t offset, uint32_t size);

    void ClearTagData();

    Result CreateApplicationArea(NTAGData* data, ApplicationAreaCreateInfo const& createInfo);
    Result SetRegisterInfo(RegisterInfoSet const& info);

    Result DeleteApplicationArea();
    Result DeleteRegisterInfo();
    Result Format(NTAGData* data, void* appData, int32_t appDataSize);
    
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
    NTAGData ntagData;
    uint32_t numAppAreas;

    // +0xdec
    AppAreaInfo appAreaInfo[16];
    uint32_t dataBufferCapacity;

    // +0xf58
    bool updateWriteCount;
    bool updateTitleId;

private: // custom
    std::string path;
};

} // namespace nn::nfp
