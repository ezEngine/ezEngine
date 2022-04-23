#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <JoltPlugin/Constraints/JoltDistanceConstraintComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltDistanceConstraintComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("MinDistance", GetMinDistance, SetMinDistance)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("MaxDistance", GetMaxDistance, SetMaxDistance)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("Frequency", GetFrequency, SetFrequency)->AddAttributes(new ezClampValueAttribute(0.0f, 120.0f), new ezDefaultValueAttribute(2.0f)),
    EZ_ACCESSOR_PROPERTY("Damping", GetDamping, SetDamping)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f), new ezDefaultValueAttribute(0.5f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezSphereVisualizerAttribute("MinDistance", ezColor::IndianRed),
    new ezSphereVisualizerAttribute("MaxDistance", ezColor::LightSkyBlue),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltDistanceConstraintComponent::ezJoltDistanceConstraintComponent() = default;
ezJoltDistanceConstraintComponent::~ezJoltDistanceConstraintComponent() = default;

void ezJoltDistanceConstraintComponent::SetMinDistance(float value)
{
  m_fMinDistance = value;
  QueueApplySettings();
}

void ezJoltDistanceConstraintComponent::SetMaxDistance(float value)
{
  m_fMaxDistance = value;
  QueueApplySettings();
}

void ezJoltDistanceConstraintComponent::SetFrequency(float value)
{
  m_fFrequency = value;
  QueueApplySettings();
}

void ezJoltDistanceConstraintComponent::SetDamping(float value)
{
  m_fDamping = value;
  QueueApplySettings();
}

void ezJoltDistanceConstraintComponent::ApplySettings()
{
  ezJoltConstraintComponent::ApplySettings();

  JPH::DistanceConstraint* pConstraint = static_cast<JPH::DistanceConstraint*>(m_pConstraint);

  pConstraint->SetFrequency(m_fFrequency);
  pConstraint->SetDamping(m_fDamping);

  const float fMin = ezMath::Max(0.0f, m_fMinDistance);
  const float fMax = ezMath::Max(fMin, m_fMaxDistance);
  pConstraint->SetDistance(fMin, fMax);

  if (pConstraint->GetBody2()->IsInBroadPhase())
  {
    // wake up the bodies that are attached to this constraint
    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
    pModule->GetJoltSystem()->GetBodyInterface().ActivateBody(pConstraint->GetBody2()->GetID());
  }
}

void ezJoltDistanceConstraintComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fMinDistance;
  s << m_fMaxDistance;
  s << m_fFrequency;
  s << m_fDamping;
}

void ezJoltDistanceConstraintComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fMinDistance;
  s >> m_fMaxDistance;
  s >> m_fFrequency;
  s >> m_fDamping;
}

void ezJoltDistanceConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto diff0 = pBody0->GetPosition() - pBody0->GetCenterOfMassPosition();
  const auto diff1 = pBody1->GetPosition() - pBody1->GetCenterOfMassPosition();

  JPH::DistanceConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;
  opt.mMinDistance = 0;
  opt.mMaxDistance = 1;
  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1 = ezJoltConversionUtils::ToVec3(m_localFrameA.m_vPosition) + diff0;
  opt.mPoint2 = ezJoltConversionUtils::ToVec3(m_localFrameB.m_vPosition) + diff1;
  opt.mDamping = m_fDamping;
  opt.mFrequency = m_fFrequency;

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}
