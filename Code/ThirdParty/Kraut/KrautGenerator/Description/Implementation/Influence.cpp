#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/Influence.h>

namespace Kraut
{
  Influence::Influence() = default;
  Influence::~Influence() = default;

  const aeVec3 Influence_Position::ComputeInfluence(const aeVec3& vPosition) const
  {
    const aeVec3 vDirection = (m_vPosition - vPosition).GetNormalizedSafe();
    return ComputeInfluenceStrength(vPosition) * vDirection;
  }

  float Influence_Position::ComputeInfluenceStrength(const aeVec3& vPosition) const
  {
    if (m_Falloff == Falloff::None)
    {
      return m_fStrength;
    }

    const float fDistance = (m_vPosition - vPosition).GetLength();

    if (fDistance >= m_fMaxRadius)
      return 0.0f;

    if (m_Falloff == Falloff::Hard)
    {
      if (fDistance <= m_fMaxRadius)
        return m_fStrength;

      return 0.0f;
    }

    const float fLinearFactor = 1.0f - ((fDistance - m_fMinRadius) / (m_fMaxRadius - m_fMinRadius));

    if (m_Falloff == Falloff::Linear)
    {
      if (fDistance <= m_fMinRadius)
        return m_fStrength;

      return m_fStrength * fLinearFactor;
    }

    if (m_Falloff == Falloff::Quadratic)
    {
      if (fDistance <= m_fMinRadius)
        return m_fStrength;

      return m_fStrength * aeMath::Square(fLinearFactor);
    }

    return 0.0f;
  }

  const aeVec3 Influence_Direction::ComputeInfluence(const aeVec3& vPosition) const
  {
    return ComputeInfluenceStrength(vPosition) * m_vDirection;
  }

} // namespace Kraut
