#include <PCH.h>
#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>


void ezSceneViewConfig::ApplyPerspectiveSetting()
{
  switch (m_Perspective)
  {
  case ezSceneViewPerspective::Perspective:
    {
      m_Camera.SetCameraMode(ezCamera::PerspectiveFixedFovY, 90.0f, 0.1f, 1000.0f);
    }
    break;

  case ezSceneViewPerspective::Orhogonal_Front:
    {
      m_Camera.SetCameraMode(ezCamera::OrthoFixedHeight, 20.0f, -10000.0f, 10000.0f);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + ezVec3(-1, 0, 0), ezVec3(0, 0, 1));
    }
    break;

  case ezSceneViewPerspective::Orhogonal_Right:
    {
      m_Camera.SetCameraMode(ezCamera::OrthoFixedHeight, 20.0f, -10000.0f, 10000.0f);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + ezVec3(0, -1, 0), ezVec3(0, 0, 1));
    }
    break;

  case ezSceneViewPerspective::Orhogonal_Top:
    {
      m_Camera.SetCameraMode(ezCamera::OrthoFixedHeight, 20.0f, -10000.0f, 10000.0f);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + ezVec3(0, 0, -1), ezVec3(1, 0, 0));
    }
    break;
  }

}


