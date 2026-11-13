#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/Frustum.h>
#include <RendererCore/Debug/DebugShapes.h>

namespace
{
  EZ_ALWAYS_INLINE ezVec3 EllipsePoint(const ezVec3& vCenter, const ezVec3& vAxisA, const ezVec3& vAxisB, ezAngle angle)
  {
    return vCenter + vAxisA * ezMath::Cos(angle) + vAxisB * ezMath::Sin(angle);
  }

  /// The tip of the cone is the first point, followed by the 33 points of the (closed) rim.
  void ComputeLimitConePoints(ezVec3* out_pPoints, ezAngle halfAngle1, ezAngle halfAngle2, const ezMat4& mTransform)
  {
    constexpr ezUInt32 NUM_SEGMENTS = 32;

    // copied from NVIDIA's PhysX SDK: Cm::visualizeLimitCone
    const float tanQSwingZ = ezMath::Tan(halfAngle1 / 4.0f);
    const float tanQSwingY = ezMath::Tan(halfAngle2 / 4.0f);

    out_pPoints[0] = mTransform.GetTranslationVector();

    for (ezUInt32 i = 0; i <= NUM_SEGMENTS; i++)
    {
      const ezAngle angle = ezAngle::MakeFromRadian(2 * ezMath::Pi<float>() / NUM_SEGMENTS * i);
      const float c = ezMath::Cos(angle);
      const float s = ezMath::Sin(angle);
      const ezVec3 rv(0, -tanQSwingZ * s, tanQSwingY * c);
      const float rv2 = rv.GetLengthSquared();
      const float r = (1 / (1 + rv2));
      const ezQuat q = ezQuat(0, r * 2 * rv.y, r * 2 * rv.z, r * (1 - rv2));
      out_pPoints[1 + i] = mTransform.TransformPosition(q * ezVec3(1.0f, 0, 0));
    }
  }

  constexpr ezUInt32 LIMIT_CONE_POINTS = 34;
  constexpr ezUInt32 CYLINDER_SEGMENTS = 16;

  /// Returns the number of points along the arc of ezDebugRenderer::DrawAngle(), which are relative to the origin, before the transform.
  ezUInt32 ComputeAnglePoints(ezHybridArray<ezVec3, 80>& out_points, ezAngle startAngle, ezAngle endAngle, const ezVec3& vForwardAxis, const ezVec3& vRotationAxis)
  {
    startAngle.NormalizeRange();
    endAngle.NormalizeRange();

    if (startAngle > endAngle)
      startAngle -= ezAngle::MakeFromDegree(360);

    const ezAngle range = endAngle - startAngle;
    const ezUInt32 uiTesselation = ezMath::Max(1u, (ezUInt32)(range / ezAngle::MakeFromDegree(5)));
    const ezAngle step = range / (float)uiTesselation;

    const ezQuat qStep = ezQuat::MakeFromAxisAndAngle(vRotationAxis, step);
    ezVec3 vDir = ezQuat::MakeFromAxisAndAngle(vRotationAxis, startAngle) * vForwardAxis;

    out_points.PushBack(vDir);
    for (ezUInt32 i = 0; i < uiTesselation; ++i)
    {
      vDir = qStep * vDir;
      out_points.PushBack(vDir);
    }

    return out_points.GetCount();
  }
} // namespace

//////////////////////////////////////////////////////////////////////////
// Building blocks

void ezDebugShapes::AppendLineArc(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, const ezVec3& vCenter, const ezVec3& vAxisA, const ezVec3& vAxisB, ezAngle startAngle, ezAngle endAngle, ezUInt32 uiSegments, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  if (uiSegments == 0 || (vAxisA.IsZero() && vAxisB.IsZero()))
    return;

  const ezMat4& transform = mTransform.m_Mat4;
  const ezAngle step = (endAngle - startAngle) / (float)uiSegments;

  ref_lines.Reserve(ref_lines.GetCount() + uiSegments);

  ezVec3 vPrev = transform.TransformPosition(EllipsePoint(vCenter, vAxisA, vAxisB, startAngle));

  for (ezUInt32 i = 1; i <= uiSegments; ++i)
  {
    const ezVec3 vNext = transform.TransformPosition(EllipsePoint(vCenter, vAxisA, vAxisB, startAngle + step * (float)i));
    ref_lines.PushBack({vPrev, vNext});
    vPrev = vNext;
  }
}

