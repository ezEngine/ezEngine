#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Transform.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

template <typename Type>
class ezRectTemplate;
using ezRectFloat = ezRectTemplate<float>;

class ezFormatString;
class ezFrustum;
struct ezRenderViewContext;

/// \brief Draws simple shapes into the scene or view.
///
/// Shapes can be rendered for a single frame, or 'persistent' for a certain duration.
/// The 'context' specifies whether shapes are generally visible in a scene, from all views,
/// or specific to a single view. See the ezDebugRendererContext constructors for what can be implicitly
/// used as a context.
class EZ_RENDERERCORE_DLL ezDebugRenderer
{
public:
  struct Line
  {
    EZ_DECLARE_POD_TYPE();

    Line();
    Line(const ezVec3& vStart, const ezVec3& vEnd);
    Line(const ezVec3& vStart, const ezVec3& vEnd, const ezColor& color);

    ezVec3 m_start;
    ezVec3 m_end;

    ezColor m_startColor = ezColor::White;
    ezColor m_endColor = ezColor::White;
  };

  struct Triangle
  {
    EZ_DECLARE_POD_TYPE();

    Triangle();
    Triangle(const ezVec3& v0, const ezVec3& v1, const ezVec3& v2);

    ezVec3 m_position[3];
    ezColor m_color = ezColor::White;
  };

