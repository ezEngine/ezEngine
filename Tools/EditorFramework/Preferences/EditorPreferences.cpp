#include <PCH.h>
#include <EditorFramework/Preferences/EditorPreferences.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorPreferencesShared, 1, ezRTTIDefaultAllocator<ezEditorPreferencesShared>)
{
  //EZ_BEGIN_PROPERTIES
  //{
  //}
  //EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezEditorPreferencesShared::ezEditorPreferencesShared()
  : ezPreferences(Domain::Application, ezPreferences::Visibility::Shared, "Editor - General")
{
}

//////////////////////////////////////////////////////////////////////////


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorPreferencesUser, 1, ezRTTIDefaultAllocator<ezEditorPreferencesUser>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Gizmo Scale", m_fGizmoScale)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.2f, 5.0f)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezEditorPreferencesUser::ezEditorPreferencesUser()
  : ezPreferences(Domain::Application, ezPreferences::Visibility::User, "Editor - General")
{
  m_fGizmoScale = 1.0f;
}
