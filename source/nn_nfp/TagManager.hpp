#pragma once
#include "Tag.hpp"
#include "TagStream.hpp"
#include "TagStreamImpl.hpp"

#include <string>
#include <coreinit/mutex.h>
#include <coreinit/event.h>

namespace nn::nfp {

class TagManager {
public:
    TagManager();
    virtual ~TagManager();

    bool IsInitialized();

    Result Initialize();
    Result Finalize();

    Result GetNfpState(NfpState& state);

    Result SetActivateEvent(OSEvent* event);
    Result SetDeactivateEvent(OSEvent* event);

    Result StartDetection();
    Result StopDetection();

    Result Mount();
    Result MountReadOnly();
    Result MountRom();
    Result Unmount();

    Result Flush();
    Result Restore();
    Result Format(const void* data, uint32_t size);

    bool IsExistApplicationArea();
    Result CreateApplicationArea(ApplicationAreaCreateInfo const& createInfo);
    Result WriteApplicationArea(const void* data, uint32_t size, const TagId* tagId);
    Result OpenApplicationArea(uint32_t id);
    Result ReadApplicationArea(void* outData, uint32_t size);
    Result DeleteApplicationArea();

    Result GetTagInfo(TagInfo* outTagInfo);
    Result GetNfpCommonInfo(CommonInfo* outCommonInfo);
    Result GetNfpRegisterInfo(RegisterInfo* outRegisterInfo);
    Result DeleteNfpRegisterInfo();
    Result GetNfpReadOnlyInfo(ReadOnlyInfo* outReadOnlyInfo);
    Result GetNfpRomInfo(RomInfo* outRomInfo);
    Result GetNfpAdminInfo(AdminInfo* outAdminInfo);
    Result SetNfpRegisterInfo(RegisterInfoSet const& info);

    Result OpenStream(uint32_t id);
    Result CloseStream();
    Result ReadStream(void* data, uint32_t size);
    Result WriteStream(const void* data, uint32_t size);

private:
    void Reset();

    bool UpdateInternal();
    void SetNfpState(NfpState state);

    void Activate();
    void Deactivate();

    Result VerifyTagInfo();
    Result GetTagInfo(TagInfo* outTagInfo, uint8_t index);

    Result MountTag();

    Result GetAppAreaInfo(Tag::AppAreaInfo* outInfo, uint32_t* outNumAreas, int32_t maxAreas);

    bool CheckRegisterInfo();

public: // custom
    enum EmulationState {
        EMULATION_OFF,
        EMULATION_ON
    };

    enum UUIDRandomizationState {
        RANDOMIZATION_OFF,
        RANDOMIZATION_ONCE,
        RANDOMIZATION_EVERY_READ
    };

    void SetEmulationState(EmulationState state) {
        emulationState = state;
        pendingRemove = state == EMULATION_OFF;
    }

    EmulationState GetEmulationState() const {
        return emulationState;
    }

    void SetUUIDRandomizationState(UUIDRandomizationState state) {
        uuidRandomizationState = state;
    }

    UUIDRandomizationState GetUUIDRandomizationState() const {
        return uuidRandomizationState;
    }

    void SetTagEmulationPath(std::string path) {
        tagEmulationPath = path;
    }

    std::string const& GetTagEmulationPath() const {
        return tagEmulationPath;
    }

    void SetRemoveAfterSeconds(float secs) {
        this->removeAfterSeconds = secs;
    }

    Result LoadTag();
    void HandleTagUpdates();

    void NotifyNFCGetTagInfo();

private:
    // +0x0
    OSMutex mutex;

    // +0x15c
    OSEvent* activateEvent;
    OSEvent* deactivateEvent;

    // +0x164
    NfpState nfpState;
    // +0x170
    Tag::State tagStates[3];

    // +0x1d0
    bool hasTag;
    uint8_t currentTagIndex;
    // +0x1d6
    bool readOnly;

    // +0x1d8
    TagStreamImpl tagStreamImpl;
    Tag* currentTag;
    Tag tag;
    TagStream tagStream;

private: // custom
    EmulationState emulationState;
    UUIDRandomizationState uuidRandomizationState;
    std::string tagEmulationPath;
    float removeAfterSeconds;
    bool pendingRemove;
    OSTime pendingTagRemoveTime;
};

} // namespace nn::nfp
