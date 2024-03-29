#include <wups.h>
#include <nfc/nfc.h>
#include <nfpii.h>
#include <cstdio>
#include <string>

#include "debug/logger.h"

/*  Some games call NFCGetTagInfo for amiibo detection instead of using nn_nfp for some reason.
    Notably Amiibo Festival and Mario Party 10 are doing this.
    This function replacement passes the callback to the module, which calls it on the next nfc proc.
*/

DECL_FUNCTION(NFCError, NFCGetTagInfo, uint32_t index, uint32_t timeout, NFCGetTagInfoCallbackFn callback, void* arg)
{
    // DEBUG_FUNCTION_LINE("NFCGetTagInfo");

    if (!NfpiiIsInitialized()) {
        return real_NFCGetTagInfo(index, timeout, callback, arg);
    }

    if (index != 0) {
        return -0x1385;
    }

    return NfpiiQueueNFCGetTagInfo(callback, arg);
}

WUPS_MUST_REPLACE(NFCGetTagInfo, WUPS_LOADER_LIBRARY_NFC, NFCGetTagInfo);
