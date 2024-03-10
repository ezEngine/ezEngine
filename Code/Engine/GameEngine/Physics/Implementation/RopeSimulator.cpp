#include <GameEngine/GameEnginePCH.h>

#include <Foundation/SimdMath/SimdConversion.h>
#include <GameEngine/Physics/RopeSimulator.h>

ezRopeSimulator::ezRopeSimulator() = default;
ezRopeSimulator::~ezRopeSimulator() = default;

void ezRopeSimulator::SimulateRope(const ezTime& diff)
{
  m_LeftOverTimeStep += diff;

  constexpr ezTime tStep = ezTime::MakeFromSeconds(1.0 / 60.0);
  const ezSimdFloat tStepSqr = static_cast<float>(tStep.GetSeconds() * tStep.GetSeconds());
  const ezSimdFloat fAllowedError = m_fSegmentLength;

  while (m_LeftOverTimeStep >= tStep)
  {
    SimulateStep(tStepSqr, 32, fAllowedError);

    m_LeftOverTimeStep -= tStep;
  }
}

void ezRopeSimulator::SimulateStep(const ezSimdFloat fDiffSqr, ezUInt32 uiMaxIterations, ezSimdFloat fAllowedError)
{
  if (m_Nodes.GetCount() < 2)
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

void ezRopeSimulator::SimulateTillEquilibrium(ezSimdFloat fAllowedMovement, ezUInt32 uiMaxIterations)
{
  constexpr ezTime tStep = ezTime::MakeFromSeconds(1.0 / 60.0);
  ezSimdFloat tStepSqr = static_cast<float>(tStep.GetSeconds() * tStep.GetSeconds());

  ezUInt8 uiInEquilibrium = 0;

  while (uiInEquilibrium < 100 && uiMaxIterations > 0)
  {
    --uiMaxIterations;

    SimulateStep(tStepSqr, 32, m_fSegmentLength);
    uiInEquilibrium++;

    if (!HasEquilibrium(fAllowedMovement))
    {
      uiInEquilibrium = 0;
    }
  }
}

bool ezRopeSimulator::HasEquilibrium(ezSimdFloat fAllowedMovement) const
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

float ezRopeSimulator::GetTotalLength() const
{
  if (m_Nodes.GetCount() <= 1)
    return 0.0f;

  float len = 0;

  ezSimdVec4f prev = m_Nodes[0].m_vPosition;
  for (ezUInt32 i = 1; i < m_Nodes.GetCount(); ++i)
  {
    const ezSimdVec4f cur = m_Nodes[i].m_vPosition;

    len += (cur - prev).GetLength<3>();

    prev = cur;
  }

  return len;
}

ezSimdVec4f ezRopeSimulator::GetPositionAtLength(float fLength) const
{
  if (m_Nodes.IsEmpty())
    return ezSimdVec4f::MakeZero();

  ezSimdVec4f prev = m_Nodes[0].m_vPosition;
  for (ezUInt32 i = 1; i < m_Nodes.GetCount(); ++i)
  {
    const ezSimdVec4f cur = m_Nodes[i].m_vPosition;

    const ezSimdVec4f dir = cur - prev;
    const float dist = dir.GetLength<3>();

    if (fLength <= dist)
    {
      const float interpolate = fLength / dist;
      return prev + dir * interpolate;
    }

    fLength -= dist;
    prev = cur;
  }

  return m_Nodes.PeekBack().m_vPosition;
}

ezSimdVec4f ezRopeSimulator::MoveTowards(const ezSimdVec4f posThis, const ezSimdVec4f posNext, ezSimdFloat factor, const ezSimdVec4f fallbackDir, ezSimdFloat& inout_fError)
{
  ezSimdVec4f vDir = (posNext - posThis);
  ezSimdFloat fLen = vDir.GetLength<3>();

  if (fLen < m_fSegmentLength)
  {
    return ezSimdVec4f::MakeZero();
  }

  vDir /= fLen;
  fLen -= m_fSegmentLength;

  const ezSimdFloat fLocalError = fLen * factor;

  vDir *= fLocalError;

  // keep track of how much the rope had to be moved to fulfill the constraint
  inout_fError += fLocalError.Abs();

  return vDir;
}

ezSimdFloat ezRopeSimulator::EnforceDistanceConstraint()
{
  // this is the "Jakobsen method" to enforce the distance constraints in each rope node
  // just move each node half the error amount towards the left and right neighboring nodes
  // the ends are either not moved at all (when they are 'attached' to something)
  // or they are moved most of the way
  // this is applied iteratively until the overall error is pretty low

  auto& firstNode = m_Nodes[0];
  auto& lastNode = m_Nodes.PeekBack();

  ezSimdFloat fError = ezSimdFloat::MakeZero();

  if (!m_bFirstNodeIsFixed)
  {
    const ezSimdVec4f posThis = m_Nodes[0].m_vPosition;
    const ezSimdVec4f posNext = m_Nodes[1].m_vPosition;

    m_Nodes[0].m_vPosition += MoveTowards(posThis, posNext, 0.75f, ezSimdVec4f(0, 0, 1), fError);
  }

  for (ezUInt32 i = 1; i < m_Nodes.GetCount() - 1; ++i)
  {
    const ezSimdVec4f posThis = m_Nodes[i].m_vPosition;
    const ezSimdVec4f posPrev = m_Nodes[i - 1].m_vPosition;
    const ezSimdVec4f posNext = m_Nodes[i + 1].m_vPosition;

    m_Nodes[i].m_vPosition += MoveTowards(posThis, posPrev, 0.5f, ezSimdVec4f(0, 0, 1), fError);
    m_Nodes[i].m_vPosition += MoveTowards(posThis, posNext, 0.5f, ezSimdVec4f(0, 0, -1), fError);
  }

  if (!m_bLastNodeIsFixed)
  {
    const ezUInt32 i = m_Nodes.GetCount() - 1;
    const ezSimdVec4f posThis = m_Nodes[i].m_vPosition;
    const ezSimdVec4f posPrev = m_Nodes[i - 1].m_vPosition;

    m_Nodes[i].m_vPosition += MoveTowards(posThis, posPrev, 0.75f, ezSimdVec4f(0, 0, 1), fError);
  }

  return fError;
}

void ezRopeSimulator::UpdateNodePositions(const ezSimdFloat tDiffSqr)
{
  const ezUInt32 uiFirstNode = m_bFirstNodeIsFixed ? 1 : 0;
  const ezUInt32 uiNumNodes = m_bLastNodeIsFixed ? m_Nodes.GetCount() - 1 : m_Nodes.GetCount();

  const ezSimdFloat damping = m_fDampingFactor;

  const ezSimdVec4f acceleration = ezSimdConversion::ToVec3(m_vAcceleration) * tDiffSqr;

  for (ezUInt32 i = uiFirstNode; i < uiNumNodes; ++i)
  {
    // this (simple) logic is the so called 'Verlet integration' (+ damping)

    auto& n = m_Nodes[i];

    const ezSimdVec4f previousPos = n.m_vPosition;

    const ezSimdVec4f vel = (n.m_vPosition - n.m_vPreviousPosition) * damping;

    // instead of using a single global acceleration, this could also use individual accelerations per node
    // this would be needed to affect the rope more localized
    n.m_vPosition += vel + acceleration;
    n.m_vPreviousPosition = previousPos;
  }

  if (m_bFirstNodeIsFixed)
  {
    m_Nodes[0].m_vPreviousPosition = m_Nodes[0].m_vPosition;
  }
  if (m_bLastNodeIsFixed)
  {
    m_Nodes.PeekBack().m_vPreviousPosition = m_Nodes.PeekBack().m_vPosition;
  }
}