void ezDebugShapes::AppendLineCircle(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, const ezVec3& vCenter, const ezVec3& vAxisA, const ezVec3& vAxisB, ezUInt32 uiSegments /*= 32*/, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  AppendLineArc(ref_lines, vCenter, vAxisA, vAxisB, ezAngle(), ezAngle::MakeFromDegree(360), uiSegments, mTransform);
}

void ezDebugShapes::AppendSolidCircle(ezDynamicArrayBase<ezDebugRendererTriangle>& ref_triangles, const ezVec3& vCenter, const ezVec3& vAxisA, const ezVec3& vAxisB, ezUInt32 uiSegments /*= 32*/, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  if (uiSegments == 0 || (vAxisA.IsZero() && vAxisB.IsZero()))
    return;

  const ezMat4& transform = mTransform.m_Mat4;
  const ezAngle step = ezAngle::MakeFromDegree(360) / (float)uiSegments;
  const ezVec3 vCenterT = transform.TransformPosition(vCenter);

  ref_triangles.Reserve(ref_triangles.GetCount() + uiSegments);

  ezVec3 vPrev = transform.TransformPosition(vCenter + vAxisA);

  for (ezUInt32 i = 1; i <= uiSegments; ++i)
  {
    const ezVec3 vNext = transform.TransformPosition(EllipsePoint(vCenter, vAxisA, vAxisB, step * (float)i));
    ref_triangles.PushBack({vCenterT, vPrev, vNext});
    vPrev = vNext;
  }
}

//////////////////////////////////////////////////////////////////////////
// Line shapes

void ezDebugShapes::AppendLineCross(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, const ezVec3& vCenter, float fLineLength, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  if (fLineLength <= 0.0f)
    return;

  const ezMat4& transform = mTransform.m_Mat4;

  const float fHalfLineLength = fLineLength * 0.5f;
  const ezVec3 xAxis = ezVec3::MakeAxisX() * fHalfLineLength;
  const ezVec3 yAxis = ezVec3::MakeAxisY() * fHalfLineLength;
  const ezVec3 zAxis = ezVec3::MakeAxisZ() * fHalfLineLength;

  ref_lines.PushBack({transform.TransformPosition(vCenter - xAxis), transform.TransformPosition(vCenter + xAxis)});
  ref_lines.PushBack({transform.TransformPosition(vCenter - yAxis), transform.TransformPosition(vCenter + yAxis)});
  ref_lines.PushBack({transform.TransformPosition(vCenter - zAxis), transform.TransformPosition(vCenter + zAxis)});
}

void ezDebugShapes::AppendLineBox(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, const ezBoundingBox& box, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  AppendLineBoxCorners(ref_lines, box, 1.0f, mTransform);
}

void ezDebugShapes::AppendLineBoxCorners(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, const ezBoundingBox& box, float fCornerFraction, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  const ezMat4& transform = mTransform.m_Mat4;

  ezVec3 corners[8];
  box.GetCorners(corners);

  for (ezUInt32 i = 0; i < 8; ++i)
  {
    corners[i] = transform * corners[i];
  }

  ezVec3 edgeEnds[12];
  edgeEnds[0] = corners[1];  // 0 -> 1
  edgeEnds[1] = corners[3];  // 1 -> 3
  edgeEnds[2] = corners[0];  // 2 -> 0
  edgeEnds[3] = corners[2];  // 3 -> 2
  edgeEnds[4] = corners[5];  // 4 -> 5
  edgeEnds[5] = corners[7];  // 5 -> 7
  edgeEnds[6] = corners[4];  // 6 -> 4
  edgeEnds[7] = corners[6];  // 7 -> 6
  edgeEnds[8] = corners[4];  // 0 -> 4
  edgeEnds[9] = corners[5];  // 1 -> 5
  edgeEnds[10] = corners[6]; // 2 -> 6
  edgeEnds[11] = corners[7]; // 3 -> 7

  fCornerFraction = ezMath::Clamp(fCornerFraction, 0.0f, 1.0f);

  if (fCornerFraction >= 1.0f)
  {
    for (ezUInt32 i = 0; i < 12; ++i)
    {
      ref_lines.PushBack({corners[i % 8], edgeEnds[i]});
    }

    return;
  }

  const float fHalfFraction = fCornerFraction * 0.5f;

  for (ezUInt32 i = 0; i < 12; ++i)
  {
    const ezVec3 edgeStart = corners[i % 8];
    const ezVec3 edgeEnd = edgeEnds[i];
    const ezVec3 edgeDir = edgeEnd - edgeStart;

    ref_lines.PushBack({edgeStart, edgeStart + edgeDir * fHalfFraction});
    ref_lines.PushBack({edgeEnd, edgeEnd - edgeDir * fHalfFraction});
  }
}

