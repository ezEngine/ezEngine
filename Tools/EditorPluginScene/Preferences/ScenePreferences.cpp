#include <PCH.h>
#include <EditorPluginScene/Preferences/ScenePreferences.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScenePreferencesUser, 1, ezRTTIDefaultAllocator<ezScenePreferencesUser>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("CameraSpeed", m_iCameraSpeed)->AddAttributes(new ezDefaultValueAttribute(15), new ezClampValueAttribute(1, 30)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE
