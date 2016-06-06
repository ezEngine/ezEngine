#include <PCH.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

//////////////////////////////////////////////////////////////////////////


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorPreferencesUser, 1, ezRTTIDefaultAllocator<ezEditorPreferencesUser>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Gizmo Scale", m_fGizmoScale)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.2f, 5.0f)),
    EZ_MEMBER_PROPERTY("Use Precompiled Tools", m_bUsePrecompiledTools)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezEditorPreferencesUser::ezEditorPreferencesUser()
  : ezPreferences(Domain::Application, "General")
{
  m_fGizmoScale = 1.0f;
  m_bUsePrecompiledTools = true;
}



void ezQtEditorApp::LoadEditorPreferences()
{
  ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
}