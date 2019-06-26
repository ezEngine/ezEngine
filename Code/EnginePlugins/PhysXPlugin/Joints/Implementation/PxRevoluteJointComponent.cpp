#include <PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Joints/PxRevoluteJointComponent.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxRevoluteJointComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("LimitRotation", m_bLimitRotation),
    EZ_MEMBER_PROPERTY("LowerLimit", m_LowerLimit)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(-360), ezAngle::Degree(+360))),
    EZ_MEMBER_PROPERTY("UpperLimit", m_UpperLimit)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(-360), ezAngle::Degree(+360))),
    EZ_MEMBER_PROPERTY("EnableDrive", m_bEnableDrive),
    EZ_MEMBER_PROPERTY("DriveVelocity", m_fDriveVelocity),
    EZ_MEMBER_PROPERTY("DriveBraking", m_bEnableDriveBraking),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxRevoluteJointComponent::ezPxRevoluteJointComponent() = default;
ezPxRevoluteJointComponent::~ezPxRevoluteJointComponent() = default;

void ezPxRevoluteJointComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_bLimitRotation;
  s << m_LowerLimit;
  s << m_UpperLimit;
}

void ezPxRevoluteJointComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_bLimitRotation;
  s >> m_LowerLimit;
  s >> m_UpperLimit;
}

PxJoint* ezPxRevoluteJointComponent::CreateJointType(PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1,
                                                     const PxTransform& localFrame1)
{
  PxRevoluteJoint* pJoint = PxRevoluteJointCreate(*(ezPhysX::GetSingleton()->GetPhysXAPI()), actor0, localFrame0, actor1, localFrame1);

  if (pJoint != nullptr)
  {
    if (m_bLimitRotation)
    {
      pJoint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);
      //pJoint->setConstraintFlag(PxConstraintFlag::eDRIVE_LIMITS_ARE_FORCES, false);

      float low = m_LowerLimit.GetRadian();
      float high = m_UpperLimit.GetRadian();

      if (low > high)
        ezMath::Swap(low, high);

      const float range = ezMath::Min(high - low, 1.99f * ezMath::BasicType<float>::Pi());
      high = low + range;

      PxJointAngularLimitPair limit(low, high);
      pJoint->setLimit(limit);
    }

    if (m_bEnableDrive)
    {
      pJoint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, true);
      pJoint->setDriveVelocity(m_fDriveVelocity);
      //pJoint->setDriveForceLimit
      //pJoint->setDriveGearRatio

      pJoint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_FREESPIN, !m_bEnableDriveBraking);
    }
  }

  return pJoint;
}

