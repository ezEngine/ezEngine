#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Core/Graphics/Camera.h>
#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/Pipeline/ViewRenderMode.h>

struct EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezSceneViewPerspective
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Orthogonal_Front,
    Orthogonal_Right,
    Orthogonal_Top,
    Perspective,

    Default = Perspective
  };

};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORENGINEPROCESSFRAMEWORK_DLL, ezSceneViewPerspective);

struct EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEngineViewConfig
{
  ezEngineViewConfig()
  {
    m_RenderMode = ezViewRenderMode::Default;
    m_Perspective = ezSceneViewPerspective::Default;
    m_CameraUsageHint = ezCameraUsageHint::EditorView;
    m_pLinkedViewConfig = nullptr;
  }

  ezViewRenderMode::Enum m_RenderMode;
  ezSceneViewPerspective::Enum m_Perspective;
  ezCameraUsageHint::Enum m_CameraUsageHint;
  bool m_bUseCameraTransformOnDevice = true;

  ezCamera m_Camera;
  ezEngineViewConfig* m_pLinkedViewConfig; // used to store which other view config this is linked to, for resetting values when switching views

  void ApplyPerspectiveSetting(float fov = 0.0f, float nearPlane = 0.1f, float farPlane = 1000.0f);
};




