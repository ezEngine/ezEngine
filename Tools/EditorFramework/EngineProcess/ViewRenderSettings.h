#pragma once

#include <EditorFramework/Plugin.h>
#include <CoreUtils/Graphics/Camera.h>
#include <Foundation/Reflection/Reflection.h>

struct EZ_EDITORFRAMEWORK_DLL ezViewRenderMode
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None,
    WireframeColor,
    WireframeMonochrome,
    TexCoordsUV0,
    VertexNormals,
    PixelDepth,

    Default = None
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORFRAMEWORK_DLL, ezViewRenderMode);

struct EZ_EDITORFRAMEWORK_DLL ezSceneViewPerspective
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Orhogonal_Front,
    Orhogonal_Right,
    Orhogonal_Top,
    Perspective,

    Default = Perspective
  };

};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORFRAMEWORK_DLL, ezSceneViewPerspective);

struct EZ_EDITORFRAMEWORK_DLL ezSceneViewConfig
{
  ezViewRenderMode::Enum m_RenderMode;
  ezSceneViewPerspective::Enum m_Perspective;

  ezCamera m_Camera;

  void ApplyPerspectiveSetting();
};




