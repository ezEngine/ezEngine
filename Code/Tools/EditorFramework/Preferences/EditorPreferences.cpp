#include <PCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Profiling/Profiling.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorPreferencesUser, 1, ezRTTIDefaultAllocator<ezEditorPreferencesUser>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RestoreProjectOnStartup", m_bLoadLastProjectAtStartup)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("FieldOfView", m_fPerspectiveFieldOfView)->AddAttributes(new ezDefaultValueAttribute(70.0f), new ezClampValueAttribute(10.0f, 150.0f)),
    EZ_MEMBER_PROPERTY("GizmoScale", m_fGizmoScale)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.2f, 5.0f)),
    EZ_MEMBER_PROPERTY("UsePrecompiledTools", m_bUsePrecompiledTools)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("ExpandSceneTreeOnSelection", m_bExpandSceneTreeOnSelection)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezEditorPreferencesUser::ezEditorPreferencesUser()
    : ezPreferences(Domain::Application, "General")
{
}

void ezQtEditorApp::LoadEditorPreferences()
{
  EZ_PROFILE_SCOPE("Preferences");
  ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
}
