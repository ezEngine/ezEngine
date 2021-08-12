#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Physics/ClothSheetSimulator.h>

void ezClothSimulator::SimulateCloth(const ezTime& tDiff)
{
  m_leftOverTimeStep += tDiff;

  constexpr ezTime tStep = ezTime::Seconds(1.0 / 60.0);
  constexpr float tStepSqr = static_cast<float>(tStep.GetSeconds() * tStep.GetSeconds());

  while (m_leftOverTimeStep >= tStep)
  {
    SimulateStep(tStepSqr, 32, m_vSegmentLength.x);

    m_leftOverTimeStep -= tStep;
  }
}

void ezClothSimulator::SimulateStep(const float tDiffSqr, ezUInt32 uiMaxIterations, double fAllowedError)
{
  if (m_Nodes.GetCount() < 4)
    return;

  UpdateNodePositions(tDiffSqr);

  // repeatedly apply the distance constraint, until the overall error is low enough
  for (ezUInt32 i = 0; i < uiMaxIterations; ++i)
  {
    const double fError = EnforceDistanceConstraint();

    if (fError < fAllowedError)
      return;
  }
}

double ezClothSimulator::EnforceDistanceConstraint()
{
  double fError = 0.0;

  for (ezUInt32 y = 0; y < m_uiHeight; ++y)
  {
    for (ezUInt32 x = 0; x < m_uiWidth; ++x)
    {
      const ezUInt32 idx = (y * m_uiWidth) + x;

      auto& n = m_Nodes[idx];

      if (n.m_bFixed)
        continue;

      const ezVec3 posThis = n.m_vPosition;

      if (x > 0)
      {
        const ezVec3 pos = m_Nodes[idx - 1].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, ezVec3(-1, 0, 0), fError, m_vSegmentLength.x);
      }

      if (x + 1 < m_uiWidth)
      {
        const ezVec3 pos = m_Nodes[idx + 1].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, ezVec3(1, 0, 0), fError, m_vSegmentLength.x);
      }

      if (y > 0)
      {
        const ezVec3 pos = m_Nodes[idx - m_uiWidth].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, ezVec3(0, -1, 0), fError, m_vSegmentLength.y);
      }

      if (y + 1 < m_uiHeight)
      {
        const ezVec3 pos = m_Nodes[idx + m_uiWidth].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, ezVec3(0, 1, 0), fError, m_vSegmentLength.y);
      }
    }
  }

  return fError;
}

ezVec3 ezClothSimulator::MoveTowards(const ezVec3& posThis, const ezVec3& posNext, float factor, const ezVec3& fallbackDir, double& inout_fError, float fSegLen)
{
  ezVec3 vDir = (posNext - posThis);
  float fLen = vDir.GetLength();

  if (ezMath::IsZero(fLen, 0.001f))
  {
    vDir = fallbackDir;
    fLen = 1;
  }

  vDir /= fLen;
  fLen -= fSegLen;

  const float fLocalError = fLen * factor;

  vDir *= fLocalError;

  // keep track of how much the rope had to be moved to fulfill the constraint
  inout_fError += ezMath::Abs(fLocalError);

  return vDir;
}

void ezClothSimulator::UpdateNodePositions(const float tDiffSqr)
{
  for (auto& n : m_Nodes)
  {
    if (n.m_bFixed)
    {
      n.m_vPreviousPosition = n.m_vPosition;
    }
    else
    {
      // this (simple) logic is the so called 'Verlet integration' (+ damping)

      const ezVec3 previousPos = n.m_vPosition;

      const ezVec3 vel = (n.m_vPosition - n.m_vPreviousPosition) * m_fDampingFactor;

      // instead of using a single global acceleration, this could also use individual accelerations per node
      // this would be needed to affect the rope more localized
      n.m_vPosition += vel + tDiffSqr * m_vAcceleration;
      n.m_vPreviousPosition = previousPos;
    }
  }
}

bool ezClothSimulator::HasEquilibrium(float fAllowedMovement) const
{
  const float fErrorSqr = ezMath::Square(fAllowedMovement);

  for (const auto& n : m_Nodes)
  {
    if ((n.m_vPosition - n.m_vPreviousPosition).GetLengthSquared() > fErrorSqr)
    {
      return false;
    }
  }

  return true;
}
