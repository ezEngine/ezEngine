#include <RendererCorePCH.h>

#include <RendererCore/Messages/ApplyOnlyToMessage.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgOnlyApplyToObject);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgOnlyApplyToObject, 1, ezRTTIDefaultAllocator<ezMsgOnlyApplyToObject>)
{
  ///\todo enable this once we have object reference properties
  /*EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Object", m_hObject),
  }
  EZ_END_PROPERTIES;*/
  EZ_BEGIN_ATTRIBUTES
  {
    new ezAutoGenVisScriptMsgSender
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


EZ_STATICLINK_FILE(RendererCore, RendererCore_Messages_Implementation_ApplyOnlyToMessage);

