#include <wups.h>
#include <nfc.h>

#include "nn_nfp/nn_nfp.hpp"
#include "debug/logger.h"

/*  This is just here since amiibo festival <3 calls NFCGetTagInfo for amiibo
    detection instead of using nn_nfp for some reason.

    TODO check if there are any other games doing this? */

DECL_FUNCTION(NFCError, NFCGetTagInfo, uint32_t index, uint32_t unk, NFCTagInfoCallback callback, void* arg)
{
    DEBUG_FUNCTION_LINE("NFCGetTagInfo");

    if (!nn::nfp::tagManager.IsInitialized()) {
        return NFC_ERR_NOT_INIT;
    }

    if (nn::nfp::tagManager.GetEmulationState() == nn::nfp::TagManager::EMULATION_OFF) {
        return NFC_ERR_GET_TAG_INFO;
    }

    nn::nfp::tagManager.NotifyNFCGetTagInfo();

    // Get emulation path and read UID
    // TODO: amiibo festival calls this several times, should probably cache the current tag data
    //       in tagManager.
    std::string path = nn::nfp::tagManager.GetTagEmulationPath();
    if (path.empty()) {
        return NFC_ERR_GET_TAG_INFO;
    }

    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        return NFC_ERR_GET_TAG_INFO;
    }

    NFCTagInfo info;
    info.uidSize = 7;
    info.protocol = 0;
    info.tag_type = 2;

    if (fread(&info.uid, 1, info.uidSize, f) != info.uidSize) {
        fclose(f);
        return NFC_ERR_GET_TAG_INFO;
    }

    fclose(f);

    // Just call the callback directly
    callback(index, NFC_ERR_OK, &info, arg);

    return NFC_ERR_OK;
}

WUPS_MUST_REPLACE(NFCGetTagInfo, WUPS_LOADER_LIBRARY_NFC, NFCGetTagInfo);
