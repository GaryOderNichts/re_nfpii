#include "Cabinet.hpp"
#include "re_nfpii.hpp"
#include "Utils.hpp"

#include <coreinit/memory.h>
#include <coreinit/dynload.h>
#include <sysapp/args.h>
#include <sysapp/switch.h>

namespace re::nfpii::Cabinet {

// The imported functions for dynamic modules won't be correct while in an applet,
// so we dynload them as a workaround.
int32_t (*dyn_SYSDeserializeSysArgs)(SYSDeserializeCallback callback, void *userArg);
int32_t (*dyn__SYSDirectlyReturnToCaller)(SYSStandardArgsOut *arg);
BOOL (*dyn__SYSDeserializeStandardArg)(SYSDeserializeArg *deserializeArg, SYSStandardArgs *standardArg);

static void loadSysappFunctionPointers()
{
    OSDynLoad_Module sysappModule;
    OSDynLoad_Acquire("sysapp.rpl", &sysappModule);

    OSDynLoad_FindExport(sysappModule, FALSE, "SYSDeserializeSysArgs", (void**) &dyn_SYSDeserializeSysArgs);
    OSDynLoad_FindExport(sysappModule, FALSE, "_SYSDirectlyReturnToCaller", (void**) &dyn__SYSDirectlyReturnToCaller);
    OSDynLoad_FindExport(sysappModule, FALSE, "_SYSDeserializeStandardArg", (void**) &dyn__SYSDeserializeStandardArg);
}

static void OnCabinetEnter()
{
    // Need to dynload sysapp pointers while in an applet
    loadSysappFunctionPointers();

    tagManager.SetAmiiboSettings(true);
}

static void OnCabinetLeave()
{
    tagManager.SetAmiiboSettings(false);
}

static void AmiiboSettingsDeserializationCallback(SYSDeserializeArg* arg, void* usrarg)
{
    AmiiboSettingsArgs* amiiboArgs = (AmiiboSettingsArgs*) usrarg;
    if (!amiiboArgs) {
        return;
    }

    if (dyn__SYSDeserializeStandardArg(arg, &amiiboArgs->standardArg)) {
        return;
    }

    if (strcmp(arg->argName, "amb:mode") == 0) {
        OSBlockMove(&amiiboArgs->argsIn.mode, arg->data, sizeof(amiiboArgs->argsIn.mode), TRUE);
        return;
    }

    if (strcmp(arg->argName, "amb:tag_info") == 0) {
        OSBlockMove(&amiiboArgs->argsIn.tag_info, arg->data, sizeof(amiiboArgs->argsIn.tag_info), TRUE);
        return;
    }

    if (strcmp(arg->argName, "amb:is_registered") == 0) {
        OSBlockMove(&amiiboArgs->argsIn.is_registered, arg->data, sizeof(amiiboArgs->argsIn.is_registered), TRUE);
        return;
    }

    if (strcmp(arg->argName, "amb:register_info") == 0) {
        OSBlockMove(&amiiboArgs->argsIn.register_info, arg->data, sizeof(amiiboArgs->argsIn.register_info), TRUE);
        return;
    }

    if (strcmp(arg->argName, "amb:common_info") == 0) {
        OSBlockMove(&amiiboArgs->argsIn.common_info, arg->data, sizeof(amiiboArgs->argsIn.common_info), TRUE);
        return;
    }
}

Result GetArgs(AmiiboSettingsArgs* args)
{
    OnCabinetEnter();

    if (!args) {
        return NFP_INVALID_PARAM;
    }

    memset(args, 0, sizeof(*args));
    if (dyn_SYSDeserializeSysArgs(AmiiboSettingsDeserializationCallback, args) != 0) {
        return RESULT(0xa1b3e880);
    }

    return NFP_SUCCESS;
}

struct AmiiboSettingsReturnArg {
    char argName[0x10];
    AmiiboSettingsResult result;
};

static bool VerifyResultValueBlock(SYSArgDataBlock const& block)
{
    if (block.type != SYS_ARG_TYPE_DATA) {
        return false;
    } 
    
    if (block.data.size != sizeof(AmiiboSettingsResult) + sizeof("ambResultValue:")) {
        return false;
    }

    if (memcmp(block.data.ptr, "ambResultValue:", sizeof("ambResultValue:")) != 0) {
        return false;
    }

    return true;
}

Result GetResult(AmiiboSettingsResult* result, SYSArgDataBlock const& block)
{
    if (!result) {
        return NFP_INVALID_PARAM;
    }

    Initialize();

    if (!VerifyResultValueBlock(block)) {
        return RESULT(0xa1b12c00);
    }

    AmiiboSettingsReturnArg* returnArg = (AmiiboSettingsReturnArg*) block.data.ptr;
    OSBlockMove(result, &returnArg->result, sizeof(returnArg->result), TRUE);

    return NFP_SUCCESS;
}

Result InitializeArgsIn(AmiiboSettingsArgsIn* args)
{
    if (!args) {
        return NFP_INVALID_PARAM;
    }

    args->mode = 0;
    memset(&args->tag_info, 0, sizeof(args->tag_info));
    args->is_registered = false;
    memset(&args->padding, 0, sizeof(args->padding));
    memset(&args->register_info, 0, sizeof(args->register_info));
    memset(&args->common_info, 0, sizeof(args->common_info));
    memset(args->reserved, 0, sizeof(args->reserved));

    return NFP_SUCCESS;
}

Result ReturnToCallerWithResult(AmiiboSettingsResult const& result)
{
    AmiiboSettingsReturnArg returnArg;
    OSBlockMove(returnArg.argName, "ambResultValue:", sizeof("ambResultValue:"), TRUE);
    OSBlockMove(&returnArg.result, &result, sizeof(result), TRUE);
    memset(returnArg.result.reserved, 0, sizeof(returnArg.result.reserved));

    SYSStandardArgsOut args;
    args.data = &returnArg;
    args.size = sizeof(returnArg);

    Finalize();

    OnCabinetLeave();

    if (dyn__SYSDirectlyReturnToCaller(&args) != 0) {
        return RESULT(0xa1b3e880);
    }

    return NFP_SUCCESS;
}

static bool VerifyAmiiboSettingsArgs(AmiiboSettingsArgsIn const& args)
{
    if (args.mode != 0x0 && args.mode != 0x1 && args.mode != 0x2 && args.mode != 0x64) {
        return false;
    }

    if (!CheckZero(&args.tag_info.reserved1, sizeof(args.tag_info.reserved1))) {
        return false;
    }
    if (!CheckZero(&args.padding, sizeof(args.padding))) {
        return false;
    }
    if (!CheckZero(&args.common_info.reserved, sizeof(args.common_info.reserved))) {
        return false;
    }
    if (!CheckZero(&args.register_info.reserved, sizeof(args.register_info.reserved))) {
        return false;
    }
    if (!CheckZero(&args.reserved, sizeof(args.reserved))) {
        return false;
    }

    return true;
}

Result SwitchToCabinet(AmiiboSettingsArgsIn const& args, const char* standardArg, uint32_t standardArgSize)
{
    if (!VerifyAmiiboSettingsArgs(args)) {
        return NFP_INVALID_PARAM;
    }

    SYSClearSysArgs();
    SYSStandardArgsIn standardArgs;
    standardArgs.argString = standardArg;
    standardArgs.size = standardArgSize;
    if (_SYSSerializeStandardArgsIn(&standardArgs) < 0) {
        return NFP_INVALID_PARAM;
    }

    if (SYSSerializeSysArgs("amb:mode", &args.mode, sizeof(args.mode)) < 0) {
        return NFP_INVALID_PARAM;
    }

    if (SYSSerializeSysArgs("amb:tag_info", &args.tag_info, sizeof(args.tag_info)) < 0) {
        return NFP_INVALID_PARAM;
    }

    if (SYSSerializeSysArgs("amb:is_registered", &args.is_registered, sizeof(args.is_registered)) < 0) {
        return NFP_INVALID_PARAM;
    }

    if (SYSSerializeSysArgs("amb:register_info", &args.register_info, sizeof(args.register_info)) < 0) {
        return NFP_INVALID_PARAM;
    }

    if (SYSSerializeSysArgs("amb:common_info", &args.common_info, sizeof(args.common_info)) < 0) {
        return NFP_INVALID_PARAM;
    }

    Finalize();

    // Switch to amiibo settings
    if (_SYSDirectlySwitchTo(SYSAPP_PFID_CABINETU) != 0) {
        return RESULT(0xa1b3e880);
    }

    return NFP_SUCCESS;
}

} // namespace re::nfpii::Cabinet
