#include <PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <PhysXPlugin/Joints/PxRevoluteJointComponent.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxRevoluteJointComponent, 4, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("LimitMode", ezPxJointLimitMode, GetLimitMode, SetLimitMode),
    EZ_ACCESSOR_PROPERTY("LowerLimit", GetLowerLimitAngle, SetLowerLimitAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(-360), ezAngle::Degree(+360))),
    EZ_ACCESSOR_PROPERTY("UpperLimit", GetUpperLimitAngle, SetUpperLimitAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(-360), ezAngle::Degree(+360))),
    EZ_ACCESSOR_PROPERTY("SpringStiffness", GetSpringStiffness, SetSpringStiffness)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SpringDamping", GetSpringDamping, SetSpringDamping)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("EnableDrive", m_bEnableDrive),
    EZ_ACCESSOR_PROPERTY("DriveVelocity", GetDriveVelocity, SetDriveVelocity),
    EZ_ACCESSOR_PROPERTY("MaxDriveTorque", GetDriveTorque, SetDriveTorque)->AddAttributes(new ezDefaultValueAttribute(100.0f)),
    EZ_MEMBER_PROPERTY("DriveBraking", m_bEnableDriveBraking),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.2, ezColor::LightSkyBlue)
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

  s << m_bEnableDrive;
  s << m_bEnableDriveBraking;
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

  s >> m_bEnableDrive;
  s >> m_bEnableDriveBraking;
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
  ApplyLimits();
}

void ezPxRevoluteJointComponent::SetLowerLimitAngle(ezAngle f)
{
  m_LowerLimit = f;
  ApplyLimits();
}

void ezPxRevoluteJointComponent::SetUpperLimitAngle(ezAngle f)
{
  m_UpperLimit = f;
  ApplyLimits();
}

void ezPxRevoluteJointComponent::SetSpringStiffness(float f)
{
  m_fSpringStiffness = f;
  ApplyLimits();
}

void ezPxRevoluteJointComponent::SetSpringDamping(float f)
{
  m_fSpringDamping = f;
  ApplyLimits();
}

void ezPxRevoluteJointComponent::SetDriveVelocity(float f)
{
  m_fDriveVelocity = f;
  ApplyDrive();
}

void ezPxRevoluteJointComponent::SetDriveTorque(float f)
{
  m_fMaxDriveTorque = f;
  ApplyDrive();
}

void ezPxRevoluteJointComponent::CreateJointType(PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1, const PxTransform& localFrame1)
{
  m_pJoint = PxRevoluteJointCreate(*(ezPhysX::GetSingleton()->GetPhysXAPI()), actor0, localFrame0, actor1, localFrame1);

  ApplyLimits();
  ApplyDrive();
}

void ezPxRevoluteJointComponent::ApplyLimits()
{
  if (m_pJoint == nullptr)
    return;

  physx::PxRevoluteJoint* pJoint = static_cast<physx::PxRevoluteJoint*>(m_pJoint);

  pJoint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, m_LimitMode != ezPxJointLimitMode::NoLimit);

  if (m_LimitMode != ezPxJointLimitMode::NoLimit)
  {
    //pJoint->setConstraintFlag(PxConstraintFlag::eDRIVE_LIMITS_ARE_FORCES, false);

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

    pJoint->setLimit(limit);
  }
}

void ezPxRevoluteJointComponent::ApplyDrive()
{
  if (m_pJoint == nullptr)
    return;

  physx::PxRevoluteJoint* pJoint = static_cast<physx::PxRevoluteJoint*>(m_pJoint);

  pJoint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, m_bEnableDrive);

  if (m_bEnableDrive)
  {
    pJoint->setDriveVelocity(m_fDriveVelocity);
    pJoint->setDriveForceLimit(m_fMaxDriveTorque);
    //pJoint->setDriveGearRatio

    pJoint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_FREESPIN, !m_bEnableDriveBraking);
  }
}
