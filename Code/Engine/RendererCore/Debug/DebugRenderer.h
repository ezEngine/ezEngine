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
class ezRenderViewContext;

/// Horizontal alignment of debug text
struct ezDebugTextHAlign
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Left,
    Center,
    Right,

    Default = Left
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezDebugTextHAlign);

/// Vertical alignment of debug text
struct ezDebugTextVAlign
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Top,
    Center,
    Bottom,

    Default = Top
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezDebugTextVAlign);

/// Screen placement of debug text
struct ezDebugTextPlacement
{
  using StorageType = ezUInt8;

  enum Enum
  {
    TopLeft,
    TopCenter,
    TopRight,
    BottomLeft,
    BottomCenter,
    BottomRight,

    ENUM_COUNT,

    Default = TopLeft
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezDebugTextPlacement);

struct EZ_RENDERERCORE_DLL ezDebugRendererLine
{
  EZ_DECLARE_POD_TYPE();

  ezDebugRendererLine();
  ezDebugRendererLine(const ezVec3& vStart, const ezVec3& vEnd);
  ezDebugRendererLine(const ezVec3& vStart, const ezVec3& vEnd, const ezColor& color);

  ezVec3 m_start;
  ezVec3 m_end;

  ezColor m_startColor = ezColor::White;
  ezColor m_endColor = ezColor::White;
};

struct EZ_RENDERERCORE_DLL ezDebugRendererTriangle
{
  EZ_DECLARE_POD_TYPE();

  ezDebugRendererTriangle();
  ezDebugRendererTriangle(const ezVec3& v0, const ezVec3& v1, const ezVec3& v2);

  ezVec3 m_position[3];
  ezColor m_color = ezColor::White;
};

struct EZ_RENDERERCORE_DLL ezDebugRendererTexturedTriangle
{
  EZ_DECLARE_POD_TYPE();

  ezVec3 m_position[3];
  ezVec2 m_texcoord[3];
  ezColor m_color = ezColor::White;
};

/// Enables a function to take an ezMat3, ezMat4 or ezTransfrom.
struct ezMatOrTransform
{
  ezMatOrTransform(const ezMat4& mMat4)
    : m_Mat4(mMat4)
  {
  }

  ezMatOrTransform(const ezMat3& mMat3)
  {
    m_Mat4.SetIdentity();
    m_Mat4.SetRotationalPart(mMat3);
  }

  ezMatOrTransform(const ezTransform& transform)
  {
    m_Mat4 = transform.GetAsMat4();
  }

