#include <PCH.h>
#include <RendererCore/Messages/ApplyOnlyToMessage.h>

EZ_IMPLEMENT_MESSAGE_TYPE(ezApplyOnlyToMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezApplyOnlyToMessage, 1, ezRTTIDefaultAllocator<ezApplyOnlyToMessage>)
{
  ///\todo enable this once we have object reference properties
  /*EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Object", m_hObject),
  }
  EZ_END_PROPERTIES*/
}
EZ_END_DYNAMIC_REFLECTED_TYPE
