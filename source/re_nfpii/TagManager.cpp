#include "TagManager.hpp"
#include "re_nfpii.hpp"
#include "Lock.hpp"
#include "ntag_crypt.h"
#include "debug/logger.h"

#include <cstring>

namespace re::nfpii {

TagManager::TagManager()
{
    activateEvent = nullptr;
    nfpState = NfpState::Uninitialized;
    deactivateEvent = nullptr;
    memset(tagStates, 0, sizeof(tagStates));
    readOnly = false;

    OSInitMutex(&mutex);

    emulationState = EMULATION_OFF;
    uuidRandomizationState = RANDOMIZATION_OFF;
    tagEmulationPath = "";
    removeAfterSeconds = 0.0f;
    pendingRemove = false;
    pendingTagRemoveTime = 0;
}

TagManager::~TagManager()
{
}

bool TagManager::IsInitialized()
{
    return nfpState != NfpState::Uninitialized;
}

Result TagManager::Initialize()
{
    Lock lock(&mutex);

    if (nfpState != NfpState::Uninitialized) {
        return NFP_INVALID_STATE;
    }

    // Seed random
    srand(OSGetTick());

    // Reset the internal state
    Reset();

    // TODO ACPInitialize

    SetNfpState(NfpState::Initialized);

    return NFP_SUCCESS;
}

Result TagManager::Finalize()
{
    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    StopDetection();

    Reset();

    return NFP_SUCCESS;
}

Result TagManager::GetNfpState(NfpState& state)
{
    if (UpdateInternal()) {
        // Handle custom tag updates here
        // TODO what if an application doesn't call GetNfpState periodically?
        HandleTagUpdates();

        state = nfpState;
    } else {
        state = NfpState::Uninitialized;
    }

    return NFP_SUCCESS;
}

Result TagManager::SetActivateEvent(OSEvent* event)
{
    if (!event) {
        return NFP_INVALID_PARAM;
    }

    if (nfpState != NfpState::Initialized) {
        return NFP_INVALID_STATE;
    }

    OSInitEventEx(event, FALSE, OS_EVENT_MODE_AUTO, (char*) "NfcActivateEvent");
    activateEvent = event;

    return NFP_SUCCESS;
}

Result TagManager::SetDeactivateEvent(OSEvent* event)
{
    if (!event) {
        return NFP_INVALID_PARAM;
    }

    if (nfpState != NfpState::Initialized) {
        return NFP_INVALID_STATE;
    }

    OSInitEventEx(event, FALSE, OS_EVENT_MODE_AUTO, (char*) "NfcDeactivateEvent");
    deactivateEvent = event;

    return NFP_SUCCESS;
}

Result TagManager::StartDetection()
{
    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Initialized && nfpState != NfpState::Removed) {
        return NFP_INVALID_STATE;
    }

    // nfp would start ntag reading here which resets the events
    if (activateEvent) {
        OSResetEvent(activateEvent);
    }
    if (deactivateEvent) {
        OSResetEvent(deactivateEvent);
    }

    SetNfpState(NfpState::Searching);

    // If emulation isn't turned off load the tag
    if (emulationState != EMULATION_OFF) {
        Result res = LoadTag();
        DEBUG_FUNCTION_LINE("LoadTag: %x", ((NNResult) res).value);

        // We still "succeed" on failure but turn off emulation
        if (res.IsFailure()) {
            emulationState = EMULATION_OFF;
        }
    }

    return NFP_SUCCESS;
}

Result TagManager::StopDetection()
{
    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Searching && nfpState != NfpState::Found &&
        nfpState != NfpState::Mounted && nfpState != NfpState::MountedROM &&
        nfpState != NfpState::Removed) {
        return NFP_INVALID_STATE;
    }

    // Unmount the tag if it's currently mounted
    if (nfpState == NfpState::Mounted || nfpState == NfpState::MountedROM) {
        Unmount();
    }

    // Deactivate the tag if we have one
    if (nfpState == NfpState::Found) {
        Deactivate();
    }

    // Clear the current tag if we have one
    if (currentTag) {
        currentTag->ClearTagData();
        currentTag = nullptr;
    }

    SetNfpState(NfpState::Initialized);

    return NFP_SUCCESS;
}


Result TagManager::Mount()
{
    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (currentTagIndex > 2) {
        return NFP_OUT_OF_RANGE;
    }

    if (nfpState != NfpState::Found) {
        return NFP_INVALID_STATE;
    }

    Result res = VerifyTagInfo();
    if (res.IsFailure()) {
        return res;
    }

    // TODO verify tag state

    res = MountTag();
    return res;
}

