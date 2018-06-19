#include <PCH.h>

#include <RtsGamePlugin/Components/ComponentMessages.h>

// clang-format off

EZ_IMPLEMENT_MESSAGE_TYPE(RtsMsgNavigateTo);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsMsgNavigateTo, 1, ezRTTIDefaultAllocator<RtsMsgNavigateTo>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(RtsMsgAssignPosition);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsMsgAssignPosition, 1, ezRTTIDefaultAllocator<RtsMsgAssignPosition>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(RtsMsgSetTarget);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsMsgSetTarget, 1, ezRTTIDefaultAllocator<RtsMsgSetTarget>)
EZ_END_DYNAMIC_REFLECTED_TYPE


EZ_IMPLEMENT_MESSAGE_TYPE(RtsMsgApplyDamage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsMsgApplyDamage, 1, ezRTTIDefaultAllocator<RtsMsgApplyDamage>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(RtsMsgUnitHealthStatus);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsMsgUnitHealthStatus, 1, ezRTTIDefaultAllocator<RtsMsgUnitHealthStatus>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(RtsMsgGatherUnitStats);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsMsgGatherUnitStats, 1, ezRTTIDefaultAllocator<RtsMsgGatherUnitStats>)
EZ_END_DYNAMIC_REFLECTED_TYPE

// clang-format on
