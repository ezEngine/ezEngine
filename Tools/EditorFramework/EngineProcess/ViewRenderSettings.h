#pragma once

#include <EditorFramework/Plugin.h>
#include <CoreUtils/Graphics/Camera.h>

struct ezViewRenderMode
{
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

struct ezSceneViewPerspective
{
  enum Enum
  {
    Orhogonal_Front,
    Orhogonal_Right,
    Orhogonal_Top,
    Perspective,

    Default = Perspective
  };

};

struct EZ_EDITORFRAMEWORK_DLL ezSceneViewConfig
{
  ezViewRenderMode::Enum m_RenderMode;
  ezSceneViewPerspective::Enum m_Perspective;

  ezCamera m_Camera;

  void ApplyPerspectiveSetting();
};




