#pragma once
#include "Tag.hpp"
#include "TagStream.hpp"

#include <string>
#include <coreinit/mutex.h>
#include <coreinit/event.h>
#include <coreinit/alarm.h>

#include <nfpii.h>
#include <nfc.h>

namespace re::nfpii {
using nn::Result;
using namespace nn::nfp;

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
    Result Format(const void* data, int32_t size);

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

    static void NfcProcCallback(OSAlarm* alarm, OSContext* context);

public: // custom
    void SetEmulationState(NfpiiEmulationState state)
    {
        emulationState = state;
        pendingRemove = state == EMULATION_OFF;
    }

    NfpiiEmulationState GetEmulationState() const
    {
        return emulationState;
    }

    void SetUUIDRandomizationState(NfpiiUUIDRandomizationState state)
    {
        uuidRandomizationState = state;
    }

    NfpiiUUIDRandomizationState GetUUIDRandomizationState() const
    {
        return uuidRandomizationState;
    }

    void SetTagEmulationPath(std::string path)
    {
        tagEmulationPath = path;
    }

    std::string const& GetTagEmulationPath() const
    {
        return tagEmulationPath;
    }

    void SetRemoveAfterSeconds(float secs)
    {
        this->removeAfterSeconds = secs;
    }

    void SetAmiiboSettings(bool inAmiiboSettings)
    {
        this->inAmiiboSettings = inAmiiboSettings;
    }

    Result LoadTag();
    void HandleTagUpdates();

    NFCError QueueNFCGetTagInfo(NFCTagInfoCallback callback, void* arg);
    void HandleNFCGetTagInfo();

private:
    // +0x0
    OSMutex mutex;

    // +0xe0
    OSAlarm nfcProcAlarm;

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
    TagStream::TagStreamImpl tagStreamImpl;
    Tag* currentTag;
    Tag tag;
    TagStream tagStream;

private: // custom
    NfpiiEmulationState emulationState;
    NfpiiUUIDRandomizationState uuidRandomizationState;
    std::string tagEmulationPath;
    float removeAfterSeconds;
    bool pendingRemove;
    OSTime pendingTagRemoveTime;

    bool pendingTagInfo;
    NFCTagInfoCallback nfcTagInfoCallback;
    void* nfcTagInfoArg;

    bool inAmiiboSettings;
    OSTime amiiboSettingsReattachTimeout;
};

} // namespace re::nfpii
