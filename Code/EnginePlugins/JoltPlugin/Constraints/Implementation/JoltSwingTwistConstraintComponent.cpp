#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Constraints/JoltSwingTwistConstraintComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltSwingTwistConstraintComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("SwingLimitY", GetSwingLimitY, SetSwingLimitY)->AddAttributes(new ezClampValueAttribute(ezAngle(), ezAngle::Degree(175))),
    EZ_ACCESSOR_PROPERTY("SwingLimitZ", GetSwingLimitZ, SetSwingLimitZ)->AddAttributes(new ezClampValueAttribute(ezAngle(), ezAngle::Degree(175))),

    EZ_ACCESSOR_PROPERTY("Friction", GetFriction, SetFriction)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),

    EZ_ACCESSOR_PROPERTY("LowerTwistLimit", GetLowerTwistLimit, SetLowerTwistLimit)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(5), ezAngle::Degree(175)), new ezDefaultValueAttribute(ezAngle::Degree(90))),
    EZ_ACCESSOR_PROPERTY("UpperTwistLimit", GetUpperTwistLimit, SetUpperTwistLimit)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(5), ezAngle::Degree(175)), new ezDefaultValueAttribute(ezAngle::Degree(90))),

    //EZ_ENUM_ACCESSOR_PROPERTY("TwistDriveMode", ezJoltConstraintDriveMode, GetTwistDriveMode, SetTwistDriveMode),
    //EZ_ACCESSOR_PROPERTY("TwistDriveTargetValue", GetTwistDriveTargetValue, SetTwistDriveTargetValue),
    //EZ_ACCESSOR_PROPERTY("TwistDriveStrength", GetTwistDriveStrength, SetTwistDriveStrength)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezMinValueTextAttribute("Maximum"))
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezConeVisualizerAttribute(ezBasisAxis::PositiveX, "SwingLimitY", 0.3f, nullptr)
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltSwingTwistConstraintComponent::ezJoltSwingTwistConstraintComponent() = default;
ezJoltSwingTwistConstraintComponent::~ezJoltSwingTwistConstraintComponent() = default;

void ezJoltSwingTwistConstraintComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_SwingLimitY;
  s << m_SwingLimitZ;

  s << m_LowerTwistLimit;
  s << m_UpperTwistLimit;

  s << m_fFriction;

  // s << m_TwistDriveMode;
  // s << m_TwistDriveTargetValue;
  // s << m_fTwistDriveStrength;
}

void ezJoltSwingTwistConstraintComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_SwingLimitY;
  s >> m_SwingLimitZ;

  s >> m_LowerTwistLimit;
  s >> m_UpperTwistLimit;

  s >> m_fFriction;

  // s >> m_TwistDriveMode;
  // s >> m_TwistDriveTargetValue;
  // s >> m_fTwistDriveStrength;
}

void ezJoltSwingTwistConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::SwingTwistConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;
  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPosition1 = inv1 * ezJoltConversionUtils::ToVec3(m_localFrameA.m_vPosition);
  opt.mPosition2 = inv2 * ezJoltConversionUtils::ToVec3(m_localFrameB.m_vPosition);
  opt.mPlaneHalfConeAngle = m_SwingLimitY.GetRadian() * 0.5f;
  opt.mNormalHalfConeAngle = m_SwingLimitZ.GetRadian() * 0.5f;
  opt.mMaxFrictionTorque = m_fFriction;
  opt.mTwistAxis1 = inv1.Multiply3x3(ezJoltConversionUtils::ToVec3(m_localFrameA.m_qRotation * ezVec3::UnitXAxis()));
  opt.mTwistAxis2 = inv2.Multiply3x3(ezJoltConversionUtils::ToVec3(m_localFrameB.m_qRotation * ezVec3::UnitXAxis()));
  opt.mTwistMinAngle = -m_LowerTwistLimit.GetRadian();
  opt.mTwistMaxAngle = m_UpperTwistLimit.GetRadian();
  opt.mPlaneAxis1 = inv1.Multiply3x3(ezJoltConversionUtils::ToVec3(m_localFrameA.m_qRotation * ezVec3::UnitYAxis()));
  opt.mPlaneAxis2 = inv2.Multiply3x3(ezJoltConversionUtils::ToVec3(m_localFrameB.m_qRotation * ezVec3::UnitYAxis()));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}