void ezDebugShapes::AppendLineSphere(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, const ezBoundingSphere& sphere, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  const ezVec3 c = sphere.m_vCenter;
  const float r = sphere.m_fRadius;

  AppendLineCircle(ref_lines, c, ezVec3(0, r, 0), ezVec3(0, 0, r), 32, mTransform);
  AppendLineCircle(ref_lines, c, ezVec3(r, 0, 0), ezVec3(0, 0, r), 32, mTransform);
  AppendLineCircle(ref_lines, c, ezVec3(r, 0, 0), ezVec3(0, r, 0), 32, mTransform);
}

void ezDebugShapes::AppendLineCylinderZ(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, float fLength, float fRadius, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  const ezMat4& transform = mTransform.m_Mat4;
  const float fOffsetZ = fLength * 0.5f;
  const ezVec3 vAxisA(fRadius, 0, 0);
  const ezVec3 vAxisB(0, fRadius, 0);

  AppendLineCircle(ref_lines, ezVec3(0, 0, fOffsetZ), vAxisA, vAxisB, 32, transform);

  if (fLength == 0.0f)
    return;

  AppendLineCircle(ref_lines, ezVec3(0, 0, -fOffsetZ), vAxisA, vAxisB, 32, transform);

  for (const ezVec3& vSide : {vAxisA, -vAxisA, vAxisB, -vAxisB})
  {
    ref_lines.PushBack({transform * (vSide + ezVec3(0, 0, fOffsetZ)), transform * (vSide - ezVec3(0, 0, fOffsetZ))});
  }
}

void ezDebugShapes::AppendLineCapsuleZ(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, float fLength, float fRadius, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  AppendLineCylinderZ(ref_lines, fLength, fRadius, mTransform);

  const ezVec3 vTop(0, 0, fLength * 0.5f);
  const ezVec3 vX(fRadius, 0, 0);
  const ezVec3 vY(0, fRadius, 0);
  const ezVec3 vZ(0, 0, fRadius);
  const ezAngle halfCircle = ezAngle::MakeFromDegree(180);

  AppendLineArc(ref_lines, vTop, vY, vZ, ezAngle(), halfCircle, 16, mTransform);
  AppendLineArc(ref_lines, vTop, vX, vZ, ezAngle(), halfCircle, 16, mTransform);
  AppendLineArc(ref_lines, -vTop, vY, -vZ, ezAngle(), halfCircle, 16, mTransform);
  AppendLineArc(ref_lines, -vTop, vX, -vZ, ezAngle(), halfCircle, 16, mTransform);
}

void ezDebugShapes::AppendLineFrustum(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, const ezFrustum& frustum)
{
  ezVec3 c[8];
  if (frustum.ComputeCornerPoints(c).Failed())
    return;

  using C = ezFrustum::FrustumCorner;

  ref_lines.PushBack({c[C::NearBottomLeft], c[C::FarBottomLeft]});
  ref_lines.PushBack({c[C::NearBottomRight], c[C::FarBottomRight]});
  ref_lines.PushBack({c[C::NearTopLeft], c[C::FarTopLeft]});
  ref_lines.PushBack({c[C::NearTopRight], c[C::FarTopRight]});

  ref_lines.PushBack({c[C::NearBottomLeft], c[C::NearBottomRight]});
  ref_lines.PushBack({c[C::NearBottomRight], c[C::NearTopRight]});
  ref_lines.PushBack({c[C::NearTopRight], c[C::NearTopLeft]});
  ref_lines.PushBack({c[C::NearTopLeft], c[C::NearBottomLeft]});

  ref_lines.PushBack({c[C::FarBottomLeft], c[C::FarBottomRight]});
  ref_lines.PushBack({c[C::FarBottomRight], c[C::FarTopRight]});
  ref_lines.PushBack({c[C::FarTopRight], c[C::FarTopLeft]});
  ref_lines.PushBack({c[C::FarTopLeft], c[C::FarBottomLeft]});
}