Result TagManager::MountReadOnly()
{
    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (currentTagIndex > 2) {
        return NFP_OUT_OF_RANGE;
    }

    if (nfpState != NfpState::Found) {
        return NFP_INVALID_STATE;
    }

    Result res = VerifyTagInfo();
    if (res.IsFailure()) {
        return res;
    }

    // TODO verify tag state

    readOnly = true;
    res = MountTag();
    if (res.IsFailure()) {
        readOnly = false;
    }

    return res;
}

Result TagManager::MountRom()
{
    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (currentTagIndex > 2) {
        return NFP_OUT_OF_RANGE;
    }

    if (nfpState != NfpState::Found) {
        return NFP_INVALID_STATE;
    }

    Result res = VerifyTagInfo();
    if (res.IsFailure()) {
        return res;
    }

    // TODO verify tag state

    SetNfpState(NfpState::MountedROM);

    return NFP_SUCCESS;
}

Result TagManager::Unmount()
{
    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Mounted && nfpState != NfpState::MountedROM) {
        return NFP_INVALID_STATE;
    }

    if (tagStream.IsOpened()) {
        tagStream.Close();
    }

    Result res = tag.Unmount();
    if (res.IsSuccess()) {
        SetNfpState(NfpState::Found);
        readOnly = false;
    }

    return res;
}

Result TagManager::Flush()
{
    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Mounted) {
        return NFP_INVALID_STATE;
    }

    if (readOnly) {
        return NFP_INVALID_STATE;
    }

    if (!currentTag) {
        return NFP_INVALID_STATE;
    }

    TagInfo info;
    Result res = GetTagInfo(&info, currentTagIndex);
    if (res.IsFailure()) {
        return res;
    }

    res = currentTag->Write(&info, true);
    if (res.IsFailure()) {
        return res;   
    }

    return NFP_SUCCESS;
}

Result TagManager::Restore()
{
    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Found) {
        return NFP_INVALID_STATE;
    }

    // TODO implement restore

    return NFP_SUCCESS;
}

Result TagManager::Format(const void* data, uint32_t size)
{
    if (!size || !data) {
        return NFP_INVALID_PARAM;
    }

    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Found) {
        return NFP_INVALID_STATE;
    }

    // TODO implement format

    return NFP_SUCCESS;
}

bool TagManager::IsExistApplicationArea()
{
    if (UpdateInternal() && nfpState == NfpState::Mounted) {
        return tag.IsExistApplicationArea();
    }

    return false;
}

Result TagManager::CreateApplicationArea(ApplicationAreaCreateInfo const& createInfo)
{
    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Mounted) {
        return NFP_INVALID_STATE;
    }

    if (readOnly) {
        return NFP_INVALID_STATE;
    }

    Result res = VerifyTagInfo();
    if (res.IsFailure()) {
        return res;
    }

    if (IsExistApplicationArea()) {
        return NFP_APP_AREA_ALREADY_EXISTS;
    }

    return tag.CreateApplicationArea(tag.GetData(), createInfo);
}

Result TagManager::WriteApplicationArea(const void* data, uint32_t size, const TagId* tagId)
{
    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (!size) {
        return NFP_INVALID_PARAM;
    }

    if (nfpState != NfpState::Mounted || readOnly) {
        return NFP_INVALID_STATE;
    }

    NTAGData* ntagData = tagStates[currentTagIndex].tag->GetData();
    if (ntagData->uidSize != tagId->lenght || (memcmp(ntagData->uid, tagId->id, tagId->lenght) != 0)) {
        return NFP_APP_AREA_TAGID_MISMATCH;
    }

    return tagStream.Write(data, size);
}

Result TagManager::OpenApplicationArea(uint32_t id)
{
    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Mounted) {
        return NFP_INVALID_STATE;
    }

    if (!IsExistApplicationArea()) {
        return NFP_APP_AREA_MISING;
    }

    // We don't need to do anything if we have this id already opened
    if (tagStream.CheckOpened(id)) {
        return NFP_SUCCESS;
    }

    // Re-open tag-stream if no opened or id mismatch
    tagStream.Close();

    uint32_t numAreas;
    Tag::AppAreaInfo appAreaInfo;
    Result res = GetAppAreaInfo(&appAreaInfo, &numAreas, 1);
    if (res.IsFailure() || !numAreas) {
        return res;
    }

    if (appAreaInfo.id != id) {
        return NFP_APP_AREA_ID_MISMATCH;
    }

    return tagStream.Open(id);
}

