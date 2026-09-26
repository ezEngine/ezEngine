#include <GameEngine/GameEnginePCH.h>

#include <Core/Graphics/Geometry.h>
#include <GameEngine/Gameplay/GreyBoxComponent.h>

ezResult ezGreyBoxComponent::BuildConeGeometry(ezGeometry& out_geometry) const
{
  const ezVec3 size = ezVec3(m_fSizeNegX + m_fSizePosX, m_fSizeNegY + m_fSizePosY, m_fSizeNegZ + m_fSizePosZ).Abs();
  const ezVec3 center = ezVec3(m_fSizePosX - m_fSizeNegX, m_fSizePosY - m_fSizeNegY, m_fSizePosZ - m_fSizeNegZ) * 0.5f;
  if (!size.IsValid() || !center.IsValid() || size.x <= 0 || size.y <= 0 || size.z <= 0 ||
      !ezMath::IsFinite(m_fBaseRadiusScale) || !ezMath::IsFinite(m_fTopRadiusScale) || !ezMath::IsFinite(m_fProfileCurve))
    return EZ_FAILURE;

  const ezUInt32 sides = ezMath::Clamp(m_uiSides, 3u, 128u);
  const ezUInt32 segments = ezMath::Clamp(m_uiHeightSegments, 1u, 64u);
  const float bottom = ezMath::Clamp(m_fBaseRadiusScale, 0.0f, 4.0f);
  const float top = ezMath::Clamp(m_fTopRadiusScale, 0.0f, 4.0f);
  const float curve = ezMath::Clamp(m_fProfileCurve, -0.95f, 1.0f);
  if (bottom == 0 && top == 0)
    return EZ_FAILURE;

  // Four sides make an axis-aligned square pyramid that fills the requested X/Y extents.
  const float polygonScale = sides == 4 ? ezMath::Sqrt(2.0f) : 1.0f;
  const float angleOffset = sides == 4 ? ezMath::Pi<float>() * 0.25f : 0.0f;
  const float rx = size.x * 0.5f * polygonScale;
  const float ry = size.y * 0.5f * polygonScale;
  auto radius = [bottom, top, curve](float t)
  {
    if (t == 0)
      return bottom;
    if (t == 1)
      return top;
    return ezMath::Lerp(bottom, top, t) * (1.0f + curve * ezMath::Sin(ezAngle::MakeFromRadian(ezMath::Pi<float>() * t)));
  };
  auto angle = [sides, angleOffset](ezUInt32 i)
  {
    return ezAngle::MakeFromRadian(angleOffset + (2.0f * ezMath::Pi<float>()) * static_cast<float>(i % sides) / sides);
  };
  auto position = [&](ezUInt32 i, float t)
  {
    const float r = radius(t);
    return center + ezVec3(rx * r * ezMath::Cos(angle(i)), ry * r * ezMath::Sin(angle(i)), (t - 0.5f) * size.z);
  };
  auto normal = [&](ezUInt32 i, float t)
  {
    const ezAngle a = angle(i);
    const ezAngle h = ezAngle::MakeFromRadian(ezMath::Pi<float>() * t);
    const float dr = (top - bottom) * (1.0f + curve * ezMath::Sin(h)) +
                     ezMath::Lerp(bottom, top, t) * curve * ezMath::Pi<float>() * ezMath::Cos(h);
    ezVec3 n(ry * ezMath::Cos(a), rx * ezMath::Sin(a), -rx * ry * dr / size.z);
    n.NormalizeIfNotZero(ezVec3::MakeAxisZ()).IgnoreResult();
    return n;
  };

  for (ezUInt32 j = 0; j < segments; ++j)
  {
    const float t0 = static_cast<float>(j) / segments;
    const float t1 = static_cast<float>(j + 1) / segments;
    for (ezUInt32 i = 0; i < sides; ++i)
    {
      ezVec3 points[4] = {position(i, t0), position(i + 1, t0), position(i + 1, t1), position(i, t1)};
      ezUInt32 sample[4] = {0, 1, 2, 3};
      ezUInt32 count = 4;
      // Use a triangle at an apex rather than a zero-area quad.
      if (radius(t1) == 0)
        count = 3;
      else if (radius(t0) == 0)
      {
        sample[1] = 2;
        sample[2] = 3;
        count = 3;
      }
      ezVec3 faceNormal = (points[sample[1]] - points[0]).CrossRH(points[sample[2]] - points[0]);
      if (faceNormal.NormalizeIfNotZero(ezVec3::MakeAxisZ()).Failed())
        continue;
      ezUInt32 polygon[4];
      for (ezUInt32 k = 0; k < count; ++k)
      {
        const ezUInt32 s = sample[k];
        const ezUInt32 a = i + ((s == 1 || s == 2) ? 1 : 0);
        const float t = s >= 2 ? t1 : t0;
        polygon[k] = out_geometry.AddVertex(points[s], m_bSmoothShading ? normal(a, t) : faceNormal,
          ezVec2(static_cast<float>(a) / sides, t));
      }
      out_geometry.AddPolygon(ezMakeArrayPtr(polygon, count), false);
    }
  }

  for (ezUInt32 cap = 0; cap < 2; ++cap)
  {
    const float t = static_cast<float>(cap);
    if (radius(t) == 0)
      continue;
    ezDynamicArray<ezUInt32> polygon;
    for (ezUInt32 i = 0; i < sides; ++i)
    {
      const ezUInt32 index = cap == 0 ? sides - 1 - i : i;
      const ezVec3 p = position(index, t);
      polygon.PushBack(out_geometry.AddVertex(p, cap == 0 ? -ezVec3::MakeAxisZ() : ezVec3::MakeAxisZ(),
        ezVec2((p.x - center.x) / size.x + 0.5f, (p.y - center.y) / size.y + 0.5f)));
    }
    out_geometry.AddPolygon(polygon, false);
  }
  return EZ_SUCCESS;
}
