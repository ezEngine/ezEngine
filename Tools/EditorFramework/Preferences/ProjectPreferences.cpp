#include <PCH.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Profiling/Profiling.h>


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
  : ezPreferences(Domain::Project, "General")
{
}


void ezQtEditorApp::LoadProjectPreferences()
{
  EZ_PROFILE("LoadProjectPreferences");
  ezPreferences::QueryPreferences<ezProjectPreferencesUser>();
}
