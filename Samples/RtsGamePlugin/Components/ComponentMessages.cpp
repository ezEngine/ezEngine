#include <PCH.h>
#include <RtsGamePlugin/Components/ComponentMessages.h>

EZ_IMPLEMENT_MESSAGE_TYPE(RtsMsgNavigateTo);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsMsgNavigateTo, 1, ezRTTIDefaultAllocator<RtsMsgNavigateTo>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(RtsMsgSetTarget);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsMsgSetTarget, 1, ezRTTIDefaultAllocator<RtsMsgSetTarget>)
EZ_END_DYNAMIC_REFLECTED_TYPE