Result TagManager::ReadApplicationArea(void* outData, uint32_t size)
{
    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (!outData) {
        return NFP_INVALID_PARAM;
    }

    if (nfpState != NfpState::Mounted) {
        return NFP_INVALID_STATE;
    }

    return tagStream.Read(outData, size);
}

Result TagManager::DeleteApplicationArea()
{
    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Mounted) {
        return NFP_INVALID_STATE;
    }

    if (readOnly) {
        return NFP_INVALID_STATE;
    }

    Result res = VerifyTagInfo();
    if (res.IsFailure()) {
        return res;
    }

    if (!IsExistApplicationArea()) {
        return NFP_APP_AREA_MISING;
    }

    return tag.DeleteApplicationArea();
}

Result TagManager::GetTagInfo(TagInfo* outTagInfo)
{
    // TODO RANDOMIZATION_EVERY_READ
    return GetTagInfo(outTagInfo, currentTagIndex);
}

Result TagManager::GetNfpCommonInfo(CommonInfo* outCommonInfo)
{
    if (!outCommonInfo) {
        return NFP_INVALID_PARAM;
    }

    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Mounted) {
        return NFP_INVALID_STATE;
    }   

    if (currentTagIndex > 2) {
        return NFP_OUT_OF_RANGE;
    }

    // TODO tag state stuff

    NTAGData* data = tagStates[currentTagIndex].tag->GetData();
    ReadCommonInfo(outCommonInfo, data);

    return NFP_SUCCESS;
}

Result TagManager::GetNfpRegisterInfo(RegisterInfo* outRegisterInfo)
{
    if (!outRegisterInfo) {
        return NFP_INVALID_PARAM;
    }

    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Mounted) {
        return NFP_INVALID_STATE;
    }

    if (currentTagIndex > 2) {
        return NFP_OUT_OF_RANGE;
    }

    // TODO tag state stuff

    if (!CheckRegisterInfo()) {
        return NFP_NO_REGISTER_INFO;
    }

    NTAGData* data = tagStates[currentTagIndex].tag->GetData();
    ReadRegisterInfo(outRegisterInfo, data);

    return NFP_SUCCESS;
}

Result TagManager::DeleteNfpRegisterInfo()
{
    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Mounted) {
        return NFP_INVALID_STATE;
    }

    if (readOnly) {
        return NFP_INVALID_STATE;
    }

    Result res = VerifyTagInfo();
    if (res.IsFailure()) {
        return res;
    }

    if (!CheckRegisterInfo()) {
        return NFP_NO_REGISTER_INFO;
    }

    return tag.DeleteRegisterInfo();
}

Result TagManager::GetNfpReadOnlyInfo(ReadOnlyInfo* outReadOnlyInfo)
{
    if (!outReadOnlyInfo) {
        return NFP_INVALID_PARAM;
    }

    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Found && nfpState != NfpState::Mounted) {
        return NFP_INVALID_STATE;
    }

    if (currentTagIndex > 2) {
        return NFP_OUT_OF_RANGE;
    }

    // TODO tag state stuff

    NTAGData* data = tagStates[currentTagIndex].tag->GetData();
    ReadReadOnlyInfo(outReadOnlyInfo, data);

    return NFP_SUCCESS;
}

Result TagManager::GetNfpRomInfo(RomInfo* outRomInfo)
{
    if (!outRomInfo) {
        return NFP_INVALID_PARAM;
    }

    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Mounted && nfpState != NfpState::MountedROM) {
        return NFP_INVALID_STATE;
    }

    if (currentTagIndex > 2) {
        return NFP_OUT_OF_RANGE;
    }

    // TODO tag state stuff

    NTAGData* data = tagStates[currentTagIndex].tag->GetData();
    ReadReadOnlyInfo(outRomInfo, data);

    return NFP_SUCCESS;
}

Result TagManager::GetNfpAdminInfo(AdminInfo* outAdminInfo)
{
    if (!outAdminInfo) {
        return NFP_INVALID_PARAM;
    }

    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Mounted) {
        return NFP_INVALID_STATE;
    }

    if (currentTagIndex > 2) {
        return NFP_OUT_OF_RANGE;
    }

    // TODO tag state stuff

    NTAGData* data = tagStates[currentTagIndex].tag->GetData();
    ReadAdminInfo(outAdminInfo, data);

    return NFP_SUCCESS;
}