void ezDebugShapes::AppendLineArrow(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, float fSize, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/, ezVec3 vForwardAxis /*= ezVec3::MakeAxisX()*/)
{
  const ezMat4& transform = mTransform.m_Mat4;

  vForwardAxis.Normalize();
  const ezVec3 right = vForwardAxis.GetOrthogonalVector().GetNormalized();
  const ezVec3 up = vForwardAxis.CrossRH(right).GetNormalized();
  const ezVec3 endPoint = transform * (vForwardAxis * fSize);
  const ezVec3 endPoint2 = vForwardAxis * fSize * 0.9f;
  const float tipSize = fSize * 0.1f;

  const ezVec3 tip[4] = {
    transform * (endPoint2 + right * tipSize),
    transform * (endPoint2 + up * tipSize),
    transform * (endPoint2 - right * tipSize),
    transform * (endPoint2 - up * tipSize),
  };

  ref_lines.PushBack({transform.GetTranslationVector(), endPoint});

  for (ezUInt32 i = 0; i < 4; ++i)
  {
    ref_lines.PushBack({endPoint, tip[i]});
  }

  for (ezUInt32 i = 0; i < 4; ++i)
  {
    ref_lines.PushBack({tip[i], tip[(i + 1) % 4]});
  }
}

void ezDebugShapes::AppendLineAngle(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, ezAngle startAngle, ezAngle endAngle, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/, ezVec3 vForwardAxis /*= ezVec3::MakeAxisX()*/, ezVec3 vRotationAxis /*= ezVec3::MakeAxisZ()*/)
{
  const ezMat4& transform = mTransform.m_Mat4;
  const ezVec3 vCenter = transform.GetTranslationVector();

  ezHybridArray<ezVec3, 80> points;
  ComputeAnglePoints(points, startAngle, endAngle, vForwardAxis, vRotationAxis);

  ezVec3 vPrev = transform.TransformPosition(points[0]);
  ref_lines.PushBack({vCenter, vPrev});

  for (ezUInt32 i = 1; i < points.GetCount(); ++i)
  {
    const ezVec3 vNext = transform.TransformPosition(points[i]);
    ref_lines.PushBack({vCenter, vNext});
    ref_lines.PushBack({vPrev, vNext});
    vPrev = vNext;
  }
}

void ezDebugShapes::AppendLineLimitCone(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, ezAngle halfAngle1, ezAngle halfAngle2, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  ezVec3 points[LIMIT_CONE_POINTS];
  ComputeLimitConePoints(points, halfAngle1, halfAngle2, mTransform.m_Mat4);

  for (ezUInt32 i = 1; i < LIMIT_CONE_POINTS; ++i)
  {
    ref_lines.PushBack({points[0], points[i]});

    if (i > 1)
    {
      ref_lines.PushBack({points[i - 1], points[i]});
    }
  }
}

void ezDebugShapes::AppendLineCylinder(ezDynamicArrayBase<ezDebugRendererLine>& ref_lines, float fRadiusStart, float fRadiusEnd, float fLength, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/, ezBasisAxis::Enum cylinderAxis /*= ezBasisAxis::PositiveX*/)
{
  const ezQuat tilt = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveX, cylinderAxis);
  const ezMat4 transform = mTransform.m_Mat4 * tilt.GetAsMat4();

  const ezVec3 vY = ezVec3::MakeAxisY();
  const ezVec3 vZ = ezVec3::MakeAxisZ();
  const ezVec3 vEnd(fLength, 0, 0);

  AppendLineCircle(ref_lines, ezVec3::MakeZero(), vY * fRadiusStart, vZ * fRadiusStart, CYLINDER_SEGMENTS, transform);

  if (fLength == 0.0f && fRadiusStart == fRadiusEnd)
    return;

  AppendLineCircle(ref_lines, vEnd, vY * fRadiusEnd, vZ * fRadiusEnd, CYLINDER_SEGMENTS, transform);

  const ezAngle step = ezAngle::MakeFromDegree(360) / (float)CYLINDER_SEGMENTS;

  for (ezUInt32 i = 0; i < CYLINDER_SEGMENTS; ++i)
  {
    const ezAngle angle = step * (float)i;
    const ezVec3 vStartPos = EllipsePoint(ezVec3::MakeZero(), vY * fRadiusStart, vZ * fRadiusStart, angle);
    const ezVec3 vEndPos = EllipsePoint(vEnd, vY * fRadiusEnd, vZ * fRadiusEnd, angle);
    ref_lines.PushBack({transform.TransformPosition(vStartPos), transform.TransformPosition(vEndPos)});
  }
}

