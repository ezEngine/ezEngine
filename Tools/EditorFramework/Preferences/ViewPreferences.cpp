#include <PCH.h>
#include <EditorFramework/Preferences/ViewPreferences.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewUserPreferences, 1, ezRTTIDefaultAllocator<ezViewUserPreferences>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RenderPipelines", m_sRenderPipelines),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE
