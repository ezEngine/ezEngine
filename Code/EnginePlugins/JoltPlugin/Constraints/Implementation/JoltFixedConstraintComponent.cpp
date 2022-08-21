#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/World/World.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <JoltPlugin/Constraints/JoltFixedConstraintComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltFixedConstraintComponent, 1, ezComponentMode::Static);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltFixedConstraintComponent::ezJoltFixedConstraintComponent() = default;
ezJoltFixedConstraintComponent::~ezJoltFixedConstraintComponent() = default;

void ezJoltFixedConstraintComponent::ApplySettings()
{
  ezJoltConstraintComponent::ApplySettings();
}

void ezJoltFixedConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto diff0 = pBody0->GetPosition() - pBody0->GetCenterOfMassPosition();
  const auto diff1 = pBody1->GetPosition() - pBody1->GetCenterOfMassPosition();

  JPH::FixedConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;

  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1 = ezJoltConversionUtils::ToVec3(m_localFrameA.m_vPosition) + diff0;
  opt.mPoint2 = ezJoltConversionUtils::ToVec3(m_localFrameB.m_vPosition) + diff1;
  opt.mAxisX1 = ezJoltConversionUtils::ToVec3(m_localFrameA.m_qRotation * ezVec3(1, 0, 0));
  opt.mAxisX2 = ezJoltConversionUtils::ToVec3(m_localFrameB.m_qRotation * ezVec3(1, 0, 0));
  opt.mAxisY1 = ezJoltConversionUtils::ToVec3(m_localFrameA.m_qRotation * ezVec3(0, 1, 0));
  opt.mAxisY2 = ezJoltConversionUtils::ToVec3(m_localFrameB.m_qRotation * ezVec3(0, 1, 0));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}