//////////////////////////////////////////////////////////////////////////
// Solid shapes

void ezDebugShapes::AppendSolidAngle(ezDynamicArrayBase<ezDebugRendererTriangle>& ref_triangles, ezAngle startAngle, ezAngle endAngle, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/, ezVec3 vForwardAxis /*= ezVec3::MakeAxisX()*/, ezVec3 vRotationAxis /*= ezVec3::MakeAxisZ()*/)
{
  const ezMat4& transform = mTransform.m_Mat4;
  const ezVec3 vCenter = transform.GetTranslationVector();

  ezHybridArray<ezVec3, 80> points;
  ComputeAnglePoints(points, startAngle, endAngle, vForwardAxis, vRotationAxis);

  for (ezUInt32 i = 1; i < points.GetCount(); ++i)
  {
    ref_triangles.PushBack({vCenter, transform.TransformPosition(points[i - 1]), transform.TransformPosition(points[i])});
  }
}

void ezDebugShapes::AppendSolidLimitCone(ezDynamicArrayBase<ezDebugRendererTriangle>& ref_triangles, ezAngle halfAngle1, ezAngle halfAngle2, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  ezVec3 points[LIMIT_CONE_POINTS];
  ComputeLimitConePoints(points, halfAngle1, halfAngle2, mTransform.m_Mat4);

  for (ezUInt32 i = 2; i < LIMIT_CONE_POINTS; ++i)
  {
    ref_triangles.PushBack({points[0], points[i], points[i - 1]});
  }
}

void ezDebugShapes::AppendSolidCylinder(ezDynamicArrayBase<ezDebugRendererTriangle>& ref_triangles, float fRadiusStart, float fRadiusEnd, float fLength, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/, bool bCapStart /*= false*/, bool bCapEnd /*= false*/, ezBasisAxis::Enum cylinderAxis /*= ezBasisAxis::PositiveX*/)
{
  const ezQuat tilt = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveX, cylinderAxis);
  const ezMat4 transform = mTransform.m_Mat4 * tilt.GetAsMat4();

  const ezVec3 vY = ezVec3::MakeAxisY();
  const ezVec3 vZ = ezVec3::MakeAxisZ();
  const ezVec3 vEnd(fLength, 0, 0);

  const ezAngle step = ezAngle::MakeFromDegree(360) / (float)CYLINDER_SEGMENTS;

  ezVec3 vCurStart = transform.TransformPosition(vY * fRadiusStart);
  ezVec3 vCurEnd = transform.TransformPosition(vEnd + vY * fRadiusEnd);

  for (ezUInt32 i = 1; i <= CYLINDER_SEGMENTS; ++i)
  {
    const ezAngle angle = step * (float)i;
    const ezVec3 vNextStart = transform.TransformPosition(EllipsePoint(ezVec3::MakeZero(), vY * fRadiusStart, vZ * fRadiusStart, angle));
    const ezVec3 vNextEnd = transform.TransformPosition(EllipsePoint(vEnd, vY * fRadiusEnd, vZ * fRadiusEnd, angle));

    ref_triangles.PushBack({vCurStart, vNextStart, vNextEnd});
    ref_triangles.PushBack({vCurStart, vNextEnd, vCurEnd});

    vCurStart = vNextStart;
    vCurEnd = vNextEnd;
  }

  // the start cap faces -X, the end cap +X
  if (bCapStart)
    AppendSolidCircle(ref_triangles, ezVec3::MakeZero(), vZ * fRadiusStart, vY * fRadiusStart, CYLINDER_SEGMENTS, transform);

  if (bCapEnd)
    AppendSolidCircle(ref_triangles, vEnd, vY * fRadiusEnd, vZ * fRadiusEnd, CYLINDER_SEGMENTS, transform);
}