  struct TexturedTriangle
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_position[3];
    ezVec2 m_texcoord[3];
    ezColor m_color = ezColor::White;
  };

  enum class HorizontalAlignment : ezUInt8
  {
    Left,
    Center,
    Right
  };

  enum class VerticalAlignment : ezUInt8
  {
    Top,
    Center,
    Bottom
  };

  enum class ScreenPlacement : ezUInt8
  {
    TopLeft,
    TopCenter,
    TopRight,
    BottomLeft,
    BottomCenter,
    BottomRight,

    ENUM_COUNT
  };

  /// \brief Renders the given set of lines for one frame.
  static void DrawLines(const ezDebugRendererContext& context, ezArrayPtr<const Line> lines, const ezColor& color, const ezTransform& transform = ezTransform::MakeIdentity());

  /// \brief Renders the given set of lines in 2D (screen-space) for one frame.
  static void Draw2DLines(const ezDebugRendererContext& context, ezArrayPtr<const Line> lines, const ezColor& color);

  /// \brief Renders a cross for one frame.
  static void DrawCross(const ezDebugRendererContext& context, const ezVec3& vGlobalPosition, float fLineLength, const ezColor& color, const ezTransform& transform = ezTransform::MakeIdentity());

  /// \brief Renders a wireframe box for one frame.
  static void DrawLineBox(const ezDebugRendererContext& context, const ezBoundingBox& box, const ezColor& color, const ezTransform& transform = ezTransform::MakeIdentity());

  /// \brief Renders the corners of a wireframe box for one frame.
  static void DrawLineBoxCorners(const ezDebugRendererContext& context, const ezBoundingBox& box, float fCornerFraction, const ezColor& color, const ezTransform& transform = ezTransform::MakeIdentity());

  /// \brief Renders a wireframe sphere for one frame.
  static void DrawLineSphere(const ezDebugRendererContext& context, const ezBoundingSphere& sphere, const ezColor& color, const ezTransform& transform = ezTransform::MakeIdentity());

  /// \brief Renders an upright wireframe capsule for one frame.
  static void DrawLineCapsuleZ(const ezDebugRendererContext& context, float fLength, float fRadius, const ezColor& color, const ezTransform& transform = ezTransform::MakeIdentity());

  /// \brief Renders a wireframe frustum for one frame.
  static void DrawLineFrustum(const ezDebugRendererContext& context, const ezFrustum& frustum, const ezColor& color, bool bDrawPlaneNormals = false);

  /// \brief Renders a solid box for one frame.
  static void DrawSolidBox(const ezDebugRendererContext& context, const ezBoundingBox& box, const ezColor& color, const ezTransform& transform = ezTransform::MakeIdentity());

  /// \brief Renders the set of filled triangles for one frame.
  static void DrawSolidTriangles(const ezDebugRendererContext& context, ezArrayPtr<Triangle> triangles, const ezColor& color);

  /// \brief Renders the set of textured triangles for one frame.
  static void DrawTexturedTriangles(const ezDebugRendererContext& context, ezArrayPtr<TexturedTriangle> triangles, const ezColor& color, const ezTexture2DResourceHandle& hTexture);

  /// \brief Renders a filled 2D rectangle in screen-space for one frame.
  static void Draw2DRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color);

  /// \brief Renders a textured 2D rectangle in screen-space for one frame.
  static void Draw2DRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color, const ezTexture2DResourceHandle& hTexture, ezVec2 vScale = ezVec2(1, 1));

  /// \brief Renders a textured 2D rectangle in screen-space for one frame.
  static void Draw2DRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color, ezGALResourceViewHandle hResourceView, ezVec2 vScale = ezVec2(1, 1));

  /// \brief Displays a string in screen-space for one frame.
  ///
  /// The string may contain newlines (\n) for multi-line output.
  /// If horizontal alignment is right, the entire text block is aligned according to the longest line.
  /// If vertical alignment is bottom, the entire text block is aligned there.
  ///
  /// Data can be output as a table, by separating columns with tabs (\t). For example:
  /// "| Col 1\t| Col 2\t| Col 3\t|\n| abc\t| 42\t| 11.23\t|"
  ///
  /// Returns the number of lines that the text was split up into.
  static ezUInt32 Draw2DText(const ezDebugRendererContext& context, const ezFormatString& text, const ezVec2I32& vPositionInPixel, const ezColor& color, ezUInt32 uiSizeInPixel = 16, HorizontalAlignment horizontalAlignment = HorizontalAlignment::Left, VerticalAlignment verticalAlignment = VerticalAlignment::Top);

  /// \brief Draws a piece of text in one of the screen corners.
  ///
  /// Text positioning is automatic, all lines are placed in each corner such that they don't overlap.
  /// Text from different corners may overlap, though.
  ///
  /// For text formatting options, see Draw2DText().
  ///
  /// The \a groupName parameter is used to insert whitespace between unrelated pieces of text,
  /// it is not displayed anywhere, though.
  ///
  /// Text size cannot be changed.
  static void DrawInfoText(const ezDebugRendererContext& context, ScreenPlacement placement, const char* szGroupName, const ezFormatString& text, const ezColor& color = ezColor::White);

  /// \brief Displays a string in 3D space for one frame.
  static ezUInt32 Draw3DText(const ezDebugRendererContext& context, const ezFormatString& text, const ezVec3& vGlobalPosition, const ezColor& color, ezUInt32 uiSizeInPixel = 16, HorizontalAlignment horizontalAlignment = HorizontalAlignment::Center, VerticalAlignment verticalAlignment = VerticalAlignment::Bottom);

  /// \brief Renders a cross at the given location for as many frames until \a duration has passed.
  static void AddPersistentCross(const ezDebugRendererContext& context, float fSize, const ezColor& color, const ezTransform& transform, ezTime duration);

  /// \brief Renders a wireframe sphere at the given location for as many frames until \a duration has passed.
  static void AddPersistentLineSphere(const ezDebugRendererContext& context, float fRadius, const ezColor& color, const ezTransform& transform, ezTime duration);

  /// \brief Renders a wireframe box at the given location for as many frames until \a duration has passed.
  static void AddPersistentLineBox(const ezDebugRendererContext& context, const ezVec3& vHalfSize, const ezColor& color, const ezTransform& transform, ezTime duration);

  /// \brief Renders a solid 2D cone in a plane with a given angle.
  ///
  /// The rotation goes around the given \a rotationAxis.
  /// An angle of zero is pointing into forwardAxis direction.
  /// Both angles may be negative.
  static void DrawAngle(const ezDebugRendererContext& context, ezAngle startAngle, ezAngle endAngle, const ezColor& solidColor, const ezColor& lineColor, const ezTransform& transform, ezVec3 vForwardAxis = ezVec3::MakeAxisX(), ezVec3 vRotationAxis = ezVec3::MakeAxisZ());

  /// \brief Renders a cone with the tip at the center position, opening up with the given angle.
  static void DrawOpeningCone(const ezDebugRendererContext& context, ezAngle halfAngle, const ezColor& colorInside, const ezColor& colorOutside, const ezTransform& transform, ezVec3 vForwardAxis = ezVec3::MakeAxisX());

  /// \brief Renders a bent cone with the tip at the center position, pointing into the +X direction opening up with halfAngle1 and halfAngle2 along the Y and Z axis.
  ///
  /// If solidColor.a > 0, the cone is rendered with as solid triangles.
  /// If lineColor.a > 0, the cone is rendered as lines.
  /// Both can be combined.
  static void DrawLimitCone(const ezDebugRendererContext& context, ezAngle halfAngle1, ezAngle halfAngle2, const ezColor& solidColor, const ezColor& lineColor, const ezTransform& transform);

  /// \brief Renders a cylinder starting at the center position, along the +X axis.
  ///
  /// If the start and end radius are different, a cone or arrow can be created.
  static void DrawCylinder(const ezDebugRendererContext& context, float fRadiusStart, float fRadiusEnd, float fLength, const ezColor& solidColor, const ezColor& lineColor, const ezTransform& transform, bool bCapStart = false, bool bCapEnd = false);

private:
  friend class ezSimpleRenderPass;

  static void Render(const ezRenderViewContext& renderViewContext);
  static void RenderInternal(const ezDebugRendererContext& context, const ezRenderViewContext& renderViewContext);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, DebugRenderer);
};

#include <RendererCore/Debug/Implementation/DebugRenderer_inl.h>
