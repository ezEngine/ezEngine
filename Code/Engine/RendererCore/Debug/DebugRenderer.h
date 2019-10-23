#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Transform.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <RendererCore/Declarations.h>

struct ezRenderViewContext;

template <typename Type>
class ezRectTemplate;
typedef ezRectTemplate<float> ezRectFloat;

class ezFrustum;

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
  };

  struct Triangle
  {
    EZ_DECLARE_POD_TYPE();

    Triangle();
    Triangle(const ezVec3& p0, const ezVec3& p1, const ezVec3& p2);

    ezVec3 m_p0;
    ezVec3 m_p1;
    ezVec3 m_p2;
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


  static void DrawLines(const ezDebugRendererContext& context, ezArrayPtr<Line> lines, const ezColor& color);

  static void DrawCross(const ezDebugRendererContext& context, const ezVec3& globalPosition, float fLineLength, const ezColor& color);

  static void DrawLineBox(const ezDebugRendererContext& context, const ezBoundingBox& box, const ezColor& color,
                          const ezTransform& transform = ezTransform::IdentityTransform());

  static void DrawLineBoxCorners(const ezDebugRendererContext& context, const ezBoundingBox& box, float fCornerFraction,
                                 const ezColor& color, const ezTransform& transform = ezTransform::IdentityTransform());

  static void DrawLineSphere(const ezDebugRendererContext& context, const ezBoundingSphere& sphere, const ezColor& color,
                             const ezTransform& transform = ezTransform::IdentityTransform());

  static void DrawLineCapsuleZ(const ezDebugRendererContext& context, float fLength, float fRadius, const ezColor& color,
                               const ezTransform& transform = ezTransform::IdentityTransform());

  static void DrawLineFrustum(const ezDebugRendererContext& context, const ezFrustum& frustum, const ezColor& color,
                              bool bDrawPlaneNormals = false);

  static void DrawSolidBox(const ezDebugRendererContext& context, const ezBoundingBox& box, const ezColor& color,
                           const ezTransform& transform = ezTransform::IdentityTransform());

  static void DrawSolidTriangles(const ezDebugRendererContext& context, ezArrayPtr<Triangle> triangles, const ezColor& color);

  static void Draw2DRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color);

  static void Draw2DRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color,
                              const ezTexture2DResourceHandle& hTexture);

  static void Draw2DText(const ezDebugRendererContext& context, const ezStringView& text, const ezVec2I32& positionInPixel,
                       const ezColor& color, ezUInt32 uiSizeInPixel = 16,
                       HorizontalAlignment::Enum horizontalAlignment = HorizontalAlignment::Left,
                       VerticalAlignment::Enum verticalAlignment = VerticalAlignment::Top);

  static void Draw3DText(const ezDebugRendererContext& context, const ezStringView& text, const ezVec3& globalPosition,
                         const ezColor& color, ezUInt32 uiSizeInPixel = 16,
                         HorizontalAlignment::Enum horizontalAlignment = HorizontalAlignment::Left,
                         VerticalAlignment::Enum verticalAlignment = VerticalAlignment::Top);

private:
  friend class ezSimpleRenderPass;

  static void Render(const ezRenderViewContext& renderViewContext);
  static void RenderInternal(const ezDebugRendererContext& context, const ezRenderViewContext& renderViewContext);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, DebugRenderer);
};

#include <RendererCore/Debug/Implementation/DebugRenderer_inl.h>

