#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Constraints/JoltHingeConstraintComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltHingeConstraintComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("LimitMode", ezJoltConstraintLimitMode, GetLimitMode, SetLimitMode),
    EZ_ACCESSOR_PROPERTY("LowerLimit", GetLowerLimitAngle, SetLowerLimitAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0), ezAngle::Degree(180))),
    EZ_ACCESSOR_PROPERTY("UpperLimit", GetUpperLimitAngle, SetUpperLimitAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0), ezAngle::Degree(180))),
    EZ_ACCESSOR_PROPERTY("Friction", GetFriction, SetFriction)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ENUM_ACCESSOR_PROPERTY("DriveMode", ezJoltConstraintDriveMode, GetDriveMode, SetDriveMode),
    EZ_ACCESSOR_PROPERTY("DriveTargetValue", GetDriveTargetValue, SetDriveTargetValue),
    EZ_ACCESSOR_PROPERTY("DriveStrength", GetDriveStrength, SetDriveStrength)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezMinValueTextAttribute("Maximum")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.2, ezColor::BurlyWood)
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltHingeConstraintComponent::ezJoltHingeConstraintComponent() = default;
ezJoltHingeConstraintComponent::~ezJoltHingeConstraintComponent() = default;

void ezJoltHingeConstraintComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_LimitMode;
  s << m_LowerLimit;
  s << m_UpperLimit;

  s << m_DriveMode;
  s << m_DriveTargetValue;
  s << m_fDriveStrength;

  s << m_fFriction;
}

void ezJoltHingeConstraintComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_LimitMode;
  s >> m_LowerLimit;
  s >> m_UpperLimit;

  s >> m_DriveMode;
  s >> m_DriveTargetValue;
  s >> m_fDriveStrength;

  s >> m_fFriction;
}

void ezJoltHingeConstraintComponent::SetLimitMode(ezJoltConstraintLimitMode::Enum mode)
{
  m_LimitMode = mode;
  QueueApplySettings();
}

void ezJoltHingeConstraintComponent::SetLowerLimitAngle(ezAngle f)
{
  m_LowerLimit = ezMath::Clamp(f, ezAngle(), ezAngle::Degree(180));
  QueueApplySettings();
}

void ezJoltHingeConstraintComponent::SetUpperLimitAngle(ezAngle f)
{
  m_UpperLimit = ezMath::Clamp(f, ezAngle(), ezAngle::Degree(180));
  QueueApplySettings();
}

void ezJoltHingeConstraintComponent::SetFriction(float f)
{
  m_fFriction = ezMath::Max(f, 0.0f);
  QueueApplySettings();
}

void ezJoltHingeConstraintComponent::SetDriveMode(ezJoltConstraintDriveMode::Enum mode)
{
  m_DriveMode = mode;
  QueueApplySettings();
}

void ezJoltHingeConstraintComponent::SetDriveTargetValue(ezAngle f)
{
  m_DriveTargetValue = f;
  QueueApplySettings();
}

void ezJoltHingeConstraintComponent::SetDriveStrength(float f)
{
  m_fDriveStrength = ezMath::Max(f, 0.0f);
  QueueApplySettings();
}

void ezJoltHingeConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::HingeConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;
  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1 = inv1 * ezJoltConversionUtils::ToVec3(m_localFrameA.m_vPosition);
  opt.mPoint2 = inv2 * ezJoltConversionUtils::ToVec3(m_localFrameB.m_vPosition);
  opt.mHingeAxis1 = inv1.Multiply3x3(ezJoltConversionUtils::ToVec3(m_localFrameA.m_qRotation * ezVec3(1, 0, 0)));
  opt.mHingeAxis2 = inv2.Multiply3x3(ezJoltConversionUtils::ToVec3(m_localFrameB.m_qRotation * ezVec3(1, 0, 0)));
  opt.mNormalAxis1 = inv1.Multiply3x3(ezJoltConversionUtils::ToVec3(m_localFrameA.m_qRotation * ezVec3(0, 1, 0)));
  opt.mNormalAxis2 = inv2.Multiply3x3(ezJoltConversionUtils::ToVec3(m_localFrameB.m_qRotation * ezVec3(0, 1, 0)));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}

void ezJoltHingeConstraintComponent::ApplySettings()
{
  ezJoltConstraintComponent::ApplySettings();

  JPH::HingeConstraint* pConstraint = static_cast<JPH::HingeConstraint*>(m_pConstraint);

  pConstraint->SetMaxFrictionTorque(m_fFriction);

  if (m_LimitMode != ezJoltConstraintLimitMode::NoLimit)
  {
    float low = m_LowerLimit.GetRadian();
    float high = m_UpperLimit.GetRadian();

    const float fLowest = ezAngle::Degree(1.0f).GetRadian();

    // there should be at least some slack
    if (low <= fLowest && high <= fLowest)
    {
      low = fLowest;
      high = fLowest;
    }

    pConstraint->SetLimits(-low, high);
  }
  else
  {
    pConstraint->SetLimits(-JPH::JPH_PI, +JPH::JPH_PI);
  }

  // drive
  {
    if (m_DriveMode == ezJoltConstraintDriveMode::NoDrive)
    {
      pConstraint->SetMotorState(JPH::EMotorState::Off);
    }
    else
    {
      if (m_DriveMode == ezJoltConstraintDriveMode::DriveVelocity)
      {
        pConstraint->SetMotorState(JPH::EMotorState::Velocity);
        pConstraint->SetTargetAngularVelocity(m_DriveTargetValue.GetRadian());
      }
      else
      {
        pConstraint->SetMotorState(JPH::EMotorState::Position);
        pConstraint->SetTargetAngle(m_DriveTargetValue.GetRadian());
      }

      const float strength = (m_fDriveStrength == 0) ? FLT_MAX : m_fDriveStrength;

      pConstraint->GetMotorSettings().mFrequency = 20.0f;
      pConstraint->GetMotorSettings().SetForceLimit(strength);
      pConstraint->GetMotorSettings().SetTorqueLimit(strength);
    }
  }

  if (pConstraint->GetBody2()->IsInBroadPhase())
  {
    // wake up the bodies that are attached to this constraint
    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
    pModule->GetJoltSystem()->GetBodyInterface().ActivateBody(pConstraint->GetBody2()->GetID());
  }
}
