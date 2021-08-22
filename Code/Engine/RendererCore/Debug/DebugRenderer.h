#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Transform.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/RendererCoreDLL.h>

template <typename Type>
class ezRectTemplate;

using ezRectFloat = ezRectTemplate<float>;

class ezFormatString;
class ezFrustum;
struct ezRenderViewContext;

class EZ_RENDERERCORE_DLL ezDebugRenderer
{
public:
  struct Line
  {
    EZ_DECLARE_POD_TYPE();

    Line();
    Line(const ezVec3& start, const ezVec3& end);

    ezVec3 m_start;
    ezVec3 m_end;

    ezColor m_startColor = ezColor::White;
    ezColor m_endColor = ezColor::White;
  };

  struct Triangle
  {
    EZ_DECLARE_POD_TYPE();

    Triangle();
    Triangle(const ezVec3& p0, const ezVec3& p1, const ezVec3& p2);

    ezVec3 m_position[3];
  };

  struct TexturedTriangle
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_position[3];
    ezVec2 m_texcoord[3];
  };

  struct HorizontalAlignment
  {
    enum Enum
    {
      Left,
      Center,
      Right
    };
  };

  struct VerticalAlignment
  {
    enum Enum
    {
      Top,
      Center,
      Bottom
    };
  };


  /// \brief Renders the given set of lines for one frame.
  static void DrawLines(const ezDebugRendererContext& context, ezArrayPtr<const Line> lines, const ezColor& color, const ezTransform& transform = ezTransform::IdentityTransform());

  /// \brief Renders the given set of lines in 2D (screenspace) for one frame.
  static void Draw2DLines(const ezDebugRendererContext& context, ezArrayPtr<const Line> lines, const ezColor& color);

  /// \brief Renders a cross for one frame.
  static void DrawCross(const ezDebugRendererContext& context, const ezVec3& globalPosition, float fLineLength, const ezColor& color, const ezTransform& transform = ezTransform::IdentityTransform());

  /// \brief Renders a wireframe box for one frame.
  static void DrawLineBox(const ezDebugRendererContext& context, const ezBoundingBox& box, const ezColor& color, const ezTransform& transform = ezTransform::IdentityTransform());

  /// \brief Renders the corners of a wireframe box for one frame.
  static void DrawLineBoxCorners(const ezDebugRendererContext& context, const ezBoundingBox& box, float fCornerFraction, const ezColor& color, const ezTransform& transform = ezTransform::IdentityTransform());

  /// \brief Renders a wireframe sphere for one frame.
  static void DrawLineSphere(const ezDebugRendererContext& context, const ezBoundingSphere& sphere, const ezColor& color, const ezTransform& transform = ezTransform::IdentityTransform());

  /// \brief Renders an upright wireframe capsule for one frame.
  static void DrawLineCapsuleZ(const ezDebugRendererContext& context, float fLength, float fRadius, const ezColor& color, const ezTransform& transform = ezTransform::IdentityTransform());

  /// \brief Renders a wireframe frustum for one frame.
  static void DrawLineFrustum(const ezDebugRendererContext& context, const ezFrustum& frustum, const ezColor& color, bool bDrawPlaneNormals = false);

  /// \brief Renders a solid box for one frame.
  static void DrawSolidBox(const ezDebugRendererContext& context, const ezBoundingBox& box, const ezColor& color, const ezTransform& transform = ezTransform::IdentityTransform());

  /// \brief Renders the set of filled triangles for one frame.
  static void DrawSolidTriangles(const ezDebugRendererContext& context, ezArrayPtr<Triangle> triangles, const ezColor& color);

  /// \brief Renders the set of textured triangles for one frame.
  static void DrawTexturedTriangles(const ezDebugRendererContext& context, ezArrayPtr<TexturedTriangle> triangles, const ezColor& color, const ezTexture2DResourceHandle& hTexture);

  /// \brief Renders a filled 2D rectangle in screenspace for one frame.
  static void Draw2DRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color);

  /// \brief Renders a textured 2D rectangle in screenspace for one frame.
  static void Draw2DRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color, const ezTexture2DResourceHandle& hTexture);

  /// \brief Displays a string in screenspace for one frame.
  static void Draw2DText(const ezDebugRendererContext& context, const ezFormatString& text, const ezVec2I32& positionInPixel, const ezColor& color, ezUInt32 uiSizeInPixel = 16, HorizontalAlignment::Enum horizontalAlignment = HorizontalAlignment::Left, VerticalAlignment::Enum verticalAlignment = VerticalAlignment::Top);

  /// \brief Displays a string in 3D space for one frame.
  static void Draw3DText(const ezDebugRendererContext& context, const ezFormatString& text, const ezVec3& globalPosition, const ezColor& color, ezUInt32 uiSizeInPixel = 16, HorizontalAlignment::Enum horizontalAlignment = HorizontalAlignment::Left, VerticalAlignment::Enum verticalAlignment = VerticalAlignment::Top);

  /// \brief Renders a cross at the given location for as many frames until \a duration has passed.
  static void AddPersistentCross(const ezDebugRendererContext& context, float fSize, const ezColor& color, const ezTransform& transform, ezTime duration);

  /// \brief Renders a wireframe sphere at the given location for as many frames until \a duration has passed.
  static void AddPersistentLineSphere(const ezDebugRendererContext& context, float fRadius, const ezColor& color, const ezTransform& transform, ezTime duration);

  /// \brief Renders a wireframe box at the given location for as many frames until \a duration has passed.
  static void AddPersistentLineBox(const ezDebugRendererContext& context, const ezVec3& halfSize, const ezColor& color, const ezTransform& transform, ezTime duration);

private:
  friend class ezSimpleRenderPass;

  static void Render(const ezRenderViewContext& renderViewContext);
  static void RenderInternal(const ezDebugRendererContext& context, const ezRenderViewContext& renderViewContext);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, DebugRenderer);
};

#include <RendererCore/Debug/Implementation/DebugRenderer_inl.h>
