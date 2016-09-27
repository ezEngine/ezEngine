#pragma once

#include <EditorFramework/Plugin.h>
#include <CoreUtils/Graphics/Camera.h>
#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/Components/Declarations.h>

struct EZ_EDITORFRAMEWORK_DLL ezViewRenderMode
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None,
    WireframeColor,
    WireframeMonochrome,
    LitOnly,
    TexCoordsUV0,
    Normals,
    DiffuseColor,
    DiffuseColorRange,
    SpecularColor,
    Roughness,
    Depth,

    Default = None
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORFRAMEWORK_DLL, ezViewRenderMode);

struct EZ_EDITORFRAMEWORK_DLL ezSceneViewPerspective
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
EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORFRAMEWORK_DLL, ezSceneViewPerspective);

struct EZ_EDITORFRAMEWORK_DLL ezSceneViewConfig
{
  ezSceneViewConfig()
  {
    m_RenderMode = ezViewRenderMode::Default;
    m_Perspective = ezSceneViewPerspective::Default;
    m_CameraUsageHint = ezCameraComponentUsageHint::EditorView;
    m_pLinkedViewConfig = nullptr;
  }

  ezViewRenderMode::Enum m_RenderMode;
  ezSceneViewPerspective::Enum m_Perspective;
  ezCameraComponentUsageHint::Enum m_CameraUsageHint;

  ezCamera m_Camera;
  ezSceneViewConfig* m_pLinkedViewConfig; // used to store which other view config this is linked to, for resetting values when switching views

  void ApplyPerspectiveSetting(float fov = 0.0f);
};




