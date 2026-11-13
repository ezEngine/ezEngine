#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <RendererCore/Debug/DebugRenderer.h>

/// Functions that generate the lines and triangles of the shapes that ezDebugRenderer draws.
///
/// All functions append to the given array and apply the given transform to the generated positions.
/// The generated lines and triangles are white, ezDebugRenderer multiplies their color with the color that is passed to it.
///
/// Use these to render a shape persistently (ezDebugRenderer::AddPersistentLines()), to combine several shapes into a single
/// draw call, or to build additional shapes for ezDebugRenderer. The arc and circle functions are the building blocks of the round shapes.
///
/// The AppendLine...() functions generate the wireframe of a shape, the AppendSolid...() functions its surface.
/// Solid triangles are front facing when seen from outside the shape, i.e. (v1 - v0).CrossRH(v2 - v0) points outwards.
/// Flat shapes (circles, angles) are only generated with one winding, render them with bTwoSided to see them from both sides.
namespace ezDebugShapes
{
  /// \name Building blocks
  /// @{

  /// An arc of the ellipse vCenter + vAxisA * cos(a) + vAxisB * sin(a), from startAngle to endAngle.
  ///
  /// With perpendicular axes of the same length this is a circular arc with that radius.
  /// Nothing is added if uiSegments is 0 or both axes are zero.
  EZ_RENDERERCORE_DLL void AppendLineArc(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, const ezVec3& vCenter, const ezVec3& vAxisA, const ezVec3& vAxisB, ezAngle startAngle, ezAngle endAngle, ezUInt32 uiSegments, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// A closed circle (or ellipse), see AppendLineArc().
  EZ_RENDERERCORE_DLL void AppendLineCircle(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, const ezVec3& vCenter, const ezVec3& vAxisA, const ezVec3& vAxisB, ezUInt32 uiSegments = 32, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// A filled circle (or ellipse), see AppendLineArc(). The front side faces into the direction of vAxisA.CrossRH(vAxisB).
  EZ_RENDERERCORE_DLL void AppendSolidCircle(ezDynamicArrayBase<ezDebugRendererTriangle>& ref_triangles, const ezVec3& vCenter, const ezVec3& vAxisA, const ezVec3& vAxisB, ezUInt32 uiSegments = 32, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// @}
  /// \name Line shapes
  /// @{

  /// Three lines of length \a fLineLength along the axes, centered at \a vCenter.
  EZ_RENDERERCORE_DLL void AppendLineCross(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, const ezVec3& vCenter, float fLineLength, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// The 12 edges of a box.
  EZ_RENDERERCORE_DLL void AppendLineBox(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, const ezBoundingBox& box, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// Only the corners of the 12 edges of a box. \a fCornerFraction is clamped to [0; 1], 1 gives the full edges.
  EZ_RENDERERCORE_DLL void AppendLineBoxCorners(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, const ezBoundingBox& box, float fCornerFraction, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// Three circles around the main axes.
  EZ_RENDERERCORE_DLL void AppendLineSphere(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, const ezBoundingSphere& sphere, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// A capsule along the Z axis, centered at the origin. \a fLength is the length of the cylindrical part.
  EZ_RENDERERCORE_DLL void AppendLineCapsuleZ(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, float fLength, float fRadius, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// A cylinder along the Z axis, centered at the origin. With a length of 0 only a single circle is added.
  EZ_RENDERERCORE_DLL void AppendLineCylinderZ(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, float fLength, float fRadius, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// The 12 edges of a frustum. Nothing is added if the corners of the frustum can't be computed.
  EZ_RENDERERCORE_DLL void AppendLineFrustum(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, const ezFrustum& frustum);

  /// An arrow from the origin to vForwardAxis * fSize, with a small pyramid as the tip.
  EZ_RENDERERCORE_DLL void AppendLineArrow(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, float fSize, ezMatOrTransform mTransform = ezMat4::MakeIdentity(), ezVec3 vForwardAxis = ezVec3::MakeAxisX());

  /// The outline of the shape of ezDebugRenderer::DrawAngle(): the arc and lines from the origin to the points on the arc.
  EZ_RENDERERCORE_DLL void AppendLineAngle(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, ezAngle startAngle, ezAngle endAngle, ezMatOrTransform mTransform = ezMat4::MakeIdentity(), ezVec3 vForwardAxis = ezVec3::MakeAxisX(), ezVec3 vRotationAxis = ezVec3::MakeAxisZ());

  /// See ezDebugRenderer::DrawLimitCone().
  EZ_RENDERERCORE_DLL void AppendLineLimitCone(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, ezAngle halfAngle1, ezAngle halfAngle2, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// A cylinder or cone starting at the origin, along the given axis, see ezDebugRenderer::DrawCylinder().
  ///
  /// A circle with radius 0 is skipped. If both circles are at the same place, only one of them is added.
  EZ_RENDERERCORE_DLL void AppendLineCylinder(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, float fRadiusStart, float fRadiusEnd, float fLength, ezMatOrTransform mTransform = ezMat4::MakeIdentity(), ezBasisAxis::Enum cylinderAxis = ezBasisAxis::PositiveX);

  /// @}
  /// \name Solid shapes
  /// @{

  /// The filled shape of ezDebugRenderer::DrawAngle(). Its front side faces into the direction of vRotationAxis.
  EZ_RENDERERCORE_DLL void AppendSolidAngle(ezDynamicArrayBase<ezDebugRendererTriangle>& ref_triangles, ezAngle startAngle, ezAngle endAngle, ezMatOrTransform mTransform = ezMat4::MakeIdentity(), ezVec3 vForwardAxis = ezVec3::MakeAxisX(), ezVec3 vRotationAxis = ezVec3::MakeAxisZ());

  /// See ezDebugRenderer::DrawLimitCone(). The front side is the outside of the cone.
  EZ_RENDERERCORE_DLL void AppendSolidLimitCone(ezDynamicArrayBase<ezDebugRendererTriangle>& ref_triangles, ezAngle halfAngle1, ezAngle halfAngle2, ezMatOrTransform mTransform = ezMat4::MakeIdentity());

  /// The surface of AppendLineCylinder(), optionally closed with caps at the start and the end.
  EZ_RENDERERCORE_DLL void AppendSolidCylinder(ezDynamicArrayBase<ezDebugRendererTriangle>& ref_triangles, float fRadiusStart, float fRadiusEnd, float fLength, ezMatOrTransform mTransform = ezMat4::MakeIdentity(), bool bCapStart = false, bool bCapEnd = false, ezBasisAxis::Enum cylinderAxis = ezBasisAxis::PositiveX);

  /// @}
} // namespace ezDebugShapes
