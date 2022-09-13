#pragma once

#include <nn/nfp.h>

namespace re::nfpii {
using nn::Result;
using namespace nn::nfp;

namespace Cabinet {

Result GetArgs(AmiiboSettingsArgs* args);

Result GetResult(AmiiboSettingsResult* result, SYSArgDataBlock const& block);

Result InitializeArgsIn(AmiiboSettingsArgsIn* args);

Result ReturnToCallerWithResult(AmiiboSettingsResult const& result);

Result SwitchToCabinet(AmiiboSettingsArgsIn const& args, const char* standardArg, uint32_t standardArgSize);

};

} // namespace re::nfpii