void ezJoltSwingTwistConstraintComponent::ApplySettings()
{
  ezJoltConstraintComponent::ApplySettings();

  auto pConstraint = static_cast<JPH::SwingTwistConstraint*>(m_pConstraint);

  pConstraint->SetMaxFrictionTorque(m_fFriction);
  pConstraint->SetPlaneHalfConeAngle(m_SwingLimitY.GetRadian() * 0.5f);
  pConstraint->SetNormalHalfConeAngle(m_SwingLimitZ.GetRadian() * 0.5f);
  pConstraint->SetTwistMinAngle(-m_LowerTwistLimit.GetRadian());
  pConstraint->SetTwistMaxAngle(m_UpperTwistLimit.GetRadian());

  // drive
  //{
  //  if (m_TwistDriveMode == ezJoltConstraintDriveMode::NoDrive)
  //  {
  //    pConstraint->SetTwistMotorState(JPH::EMotorState::Off);
  //  }
  //  else
  //  {
  //    if (m_TwistDriveMode == ezJoltConstraintDriveMode::DriveVelocity)
  //    {
  //      pConstraint->SetTwistMotorState(JPH::EMotorState::Velocity);
  //      pConstraint->SetTargetAngularVelocityCS(JPH::Vec3::sReplicate(m_TwistDriveTargetValue.GetRadian()));
  //    }
  //    else
  //    {
  //      pConstraint->SetTwistMotorState(JPH::EMotorState::Position);
  //      //pConstraint->SetTargetOrientationCS(m_TwistDriveTargetValue.GetRadian());
  //    }

  //    const float strength = (m_fTwistDriveStrength == 0) ? FLT_MAX : m_fTwistDriveStrength;

  //    pConstraint->GetTwistMotorSettings().mFrequency = 2.0f;
  //    pConstraint->GetTwistMotorSettings().SetForceLimit(strength);
  //    pConstraint->GetTwistMotorSettings().SetTorqueLimit(strength);
  //  }
  //}

  if (pConstraint->GetBody2()->IsInBroadPhase())
  {
    // wake up the bodies that are attached to this constraint
    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
    pModule->GetJoltSystem()->GetBodyInterface().ActivateBody(pConstraint->GetBody2()->GetID());
  }
}

void ezJoltSwingTwistConstraintComponent::SetSwingLimitZ(ezAngle f)
{
  m_SwingLimitZ = f;
  QueueApplySettings();
}

void ezJoltSwingTwistConstraintComponent::SetSwingLimitY(ezAngle f)
{
  m_SwingLimitY = f;
  QueueApplySettings();
}

void ezJoltSwingTwistConstraintComponent::SetFriction(float f)
{
  m_fFriction = f;
  QueueApplySettings();
}

void ezJoltSwingTwistConstraintComponent::SetLowerTwistLimit(ezAngle f)
{
  m_LowerTwistLimit = f;
  QueueApplySettings();
}

void ezJoltSwingTwistConstraintComponent::SetUpperTwistLimit(ezAngle f)
{
  m_UpperTwistLimit = f;
  QueueApplySettings();
}

// void ezJoltSwingTwistConstraintComponent::SetTwistDriveMode(ezJoltConstraintDriveMode::Enum mode)
//{
//   m_TwistDriveMode = mode;
//   QueueApplySettings();
// }
//
// void ezJoltSwingTwistConstraintComponent::SetTwistDriveTargetValue(ezAngle f)
//{
//   m_TwistDriveTargetValue = f;
//   QueueApplySettings();
// }
//
// void ezJoltSwingTwistConstraintComponent::SetTwistDriveStrength(float f)
//{
//   m_fTwistDriveStrength = f;
//   QueueApplySettings();
// }
