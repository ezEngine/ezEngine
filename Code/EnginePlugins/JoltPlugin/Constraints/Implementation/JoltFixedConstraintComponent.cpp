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

bool ezJoltFixedConstraintComponent::ExceededBreakingPoint()
{
  if (auto pConstraint = static_cast<JPH::FixedConstraint*>(m_pConstraint))
  {
    if (m_fBreakForce > 0)
    {
      if (pConstraint->GetTotalLambdaPosition().ReduceMax() >= m_fBreakForce)
      {
        return true;
      }
    }

    if (m_fBreakTorque > 0)
    {
      if (pConstraint->GetTotalLambdaRotation().ReduceMax() >= m_fBreakTorque)
      {
        return true;
      }
    }
  }

  return false;
}

void ezJoltFixedConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::FixedConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;

  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1 = inv1 * ezJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPoint2 = inv2 * ezJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);
  opt.mAxisX1 = inv1.Multiply3x3(ezJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * ezVec3(1, 0, 0)));
  opt.mAxisX2 = inv2.Multiply3x3(ezJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * ezVec3(1, 0, 0)));
  opt.mAxisY1 = inv1.Multiply3x3(ezJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * ezVec3(0, 1, 0)));
  opt.mAxisY2 = inv2.Multiply3x3(ezJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * ezVec3(0, 1, 0)));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltFixedConstraintComponent);
