#include <SampleGamePluginPCH.h>

#include <SampleGamePlugin/Messages/Messages.h>

// clang-format off
// BEGIN-DOCS-CODE-SNIPPET: message-impl
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgSetText);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgSetText, 1, ezRTTIDefaultAllocator<ezMsgSetText>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// END-DOCS-CODE-SNIPPET
// clang-format on
