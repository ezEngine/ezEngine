#include <GameEnginePCH.h>

#include <GameEngine/Physics/RopeSimulator.h>

ezRopeSimulator::ezRopeSimulator() = default;
ezRopeSimulator::~ezRopeSimulator() = default;

void ezRopeSimulator::SimulateRope(const ezTime& tDiff)
{
  m_leftOverTimeStep += tDiff;

  constexpr ezTime tStep = ezTime::Seconds(1.0 / 60.0);
  constexpr float tStepSqr = static_cast<float>(tStep.GetSeconds() * tStep.GetSeconds());

  while (m_leftOverTimeStep >= tStep)
  {
    SimulateStep(tStepSqr, 32, m_fSegmentLength);

    m_leftOverTimeStep -= tStep;
  }
}

void ezRopeSimulator::SimulateStep(const float tDiffSqr, ezUInt32 uiMaxIterations, double fAllowedError)
{
  if (m_Nodes.GetCount() < 2)
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

void ezRopeSimulator::SimulateTillEquilibrium(float fAllowedMovement, ezUInt32 uiMaxIterations)
{
  constexpr ezTime tStep = ezTime::Seconds(1.0 / 60.0);
  constexpr float tStepSqr = static_cast<float>(tStep.GetSeconds() * tStep.GetSeconds());

  const float fErrorSqr = ezMath::Square(fAllowedMovement);

  ezUInt8 uiInEquilibrium = 0;

  while (uiInEquilibrium < 100 && uiMaxIterations > 0)
  {
    --uiMaxIterations;

    SimulateStep(tStepSqr, 32, m_fSegmentLength);
    uiInEquilibrium++;

    for (const auto& n : m_Nodes)
    {
      if ((n.m_vPosition - n.m_vPreviousPosition).GetLengthSquared() > fErrorSqr)
      {
        uiInEquilibrium = 0;
        break;
      }
    }
  }
}

ezVec3 ezRopeSimulator::MoveTowards(const ezVec3& posThis, const ezVec3& posNext, float factor, const ezVec3& fallbackDir, double& inout_fError)
{
  ezVec3 vDir = (posNext - posThis);
  float fLen = vDir.GetLength();

  if (ezMath::IsZero(fLen, 0.001f))
  {
    vDir = fallbackDir;
    fLen = 1;
  }

  vDir /= fLen;
  fLen -= m_fSegmentLength;

  const float fLocalError = fLen * factor;

  vDir *= fLocalError;

  // keep track of how much the rope had to be moved to fulfill the constraint
  inout_fError += ezMath::Abs(fLocalError);

  return vDir;
}

double ezRopeSimulator::EnforceDistanceConstraint()
{
  // this is the "Jakobsen method" to enforce the distance constraints in each rope node
  // just move each node half the error amount towards the left and right neighboring nodes
  // the ends are either not moved at all (when they are 'attached' to something)
  // or they are moved most of the way
  // this is applied iteratively until the overall error is pretty low

  auto& firstNode = m_Nodes[0];
  auto& lastNode = m_Nodes.PeekBack();

  double fError = 0.0;

  if (!m_bFirstNodeIsFixed)
  {
    const ezVec3 posThis = m_Nodes[0].m_vPosition;
    const ezVec3 posNext = m_Nodes[1].m_vPosition;

    m_Nodes[0].m_vPosition += MoveTowards(posThis, posNext, 0.75f, ezVec3(0, 0, 1), fError);
  }

  for (ezUInt32 i = 1; i < m_Nodes.GetCount() - 1; ++i)
  {
    const ezVec3 posThis = m_Nodes[i].m_vPosition;
    const ezVec3 posPrev = m_Nodes[i - 1].m_vPosition;
    const ezVec3 posNext = m_Nodes[i + 1].m_vPosition;

    m_Nodes[i].m_vPosition += MoveTowards(posThis, posPrev, 0.5f, ezVec3(0, 0, 1), fError);
    m_Nodes[i].m_vPosition += MoveTowards(posThis, posNext, 0.5f, ezVec3(0, 0, -1), fError);
  }

  if (!m_bLastNodeIsFixed)
  {
    const ezUInt32 i = m_Nodes.GetCount() - 1;
    const ezVec3 posThis = m_Nodes[i].m_vPosition;
    const ezVec3 posPrev = m_Nodes[i - 1].m_vPosition;

    m_Nodes[i].m_vPosition += MoveTowards(posThis, posPrev, 0.75f, ezVec3(0, 0, 1), fError);
  }

  return fError;
}

void ezRopeSimulator::UpdateNodePositions(const float tDiffSqr)
{
  const ezUInt32 uiFirstNode = m_bFirstNodeIsFixed ? 1 : 0;
  const ezUInt32 uiNumNodes = m_bLastNodeIsFixed ? m_Nodes.GetCount() - 1 : m_Nodes.GetCount();

  for (ezUInt32 i = uiFirstNode; i < uiNumNodes; ++i)
  {
    // this (simple) logic is the so called 'Verlet integration' (+ damping)

    auto& n = m_Nodes[i];

    const ezVec3 previousPos = n.m_vPosition;

    const ezVec3 vel = (n.m_vPosition - n.m_vPreviousPosition) * m_fDampingFactor;

    // instead of using a single global acceleration, this could also use individual accelerations per node
    // this would be needed to affect the rope more localized
    n.m_vPosition += vel + tDiffSqr * m_vAcceleration;
    n.m_vPreviousPosition = previousPos;
  }
}
