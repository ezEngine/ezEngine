#include <PCH.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProjectPreferencesShared, 1, ezRTTIDefaultAllocator<ezProjectPreferencesShared>)
{
  //EZ_BEGIN_PROPERTIES
  //{
  //}
  //EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezProjectPreferencesShared::ezProjectPreferencesShared()
  : ezPreferences(Domain::Application, ezPreferences::Visibility::Shared, "Project - General")
{
}

//////////////////////////////////////////////////////////////////////////


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProjectPreferencesUser, 1, ezRTTIDefaultAllocator<ezProjectPreferencesUser>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RenderPipelines", m_sRenderPipelines),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezProjectPreferencesUser::ezProjectPreferencesUser()
  : ezPreferences(Domain::Application, ezPreferences::Visibility::User, "Project - General")
{
}
