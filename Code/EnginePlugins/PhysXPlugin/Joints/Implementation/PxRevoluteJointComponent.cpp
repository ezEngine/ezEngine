#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <PhysXPlugin/Joints/PxRevoluteJointComponent.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxRevoluteJointComponent, 5, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("LimitMode", ezPxJointLimitMode, GetLimitMode, SetLimitMode),
    EZ_ACCESSOR_PROPERTY("LowerLimit", GetLowerLimitAngle, SetLowerLimitAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(-360), ezAngle::Degree(+360))),
    EZ_ACCESSOR_PROPERTY("UpperLimit", GetUpperLimitAngle, SetUpperLimitAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(-360), ezAngle::Degree(+360))),
    EZ_ACCESSOR_PROPERTY("SpringStiffness", GetSpringStiffness, SetSpringStiffness)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SpringDamping", GetSpringDamping, SetSpringDamping)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ENUM_ACCESSOR_PROPERTY("DriveMode", ezPxJointDriveMode, GetDriveMode, SetDriveMode),
    EZ_ACCESSOR_PROPERTY("DriveVelocity", GetDriveVelocity, SetDriveVelocity),
    EZ_ACCESSOR_PROPERTY("MaxDriveTorque", GetDriveTorque, SetDriveTorque)->AddAttributes(new ezDefaultValueAttribute(100.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.2, ezColor::SlateGray)
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxRevoluteJointComponent::ezPxRevoluteJointComponent() = default;
ezPxRevoluteJointComponent::~ezPxRevoluteJointComponent() = default;

void ezPxRevoluteJointComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_LimitMode; // version 3
  s << m_LowerLimit;
  s << m_UpperLimit;

  s << m_DriveMode;
  s << m_fDriveVelocity;

  // version 2
  s << m_fMaxDriveTorque;

  // version 4
  s << m_fSpringStiffness;
  s << m_fSpringDamping;
}

void ezPxRevoluteJointComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  if (uiVersion <= 2)
  {
    bool m_bLimitRotation;
    s >> m_bLimitRotation;
    m_LimitMode = m_bLimitRotation ? ezPxJointLimitMode::HardLimit : ezPxJointLimitMode::NoLimit;
  }
  else
  {
    s >> m_LimitMode;
  }

  s >> m_LowerLimit;
  s >> m_UpperLimit;

  if (uiVersion < 5)
  {
    bool m_bEnableDrive, m_bEnableDriveBraking;
    s >> m_bEnableDrive;
    s >> m_bEnableDriveBraking;

    if (m_bEnableDrive)
    {
      m_DriveMode = m_bEnableDriveBraking ? ezPxJointDriveMode::DriveAndBrake : ezPxJointDriveMode::DriveAndSpin;
    }
  }
  else
  {
    s >> m_DriveMode;
  }

  s >> m_fDriveVelocity;

  if (uiVersion >= 2)
  {
    s >> m_fMaxDriveTorque;
  }

  if (uiVersion >= 4)
  {
    s >> m_fSpringStiffness;
    s >> m_fSpringDamping;
  }
}

void ezPxRevoluteJointComponent::SetLimitMode(ezPxJointLimitMode::Enum mode)
{
  m_LimitMode = mode;
  QueueApplySettings();
}

void ezPxRevoluteJointComponent::SetLowerLimitAngle(ezAngle f)
{
  m_LowerLimit = f;
  QueueApplySettings();
}

void ezPxRevoluteJointComponent::SetUpperLimitAngle(ezAngle f)
{
  m_UpperLimit = f;
  QueueApplySettings();
}

void ezPxRevoluteJointComponent::SetSpringStiffness(float f)
{
  m_fSpringStiffness = f;
  QueueApplySettings();
}

void ezPxRevoluteJointComponent::SetSpringDamping(float f)
{
  m_fSpringDamping = f;
  QueueApplySettings();
}

void ezPxRevoluteJointComponent::SetDriveMode(ezPxJointDriveMode::Enum mode)
{
  m_DriveMode = mode;
  QueueApplySettings();
}

void ezPxRevoluteJointComponent::SetDriveVelocity(float f)
{
  m_fDriveVelocity = f;
  QueueApplySettings();
}

void ezPxRevoluteJointComponent::SetDriveTorque(float f)
{
  m_fMaxDriveTorque = f;
  QueueApplySettings();
}

void ezPxRevoluteJointComponent::CreateJointType(PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1, const PxTransform& localFrame1)
{
  m_pJoint = PxRevoluteJointCreate(*(ezPhysX::GetSingleton()->GetPhysXAPI()), actor0, localFrame0, actor1, localFrame1);
}

void ezPxRevoluteJointComponent::ApplySettings()
{
  ezPxJointComponent::ApplySettings();

  physx::PxRevoluteJoint* pJoint = static_cast<physx::PxRevoluteJoint*>(m_pJoint);

  pJoint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, m_LimitMode != ezPxJointLimitMode::NoLimit);

  if (m_LimitMode != ezPxJointLimitMode::NoLimit)
  {

    float low = m_LowerLimit.GetRadian();
    float high = m_UpperLimit.GetRadian();

    if (low > high)
      ezMath::Swap(low, high);

    // PhysX disables any rotation if the two angles are identical, even if it is a soft ('springy') joint
    if (low == high)
      high = low + ezAngle::Degree(1.0f).GetRadian();

    const float range = ezMath::Min(high - low, 1.99f * ezMath::Pi<float>());
    high = low + range;

    PxJointAngularLimitPair limit(low, high);

    if (m_LimitMode == ezPxJointLimitMode::SoftLimit)
    {

      limit.stiffness = ezMath::Max(0.1f, m_fSpringStiffness);
      limit.damping = ezMath::Max(0.1f, m_fSpringDamping);
    }
    else
    {
      limit.restitution = ezMath::Clamp(m_fSpringStiffness, 0.0f, 1.0f);
      limit.bounceThreshold = m_fSpringDamping;
    }

    pJoint->setLimit(limit);
  }

  // drive
  {
    pJoint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, m_DriveMode != ezPxJointDriveMode::NoDrive);

    if (m_DriveMode != ezPxJointDriveMode::NoDrive)
    {
      pJoint->setDriveVelocity(m_fDriveVelocity);

      // pJoint->setConstraintFlag(PxConstraintFlag::eDRIVE_LIMITS_ARE_FORCES, false);
      pJoint->setDriveForceLimit(m_fMaxDriveTorque);

      // pJoint->setDriveGearRatio

      pJoint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_FREESPIN, m_DriveMode == ezPxJointDriveMode::DriveAndSpin);
    }
  }
}
