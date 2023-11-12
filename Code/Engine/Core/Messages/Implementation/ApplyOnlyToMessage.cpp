#include <Core/CorePCH.h>

#include <Core/Messages/ApplyOnlyToMessage.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgOnlyApplyToObject);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgOnlyApplyToObject, 1, ezRTTIDefaultAllocator<ezMsgOnlyApplyToObject>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Object", m_hObject),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


EZ_STATICLINK_FILE(Core, Core_Messages_Implementation_ApplyOnlyToMessage);