Result TagManager::SetNfpRegisterInfo(RegisterInfoSet const& info)
{
    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Mounted) {
        return NFP_INVALID_STATE;
    }

    if (readOnly) {
        return NFP_INVALID_STATE;
    }

    if (currentTagIndex > 2) {
        return NFP_OUT_OF_RANGE;
    }

    return tag.SetRegisterInfo(info);
}

Result TagManager::OpenStream(uint32_t id)
{
    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Mounted) {
        return NFP_INVALID_STATE;
    }

    return tagStreamImpl.Open(id);
}

Result TagManager::CloseStream()
{
    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    return tagStreamImpl.Close();
}

Result TagManager::ReadStream(void* data, uint32_t size)
{
    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Mounted) {
        memset(data, 0, size);
        return NFP_INVALID_STATE;
    }

    return tagStreamImpl.Read(data, size);
}

Result TagManager::WriteStream(const void* data, uint32_t size)
{
    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Mounted) {
        return NFP_INVALID_STATE;
    }

    return tagStreamImpl.Write(data, size);
}

void TagManager::Reset()
{
    hasTag = false;
    readOnly = false;
    activateEvent = nullptr;
    nfpState = NfpState::Uninitialized;
    currentTag = nullptr;
    currentTagIndex = 0;
    deactivateEvent = nullptr;
    memset(tagStates, 0, sizeof(tagStates));
}

bool TagManager::UpdateInternal()
{
    if (IsInitialized()) {
        // nn_nfp would update timeouts here

        return true;
    }

    return false;
}

void TagManager::SetNfpState(NfpState state)
{
    nfpState = state;

    DEBUG_FUNCTION_LINE("Setting nfpState to %u", state);

    if (state == NfpState::Found || state == NfpState::Mounted
       || state == NfpState::MountedROM) {
        return;
    }

    hasTag = false;
}

void TagManager::Activate()
{
    if (nfpState == NfpState::Searching) {
        SetNfpState(NfpState::Found);

        // nfp would notify IM here, should we do too?

        if (activateEvent) {
            OSSignalEvent(activateEvent);
        }
    }
}

void TagManager::Deactivate()
{
    if (nfpState == NfpState::Mounted || nfpState == NfpState::MountedROM) {
        Unmount();
    } else if (nfpState != NfpState::Found) {
        return;
    }

    SetNfpState(NfpState::Removed);

    // nfp would notify IM here, should we do too?

    if (deactivateEvent) {
        OSSignalEvent(deactivateEvent);
    }
}

Result TagManager::VerifyTagInfo()
{
    TagInfo info;
    Result res = GetTagInfo(&info, currentTagIndex);
    if (res.IsFailure()) {
        return res;
    }

    if (info.protocol != 0 || info.tag_type != 2) {
        return NFP_INVALID_TAG;
    }

    return NFP_SUCCESS;
}

Result TagManager::GetTagInfo(TagInfo* outTagInfo, uint8_t index)
{
    if (!outTagInfo) {
        return NFP_INVALID_PARAM;
    }

    Lock lock(&mutex);

    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (nfpState != NfpState::Found && nfpState != NfpState::Mounted
       && nfpState != NfpState:: MountedROM) {
        return NFP_INVALID_STATE;
    }

    if (index > 2) {
        return NFP_OUT_OF_RANGE;
    }

    // TODO need to figure out what these tag states are
    if (tagStates[index].state == 1) {
        return RESULT(0xa1b0e180);
    }

    memset(outTagInfo, 0, sizeof(TagInfo));

    NTAGData* data = tagStates[index].tag->GetData();
    ReadTagInfo(outTagInfo, data);

    if (outTagInfo->id.lenght > 10) {
        return NFP_INVALID_TAG;
    }

    return NFP_SUCCESS;
}

Result TagManager::MountTag()
{
    if (!currentTag) {
        return NFP_INVALID_STATE;
    }

    tagStreamImpl.tag = currentTag;

    Result res = currentTag->Unmount();
    if (res.IsFailure()) {
        return res;
    }

    tagStream.Close();

    if (currentTag != &tag) {
        return RESULT(0xe1b5db00); // what?
    }

    res = tag.Mount(!readOnly);
    if (res.IsFailure()) {
        return res;
    }

    SetNfpState(NfpState::Mounted);

    return NFP_SUCCESS;
}