  ezMat4 m_Mat4;
};


/// Draws simple shapes into the scene or view.
///
/// Shapes can be rendered for a single frame, or 'persistent' for a certain duration.
/// The 'context' specifies whether shapes are generally visible in a scene, from all views,
/// or specific to a single view. See the ezDebugRendererContext constructors for what can be implicitly
/// used as a context.
///
/// All functions that render lines take a line thickness. With a thickness of 0 the lines are rendered as one pixel wide
/// hardware lines. With a thickness larger than 0 they are rendered as screen-space quads, see DrawLines() for details.
///
/// This is a namespace, so other code can add functions to it for drawing additional shapes.
/// ezDebugShapes contains the functions that generate the geometry of the built-in shapes.
namespace ezDebugRenderer
{
  /// Renders the given set of lines for one frame.
  ///
  /// \param fLineThickness 0 renders one pixel wide hardware lines. Values larger than 0 render the lines with this width in pixels,
  /// multiplied with ezRenderWorld::GetDisplayScale() and the render scale of the view. Those lines are never thinner than one pixel.
  EZ_RENDERERCORE_DLL void DrawLines(const ezDebugRendererContext& context, ezArrayPtr<const ezDebugRendererLine> lines, const ezColor& color, float fLineThickness = 0.0f, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// Renders lines that are always visible on top of all geometry (no depth test), with distance-based fade-out.
  ///
  /// Useful for showing lines through walls. To get distinct colors for visible vs. occluded portions, additionally
  /// call DrawLines() with the same lines but a different color: the depth-tested DrawLines() result overrides
  /// the always-on-top DrawLinesOccluded() result wherever the lines are not occluded.
  /// Rendering of the occluded layer can be disabled globally via the CVar 'Debug.RenderOccluded'.
  ///
  /// See DrawLines() for fLineThickness. For thick lines the fade-out is computed per line end, not per pixel.
  EZ_RENDERERCORE_DLL void DrawLinesOccluded(const ezDebugRendererContext& context, ezArrayPtr<const ezDebugRendererLine> lines, const ezColor& color, float fLineThickness = 0.0f, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// Renders the given set of lines in 2D (screen-space) for one frame.
  EZ_RENDERERCORE_DLL void Draw2DLines(const ezDebugRendererContext& context, ezArrayPtr<const ezDebugRendererLine> lines, const ezColor& color);

  /// Renders a cross for one frame.
  EZ_RENDERERCORE_DLL void DrawCross(const ezDebugRendererContext& context, const ezVec3& vGlobalPosition, float fLineLength, const ezColor& color, float fLineThickness = 0.0f, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// Renders a wireframe box for one frame.
  EZ_RENDERERCORE_DLL void DrawLineBox(const ezDebugRendererContext& context, const ezBoundingBox& box, const ezColor& color, float fLineThickness = 0.0f, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// Renders the corners of a wireframe box for one frame.
  EZ_RENDERERCORE_DLL void DrawLineBoxCorners(const ezDebugRendererContext& context, const ezBoundingBox& box, float fCornerFraction, const ezColor& color, float fLineThickness = 0.0f, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// Renders a wireframe sphere for one frame.
  EZ_RENDERERCORE_DLL void DrawLineSphere(const ezDebugRendererContext& context, const ezBoundingSphere& sphere, const ezColor& color, float fLineThickness = 0.0f, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// Renders an upright wireframe capsule for one frame.
  EZ_RENDERERCORE_DLL void DrawLineCapsuleZ(const ezDebugRendererContext& context, float fLength, float fRadius, const ezColor& color, float fLineThickness = 0.0f, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// Renders an upright wireframe cylinder for one frame.
  EZ_RENDERERCORE_DLL void DrawLineCylinderZ(const ezDebugRendererContext& context, float fLength, float fRadius, const ezColor& color, float fLineThickness = 0.0f, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// Renders a wireframe frustum for one frame.
  EZ_RENDERERCORE_DLL void DrawLineFrustum(const ezDebugRendererContext& context, const ezFrustum& frustum, const ezColor& color, float fLineThickness = 0.0f, bool bDrawPlaneNormals = false);

  /// Renders a solid box for one frame.
  EZ_RENDERERCORE_DLL void DrawSolidBox(const ezDebugRendererContext& context, const ezBoundingBox& box, const ezColor& color, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// Renders the set of filled triangles for one frame.
  EZ_RENDERERCORE_DLL void DrawSolidTriangles(const ezDebugRendererContext& context, ezArrayPtr<const ezDebugRendererTriangle> triangles, const ezColor& color, bool bTwoSided = false);

  /// Renders the set of textured triangles for one frame.
  EZ_RENDERERCORE_DLL void DrawTexturedTriangles(const ezDebugRendererContext& context, ezArrayPtr<ezDebugRendererTexturedTriangle> triangles, const ezColor& color, const ezTexture2DResourceHandle& hTexture, bool bTwoSided = false);

  /// Renders a filled 2D rectangle in screen-space for one frame.
  /// fDepth is inverse device depth (1 near, 0 far), relevant only when a depth target is bound.
  EZ_RENDERERCORE_DLL void Draw2DRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color);

  /// Renders a textured 2D rectangle in screen-space for one frame.
  /// fDepth is inverse device depth (1 near, 0 far), relevant only when a depth target is bound.
  EZ_RENDERERCORE_DLL void Draw2DRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color, const ezTexture2DResourceHandle& hTexture, ezVec2 vScale = ezVec2(1, 1));

  /// Renders a textured 2D rectangle in screen-space for one frame.
  /// fDepth is inverse device depth (1 near, 0 far), relevant only when a depth target is bound.
  EZ_RENDERERCORE_DLL void Draw2DRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color, ezGALTextureHandle hResourceView, ezVec2 vScale = ezVec2(1, 1));

  /// Renders filled triangles in screen-space for one frame.
  ///
  /// The positions are in pixels, z is the depth. The winding order doesn't matter, the triangles are never culled.
  /// They are drawn in the same batch as 2D rectangles, in the order in which they were added.
  EZ_RENDERERCORE_DLL void Draw2DTriangles(const ezDebugRendererContext& context, ezArrayPtr<const ezDebugRendererTriangle> triangles, const ezColor& color);

  /// Renders a wireframe 2D rectangle in screen-space for one frame.
  /// fDepth is inverse device depth (1 near, 0 far), relevant only when a depth target is bound.
  EZ_RENDERERCORE_DLL void Draw2DLineRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color);

  /// Displays a string in screen-space for one frame.
  ///
  /// The string may contain newlines (\n) for multi-line output.
  /// If horizontal alignment is right, the entire text block is aligned according to the longest line.
  /// If vertical alignment is bottom, the entire text block is aligned there.
  ///
  /// Data can be output as a table, by separating columns with tabs (\t). For example:
  /// "| Col 1\t| Col 2\t| Col 3\t|\n| abc\t| 42\t| 11.23\t|"
  ///
  /// Returns the number of lines that the text was split up into.
  EZ_RENDERERCORE_DLL ezUInt32 Draw2DText(const ezDebugRendererContext& context, const ezFormatString& text, const ezVec2I32& vPositionInPixel, const ezColor& color, ezUInt32 uiSizeInPixel = 16, ezDebugTextHAlign::Enum horizontalAlignment = ezDebugTextHAlign::Left, ezDebugTextVAlign::Enum verticalAlignment = ezDebugTextVAlign::Top);

  /// Draws a piece of text in one of the screen corners.
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
  EZ_RENDERERCORE_DLL void DrawInfoText(const ezDebugRendererContext& context, ezDebugTextPlacement::Enum placement, ezStringView sGroupName, const ezFormatString& text, const ezColor& color = ezColor::White);

  /// Same as DrawInfoText but displays the text for a certain duration.
  EZ_RENDERERCORE_DLL void AddPersistentInfoText(const ezDebugRendererContext& context, ezDebugTextPlacement::Enum placement, const ezFormatString& text, ezTime duration, const ezColor& color = ezColor::White);

  /// Displays a string in 3D space for one frame.
  EZ_RENDERERCORE_DLL ezUInt32 Draw3DText(const ezDebugRendererContext& context, const ezFormatString& text, const ezVec3& vGlobalPosition, const ezColor& color, ezUInt32 uiSizeInPixel = 16, ezDebugTextHAlign::Enum horizontalAlignment = ezDebugTextHAlign::Center, ezDebugTextVAlign::Enum verticalAlignment = ezDebugTextVAlign::Bottom);

  /// Renders a cross at the given location for as many frames until \a duration has passed.
  EZ_RENDERERCORE_DLL void AddPersistentCross(const ezDebugRendererContext& context, float fSize, const ezColor& color, float fLineThickness, ezMatOrTransform mTransform, ezTime duration);

  /// Renders a wireframe sphere at the given location for as many frames until \a duration has passed.
  EZ_RENDERERCORE_DLL void AddPersistentLineSphere(const ezDebugRendererContext& context, float fRadius, const ezColor& color, float fLineThickness, ezMatOrTransform mTransform, ezTime duration);

  /// Renders a wireframe box at the given location for as many frames until \a duration has passed.
  EZ_RENDERERCORE_DLL void AddPersistentLineBox(const ezDebugRendererContext& context, const ezVec3& vHalfSize, const ezColor& color, float fLineThickness, ezMatOrTransform mTransform, ezTime duration);

  /// Renders lines at the given location for as many frames until \a duration has passed.
  ///
  /// Together with the functions in ezDebugShapes, this can be used to render any line shape persistently.
  EZ_RENDERERCORE_DLL void AddPersistentLines(const ezDebugRendererContext& context, ezArrayPtr<const ezDebugRendererLine> lines, const ezColor& color, float fLineThickness, ezMatOrTransform mTransform, ezTime duration);

  /// Renders a cylinder at the given location for as many frames until \a duration has passed.
  ///
  /// See DrawCylinder() for the other parameters.
  EZ_RENDERERCORE_DLL void AddPersistentCylinder(const ezDebugRendererContext& context, float fRadiusStart, float fRadiusEnd, float fLength, const ezColor& solidColor, const ezColor& lineColor, float fLineThickness, ezMatOrTransform mTransform, ezTime duration, bool bCapStart = false, bool bCapEnd = false, ezBasisAxis::Enum cylinderAxis = ezBasisAxis::PositiveX);

  /// Renders a solid 2D cone in a plane with a given angle.
  ///
  /// The rotation goes around the given \a rotationAxis.
  /// An angle of zero is pointing into forwardAxis direction.
  /// Both angles may be negative.
  EZ_RENDERERCORE_DLL void DrawAngle(const ezDebugRendererContext& context, ezAngle startAngle, ezAngle endAngle, const ezColor& solidColor, const ezColor& lineColor, float fLineThickness, ezMatOrTransform mTransform, ezVec3 vForwardAxis = ezVec3::MakeAxisX(), ezVec3 vRotationAxis = ezVec3::MakeAxisZ());

  /// Renders a cone with the tip at the center position, opening up with the given angle.
  EZ_RENDERERCORE_DLL void DrawOpeningCone(const ezDebugRendererContext& context, ezAngle halfAngle, const ezColor& colorInside, const ezColor& colorOutside, ezMatOrTransform mTransform, ezVec3 vForwardAxis = ezVec3::MakeAxisX());

  /// Renders a bent cone with the tip at the center position, pointing into the +X direction opening up with halfAngle1 and halfAngle2 along the Y and Z axis.
  ///
  /// If solidColor.a > 0, the cone is rendered with as solid triangles.
  /// If lineColor.a > 0, the cone is rendered as lines.
  /// Both can be combined.
  EZ_RENDERERCORE_DLL void DrawLimitCone(const ezDebugRendererContext& context, ezAngle halfAngle1, ezAngle halfAngle2, const ezColor& solidColor, const ezColor& lineColor, float fLineThickness, ezMatOrTransform mTransform);

  /// Renders a cylinder starting at the center position, along the given axis.
  ///
  /// If the start and end radius are different, a cone or arrow can be created.
  /// If solidColor.a > 0, the cylinder is rendered with solid triangles, if lineColor.a > 0, it is rendered as lines.
  /// The caps are only rendered as triangles.
  EZ_RENDERERCORE_DLL void DrawCylinder(const ezDebugRendererContext& context, float fRadiusStart, float fRadiusEnd, float fLength, const ezColor& solidColor, const ezColor& lineColor, float fLineThickness, ezMatOrTransform mTransform, bool bCapStart = false, bool bCapEnd = false, ezBasisAxis::Enum cylinderAxis = ezBasisAxis::PositiveX);

  /// Renders a line arrow.
  EZ_RENDERERCORE_DLL void DrawArrow(const ezDebugRendererContext& context, float fSize, const ezColor& color, float fLineThickness, ezMatOrTransform mTransform, ezVec3 vForwardAxis = ezVec3::MakeAxisX());

  /// Returns the width of single glyph in pixels for the given text size
  EZ_RENDERERCORE_DLL float GetTextGlyphWidth(ezUInt32 uiSizeInPixel = 16);

  /// Returns the line height in pixels for the given text size
  EZ_RENDERERCORE_DLL float GetTextLineHeight(ezUInt32 uiSizeInPixel = 16);

  /// Returns the factor that all debug text is scaled by.
  ///
  /// This is the user preference from the CVar 'App.TextScale' multiplied with ezRenderWorld::GetDisplayScale().
  /// All the debug text sizes that are given in pixels are multiplied with this.
  EZ_RENDERERCORE_DLL float GetTextScale();
} // namespace ezDebugRenderer

/// Called by ezDebugWorldRenderPass and ezDebugScreenRenderPass to render the output of ezDebugRenderer. Not meant to be called otherwise.
namespace ezDebugRendererInternal
{
  void RenderWorldSpace(const ezRenderViewContext& renderViewContext);
  void RenderScreenSpace(const ezRenderViewContext& renderViewContext);
} // namespace ezDebugRendererInternal

/// Helper class to expose debug rendering to scripting
class EZ_RENDERERCORE_DLL ezScriptExtensionClass_Debug
{
public:
  /// Returns the resolution of the first main view that it can find.
  static ezVec2 GetResolution();

  static void DrawCross(const ezWorld* pWorld, const ezVec3& vPosition, float fSize, const ezColor& color, const ezTransform& transform);
  static void DrawLineBox(const ezWorld* pWorld, const ezVec3& vPosition, const ezVec3& vHalfExtents, const ezColor& color, const ezTransform& transform);
  static void DrawLineSphere(const ezWorld* pWorld, const ezVec3& vPosition, float fRadius, const ezColor& color, const ezTransform& transform);

  static void DrawLineCapsuleZ(const ezWorld* pWorld, const ezVec3& vPosition, float fLength, float fRadius, const ezColor& color, const ezTransform& transform);
  static void DrawLineCylinderZ(const ezWorld* pWorld, const ezVec3& vPosition, float fLength, float fRadius, const ezColor& color, const ezTransform& transform);

  static void DrawSolidBox(const ezWorld* pWorld, const ezVec3& vPosition, const ezVec3& vHalfExtents, const ezColor& color, const ezTransform& transform);

  /// See ezDebugRenderer::DrawCylinder(). bCaps closes both ends.
  static void DrawCylinder(const ezWorld* pWorld, const ezVec3& vPosition, float fRadiusStart, float fRadiusEnd, float fLength, const ezColor& solidColor, const ezColor& lineColor, const ezTransform& transform, bool bCaps, ezEnum<ezBasisAxis> cylinderAxis);

  static void DrawArrow(const ezWorld* pWorld, const ezVec3& vPosition, float fSize, const ezColor& color, const ezTransform& transform, const ezVec3& vForwardAxis);

  /// The x and y of the position and the size are in pixels. The z of the position is the depth, see ezDebugRenderer::Draw2DRectangle().
  static void Draw2DRectangle(const ezWorld* pWorld, const ezVec3& vPositionInPixel, const ezVec3& vSizeInPixel, const ezColor& color);

  /// The x and y of the position and the size are in pixels. The z of the position is the depth, see ezDebugRenderer::Draw2DLineRectangle().
  static void Draw2DLineRectangle(const ezWorld* pWorld, const ezVec3& vPositionInPixel, const ezVec3& vSizeInPixel, const ezColor& color);

  static void Draw2DText(const ezWorld* pWorld, ezStringView sText, const ezVec3& vPositionInPixel, const ezColor& color, ezUInt32 uiSizeInPixel, ezEnum<ezDebugTextHAlign> horizontalAlignment);
  static void Draw3DText(const ezWorld* pWorld, ezStringView sText, const ezVec3& vPosition, const ezColor& color, ezUInt32 uiSizeInPixel);
  static void DrawInfoText(const ezWorld* pWorld, ezStringView sText, ezEnum<ezDebugTextPlacement> placement, ezStringView sGroupName, const ezColor& color);

  static void AddPersistentCross(const ezWorld* pWorld, const ezVec3& vPosition, float fSize, const ezColor& color, const ezTransform& transform, ezTime duration);
  static void AddPersistentLineBox(const ezWorld* pWorld, const ezVec3& vPosition, const ezVec3& vHalfExtents, const ezColor& color, const ezTransform& transform, ezTime duration);
  static void AddPersistentLineSphere(const ezWorld* pWorld, const ezVec3& vPosition, float fRadius, const ezColor& color, const ezTransform& transform, ezTime duration);
  static void AddPersistentLine(const ezWorld* pWorld, const ezVec3& vStart, const ezVec3& vEnd, const ezColor& startColor, const ezColor& endColor, ezTime duration);

  /// See ezDebugRenderer::AddPersistentCylinder(). bCaps closes both ends.
  static void AddPersistentCylinder(const ezWorld* pWorld, const ezVec3& vPosition, float fRadiusStart, float fRadiusEnd, float fLength, const ezColor& solidColor, const ezColor& lineColor, const ezTransform& transform, ezTime duration, bool bCaps, ezEnum<ezBasisAxis> cylinderAxis);

  static void DrawLine(const ezWorld* pWorld, const ezVec3& vStart, const ezVec3& vEnd, const ezColor& startColor, const ezColor& endColor);

  /// See ezDebugRenderer::DrawLines() for the meaning of fThickness.
  static void DrawThickLine(const ezWorld* pWorld, const ezVec3& vStart, const ezVec3& vEnd, const ezColor& startColor, const ezColor& endColor, float fThickness);

  static void Draw2DLine(const ezWorld* pWorld, const ezVec3& vStart, const ezVec3& vEnd, const ezColor& startColor, const ezColor& endColor);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezScriptExtensionClass_Debug);

#include <RendererCore/Debug/Implementation/DebugRenderer_inl.h>
