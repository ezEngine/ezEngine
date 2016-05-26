#include <PCH.h>
#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>


void ezSceneViewConfig::ApplyPerspectiveSetting(float fov)
{
  switch (m_Perspective)
  {
  case ezSceneViewPerspective::Perspective:
    {
      m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovY, fov == 0.0f ? 90.0f : fov, 0.1f, 1000.0f);
    }
    break;

  case ezSceneViewPerspective::Orthogonal_Front:
    {
      m_Camera.SetCameraMode(ezCameraMode::OrthoFixedHeight, fov == 0.0f ? 20.0f : fov, -10000.0f, 10000.0f);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + ezVec3(-1, 0, 0), ezVec3(0, 0, 1));
    }
    break;

  case ezSceneViewPerspective::Orthogonal_Right:
    {
      m_Camera.SetCameraMode(ezCameraMode::OrthoFixedHeight, fov == 0.0f ? 20.0f : fov, -10000.0f, 10000.0f);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + ezVec3(0, -1, 0), ezVec3(0, 0, 1));
    }
    break;

  case ezSceneViewPerspective::Orthogonal_Top:
    {
      m_Camera.SetCameraMode(ezCameraMode::OrthoFixedHeight, fov == 0.0f ? 20.0f : fov, -10000.0f, 10000.0f);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + ezVec3(0, 0, -1), ezVec3(0, -1, 0));
    }
    break;
  }

}


