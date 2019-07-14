#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSceneViewPerspective, 1)
  EZ_ENUM_CONSTANTS(ezSceneViewPerspective::Orthogonal_Front, ezSceneViewPerspective::Orthogonal_Right, ezSceneViewPerspective::Orthogonal_Top, ezSceneViewPerspective::Perspective)
EZ_END_STATIC_REFLECTED_ENUM;

void ezEngineViewConfig::ApplyPerspectiveSetting(float fov, float nearPlane, float farPlane)
{
  const float fOrthoRange = 1000.0f;

  switch (m_Perspective)
  {
    case ezSceneViewPerspective::Perspective:
    {
      m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovY, fov == 0.0f ? 70.0f : fov, nearPlane, farPlane);
    }
    break;

    case ezSceneViewPerspective::Orthogonal_Front:
    {
      m_Camera.SetCameraMode(ezCameraMode::OrthoFixedHeight, fov == 0.0f ? 20.0f : fov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + ezVec3(1, 0, 0), ezVec3(0, 0, 1));
    }
    break;

    case ezSceneViewPerspective::Orthogonal_Right:
    {
      m_Camera.SetCameraMode(ezCameraMode::OrthoFixedHeight, fov == 0.0f ? 20.0f : fov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + ezVec3(0, -1, 0), ezVec3(0, 0, 1));
    }
    break;

    case ezSceneViewPerspective::Orthogonal_Top:
    {
      m_Camera.SetCameraMode(ezCameraMode::OrthoFixedHeight, fov == 0.0f ? 20.0f : fov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + ezVec3(0, 0, -1), ezVec3(1, 0, 0));
    }
    break;
  }
}
