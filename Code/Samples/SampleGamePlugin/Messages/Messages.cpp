#include <SampleGamePluginPCH.h>

#include <SampleGamePlugin/Messages/Messages.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgSetText);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgSetText, 1, ezRTTIDefaultAllocator<ezMsgSetText>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