Result TagManager::GetAppAreaInfo(Tag::AppAreaInfo* outInfo, uint32_t* outNumAreas, int32_t maxAreas)
{
    if (!UpdateInternal()) {
        return NFP_INVALID_STATE;
    }

    if (!outInfo || !outNumAreas) {
        return NFP_INVALID_PARAM;
    }

    if (nfpState != NfpState::Mounted) {
        memset(outInfo, 0, sizeof(Tag::AppAreaInfo) * maxAreas);
        *outNumAreas = 0;
        return NFP_INVALID_STATE;
    }

    if (!currentTag) {
        return NFP_INVALID_STATE;
    }

    return currentTag->GetAppAreaInfo(outInfo, outNumAreas, maxAreas);
}

bool TagManager::CheckRegisterInfo()
{
    if (UpdateInternal() && nfpState == NfpState::Mounted) {
        return tag.HasRegisterInfo();
    }

    return false;
}

Result TagManager::LoadTag()
{    
    // Only allow loading tags when we're searching for one for now
    if (nfpState != NfpState::Searching) {
        return NFP_STATUS_RESULT(0x54321);
    }

    FILE* f = fopen(tagEmulationPath.c_str(), "rb");
    if (!f) {
        DEBUG_FUNCTION_LINE("Failed to open: %s", tagEmulationPath.c_str());
        return NFP_STATUS_RESULT(0x12345);
    }

    // Read the tag
    NTAGRawData raw;
    if (fread(&raw, 1, sizeof(raw), f) < 0x214) {
        // We need at least everything up to the config bytes
        DEBUG_FUNCTION_LINE("Failed to read tag data");
        fclose(f);
        return NFP_STATUS_RESULT(0x12345); 
    }

    fclose(f);

    // Decrypt the tag
    NTAGData data;
    if (NTAGDecrypt(&data, &raw) != 0) {
        DEBUG_FUNCTION_LINE("Failed to parse tag");
        return NFP_STATUS_RESULT(0x12345);
    }

    if (!CheckAmiiboMagic(&data)) {
        tagStates[currentTagIndex].state = 5;
        DEBUG_FUNCTION_LINE("Invalid tag magic");
        return NFP_STATUS_RESULT(0x12345);
    }

    // Update tag data
    tag.SetData(&data);

    // Update tag path
    tag.SetPath(tagEmulationPath);

    // Simulate a tag read
    tagStates[currentTagIndex].result = 0;
    tagStates[currentTagIndex].tag = &tag;
    currentTag = &tag;
    hasTag = true;
    tagStates[currentTagIndex].state = 0;

    // Try to activate the tag directly
    Activate();

    pendingRemove = false;

    // Set time once the tag should be removed
    if (removeAfterSeconds != 0.0f) {
        pendingTagRemoveTime = OSGetTime() + OSNanosecondsToTicks(removeAfterSeconds * 1e9);
    } else {
        pendingTagRemoveTime = 0;
    }

    return NFP_SUCCESS;
}

void TagManager::HandleTagUpdates()
{
    // Custom state handling:

    // Check if the tag should be removed
    if (pendingTagRemoveTime != 0) {
        if (OSGetTime() >= pendingTagRemoveTime) {
            pendingRemove = true;
            pendingTagRemoveTime = 0;
            emulationState = EMULATION_OFF;
        }
    }

    if (nfpState == NfpState::Searching) {
        // If emulation isn't turned off and we're searching, load the tag
        if (emulationState != EMULATION_OFF) {
            Result res = LoadTag();
            DEBUG_FUNCTION_LINE("LoadTag: %x", ((NNResult) res).value);

            // We still "succeed" on failure but turn off emulation
            if (res.IsFailure()) {
                emulationState = EMULATION_OFF;
            }
        }
    } else if (nfpState == NfpState::Found || nfpState == NfpState::Mounted ||
       nfpState == NfpState::MountedROM) {
        // If emulation was turned off while in a detected state, deactivate the amiibo
        if (pendingRemove) {
            Deactivate();
            pendingRemove = false;
            pendingTagRemoveTime = 0;
            emulationState = EMULATION_OFF;
        }
    }
}

void TagManager::NotifyNFCGetTagInfo()
{
    // Set time once the tag should be removed, so it works properly in amiibo festival
    if (pendingTagRemoveTime == 0 && removeAfterSeconds != 0.0f) {
        pendingTagRemoveTime = OSGetTime() + OSNanosecondsToTicks(removeAfterSeconds * 1e9);
    }
}

} // namespace re::nfpii
