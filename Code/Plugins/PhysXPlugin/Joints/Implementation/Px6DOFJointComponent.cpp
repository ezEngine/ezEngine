#include <PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Joints/Px6DOFJointComponent.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>

using namespace physx;

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezPxAxis, 1)
  EZ_BITFLAGS_CONSTANT(ezPxAxis::X),
  EZ_BITFLAGS_CONSTANT(ezPxAxis::Y),
  EZ_BITFLAGS_CONSTANT(ezPxAxis::Z),
EZ_END_STATIC_REFLECTED_BITFLAGS;

EZ_BEGIN_COMPONENT_TYPE(ezPx6DOFJointComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_BITFLAGS_MEMBER_PROPERTY("FreeLinearAxis", ezPxAxis, m_FreeLinearAxis),
    EZ_BITFLAGS_MEMBER_PROPERTY("FreeAngularAxis", ezPxAxis, m_FreeAngularAxis),
    EZ_MEMBER_PROPERTY("LinearStiffness", m_fLinearStiffness)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("LinearDamping", m_fLinearDamping)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("AngularStiffness", m_fAngularStiffness)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("AngularDamping", m_fAngularDamping)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPx6DOFJointComponent::ezPx6DOFJointComponent()
{
  m_fLinearStiffness = 0.0f;
  m_fLinearDamping = 0.0f;
  m_fAngularStiffness = 0.0f;
  m_fAngularDamping = 0.0f;
}


void ezPx6DOFJointComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_FreeLinearAxis;
  s << m_FreeAngularAxis;
  s << m_fLinearStiffness;
  s << m_fLinearDamping;
  s << m_fAngularStiffness;
  s << m_fAngularDamping;
}


void ezPx6DOFJointComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = stream.GetStream();

  s >> m_FreeLinearAxis;
  s >> m_FreeAngularAxis;
  s >> m_fLinearStiffness;
  s >> m_fLinearDamping;
  s >> m_fAngularStiffness;
  s >> m_fAngularDamping;
}

PxJoint* ezPx6DOFJointComponent::CreateJointType(PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1,
                                                     const PxTransform& localFrame1)
{
  PxD6Joint* pJoint = PxD6JointCreate(*(ezPhysX::GetSingleton()->GetPhysXAPI()), actor0, localFrame0, actor1, localFrame1);

  if (pJoint != nullptr)
  {
    pJoint->setMotion(PxD6Axis::eX, m_FreeLinearAxis.IsSet(ezPxAxis::X) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
    pJoint->setMotion(PxD6Axis::eY, m_FreeLinearAxis.IsSet(ezPxAxis::Y) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
    pJoint->setMotion(PxD6Axis::eZ, m_FreeLinearAxis.IsSet(ezPxAxis::Z) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);

    pJoint->setMotion(PxD6Axis::eTWIST, m_FreeAngularAxis.IsSet(ezPxAxis::X) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
    pJoint->setMotion(PxD6Axis::eSWING1, m_FreeAngularAxis.IsSet(ezPxAxis::Y) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
    pJoint->setMotion(PxD6Axis::eSWING2, m_FreeAngularAxis.IsSet(ezPxAxis::Z) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);

    pJoint->setDrive(PxD6Drive::eX, PxD6JointDrive(m_fLinearStiffness, m_fLinearDamping, PX_MAX_F32, true));
    pJoint->setDrive(PxD6Drive::eY, PxD6JointDrive(m_fLinearStiffness, m_fLinearDamping, PX_MAX_F32, true));
    pJoint->setDrive(PxD6Drive::eZ, PxD6JointDrive(m_fLinearStiffness, m_fLinearDamping, PX_MAX_F32, true));

    pJoint->setDrive(PxD6Drive::eSLERP, PxD6JointDrive(m_fAngularStiffness, m_fAngularDamping, PX_MAX_F32, true));
  }

  return pJoint;
}



EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Joints_Implementation_PxDistanceJointComponent);
