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
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::FixedConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;

  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1 = inv1 * ezJoltConversionUtils::ToVec3(m_localFrameA.m_vPosition);
  opt.mPoint2 = inv2 * ezJoltConversionUtils::ToVec3(m_localFrameB.m_vPosition);
  opt.mAxisX1 = inv1.Multiply3x3(ezJoltConversionUtils::ToVec3(m_localFrameA.m_qRotation * ezVec3(1, 0, 0)));
  opt.mAxisX2 = inv2.Multiply3x3(ezJoltConversionUtils::ToVec3(m_localFrameB.m_qRotation * ezVec3(1, 0, 0)));
  opt.mAxisY1 = inv1.Multiply3x3(ezJoltConversionUtils::ToVec3(m_localFrameA.m_qRotation * ezVec3(0, 1, 0)));
  opt.mAxisY2 = inv2.Multiply3x3(ezJoltConversionUtils::ToVec3(m_localFrameB.m_qRotation * ezVec3(0, 1, 0)));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}
