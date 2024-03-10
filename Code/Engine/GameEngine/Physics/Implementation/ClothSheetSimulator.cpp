#include <GameEngine/GameEnginePCH.h>

#include <Foundation/SimdMath/SimdConversion.h>
#include <GameEngine/Physics/ClothSheetSimulator.h>

void ezClothSimulator::SimulateCloth(const ezTime& diff)
{
  m_LeftOverTimeStep += diff;

  constexpr ezTime tStep = ezTime::MakeFromSeconds(1.0 / 60.0);
  const ezSimdFloat tStepSqr = static_cast<float>(tStep.GetSeconds() * tStep.GetSeconds());

  while (m_LeftOverTimeStep >= tStep)
  {
    SimulateStep(tStepSqr, 32, m_vSegmentLength.x);

    m_LeftOverTimeStep -= tStep;
  }
}

void ezClothSimulator::SimulateStep(const ezSimdFloat fDiffSqr, ezUInt32 uiMaxIterations, ezSimdFloat fAllowedError)
{
  if (m_Nodes.GetCount() < 4)
    return;

  UpdateNodePositions(fDiffSqr);

  // repeatedly apply the distance constraint, until the overall error is low enough
  for (ezUInt32 i = 0; i < uiMaxIterations; ++i)
  {
    const ezSimdFloat fError = EnforceDistanceConstraint();

    if (fError < fAllowedError)
      return;
  }
}

ezSimdFloat ezClothSimulator::EnforceDistanceConstraint()
{
  ezSimdFloat fError = ezSimdFloat::MakeZero();

  for (ezUInt32 y = 0; y < m_uiHeight; ++y)
  {
    for (ezUInt32 x = 0; x < m_uiWidth; ++x)
    {
      const ezUInt32 idx = (y * m_uiWidth) + x;

      auto& n = m_Nodes[idx];

      if (n.m_bFixed)
        continue;

      const ezSimdVec4f posThis = n.m_vPosition;

      if (x > 0)
      {
        const ezSimdVec4f pos = m_Nodes[idx - 1].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, ezSimdVec4f(-1, 0, 0), fError, m_vSegmentLength.x);
      }

      if (x + 1 < m_uiWidth)
      {
        const ezSimdVec4f pos = m_Nodes[idx + 1].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, ezSimdVec4f(1, 0, 0), fError, m_vSegmentLength.x);
      }

      if (y > 0)
      {
        const ezSimdVec4f pos = m_Nodes[idx - m_uiWidth].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, ezSimdVec4f(0, -1, 0), fError, m_vSegmentLength.y);
      }

      if (y + 1 < m_uiHeight)
      {
        const ezSimdVec4f pos = m_Nodes[idx + m_uiWidth].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, ezSimdVec4f(0, 1, 0), fError, m_vSegmentLength.y);
      }
    }
  }

  return fError;
}

ezSimdVec4f ezClothSimulator::MoveTowards(const ezSimdVec4f posThis, const ezSimdVec4f posNext, ezSimdFloat factor, const ezSimdVec4f fallbackDir, ezSimdFloat& inout_fError, ezSimdFloat fSegLen)
{
  ezSimdVec4f vDir = (posNext - posThis);
  ezSimdFloat fLen = vDir.GetLength<3>();

  if (fLen.IsEqual(ezSimdFloat::MakeZero(), 0.001f))
  {
    vDir = fallbackDir;
    fLen = 1;
  }

  vDir /= fLen;
  fLen -= fSegLen;

  const ezSimdFloat fLocalError = fLen * factor;

  vDir *= fLocalError;

  // keep track of how much the rope had to be moved to fulfill the constraint
  inout_fError += fLocalError.Abs();

  return vDir;
}

void ezClothSimulator::UpdateNodePositions(const ezSimdFloat tDiffSqr)
{
  const ezSimdFloat damping = m_fDampingFactor;
  const ezSimdVec4f acceleration = ezSimdConversion::ToVec3(m_vAcceleration) * tDiffSqr;

  for (auto& n : m_Nodes)
  {
    if (n.m_bFixed)
    {
      n.m_vPreviousPosition = n.m_vPosition;
    }
    else
    {
      // this (simple) logic is the so called 'Verlet integration' (+ damping)

      const ezSimdVec4f previousPos = n.m_vPosition;

      const ezSimdVec4f vel = (n.m_vPosition - n.m_vPreviousPosition) * damping;

      // instead of using a single global acceleration, this could also use individual accelerations per node
      // this would be needed to affect the rope more localized
      n.m_vPosition += vel + acceleration;
      n.m_vPreviousPosition = previousPos;
    }
  }
}

bool ezClothSimulator::HasEquilibrium(ezSimdFloat fAllowedMovement) const
{
  const ezSimdFloat fErrorSqr = fAllowedMovement * fAllowedMovement;

  for (const auto& n : m_Nodes)
  {
    if ((n.m_vPosition - n.m_vPreviousPosition).GetLengthSquared<3>() > fErrorSqr)
    {
      return false;
    }
  }

  return true;
}
