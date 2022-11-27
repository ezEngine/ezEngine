#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Constraints/JoltConeConstraintComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltConeConstraintComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("ConeAngle", GetConeAngle, SetConeAngle)->AddAttributes(new ezClampValueAttribute(ezAngle(), ezAngle::Degree(175))),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezConeVisualizerAttribute(ezBasisAxis::PositiveX, "ConeAngle", 0.3f, nullptr)
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltConeConstraintComponent::ezJoltConeConstraintComponent() = default;
ezJoltConeConstraintComponent::~ezJoltConeConstraintComponent() = default;

void ezJoltConeConstraintComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_ConeAngle;
}

void ezJoltConeConstraintComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_ConeAngle;
}

void ezJoltConeConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::ConeConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;
  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1 = inv1 * ezJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPoint2 = inv2 * ezJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);
  opt.mHalfConeAngle = m_ConeAngle.GetRadian() * 0.5f;
  opt.mTwistAxis1 = inv1.Multiply3x3(ezJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * ezVec3::UnitXAxis()));
  opt.mTwistAxis2 = inv2.Multiply3x3(ezJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * ezVec3::UnitXAxis()));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}

void ezJoltConeConstraintComponent::ApplySettings()
{
  ezJoltConstraintComponent::ApplySettings();

  auto pConstraint = static_cast<JPH::ConeConstraint*>(m_pConstraint);
  pConstraint->SetHalfConeAngle(m_ConeAngle.GetRadian() * 0.5f);

  if (pConstraint->GetBody2()->IsInBroadPhase())
  {
    // wake up the bodies that are attached to this constraint
    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
    pModule->GetJoltSystem()->GetBodyInterface().ActivateBody(pConstraint->GetBody2()->GetID());
  }
}

void ezJoltConeConstraintComponent::SetConeAngle(ezAngle f)
{
  m_ConeAngle = f;
  QueueApplySettings();
}
