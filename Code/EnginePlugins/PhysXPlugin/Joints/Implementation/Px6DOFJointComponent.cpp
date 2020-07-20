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

EZ_BEGIN_COMPONENT_TYPE(ezPx6DOFJointComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_BITFLAGS_MEMBER_PROPERTY("FreeLinearAxis", ezPxAxis, m_FreeLinearAxis),
    EZ_ENUM_ACCESSOR_PROPERTY("LinearLimitMode", ezPxJointLimitMode, GetLinearLimitMode, SetLinearLimitMode),
    EZ_ACCESSOR_PROPERTY("LinearRangeX", GetLinearRangeX, SetLinearRangeX),
    EZ_ACCESSOR_PROPERTY("LinearRangeY", GetLinearRangeY, SetLinearRangeY),
    EZ_ACCESSOR_PROPERTY("LinearRangeZ", GetLinearRangeZ, SetLinearRangeZ),
    EZ_ACCESSOR_PROPERTY("LinearStiffness", GetLinearStiffness, SetLinearStiffness)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("LinearDamping", GetLinearDamping, SetLinearDamping)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_BITFLAGS_MEMBER_PROPERTY("FreeAngularAxis", ezPxAxis, m_FreeAngularAxis),
    EZ_ENUM_ACCESSOR_PROPERTY("SwingLimitMode", ezPxJointLimitMode, GetSwingLimitMode, SetSwingLimitMode),
    EZ_ACCESSOR_PROPERTY("SwingLimit", GetSwingLimit, SetSwingLimit)->AddAttributes(new ezClampValueAttribute(ezAngle(), ezAngle::Degree(175))),
    EZ_ACCESSOR_PROPERTY("SwingStiffness", GetSwingStiffness, SetSwingStiffness)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SwingDamping", GetSwingDamping, SetSwingDamping)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ENUM_ACCESSOR_PROPERTY("TwistLimitMode", ezPxJointLimitMode, GetTwistLimitMode, SetTwistLimitMode),
    EZ_ACCESSOR_PROPERTY("LowerTwistLimit", GetLowerTwistLimit, SetLowerTwistLimit)->AddAttributes(new ezClampValueAttribute(-ezAngle::Degree(175), ezAngle::Degree(175))),
    EZ_ACCESSOR_PROPERTY("UpperTwistLimit", GetUpperTwistLimit, SetUpperTwistLimit)->AddAttributes(new ezClampValueAttribute(-ezAngle::Degree(175), ezAngle::Degree(175))),
    EZ_ACCESSOR_PROPERTY("TwistStiffness", GetTwistStiffness, SetTwistStiffness)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("TwistDamping", GetTwistDamping, SetTwistDamping)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
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

ezPx6DOFJointComponent::ezPx6DOFJointComponent() = default;
ezPx6DOFJointComponent::~ezPx6DOFJointComponent() = default;

void ezPx6DOFJointComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_FreeLinearAxis;
  s << m_FreeAngularAxis;
  s << m_fLinearStiffness;
  s << m_fLinearDamping;
  s << m_fSwingStiffness;
  s << m_fSwingDamping;

  // version 2
  s << m_LinearLimitMode;
  s << m_vLinearRangeX;
  s << m_vLinearRangeY;
  s << m_vLinearRangeZ;

  s << m_SwingLimitMode;
  s << m_SwingLimit;

  s << m_TwistLimitMode;
  s << m_LowerTwistLimit;
  s << m_UpperTwistLimit;
  s << m_fTwistStiffness;
  s << m_fTwistDamping;
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
  s >> m_fSwingStiffness;
  s >> m_fSwingDamping;

  if (uiVersion >= 2)
  {
    s >> m_LinearLimitMode;
    s >> m_vLinearRangeX;
    s >> m_vLinearRangeY;
    s >> m_vLinearRangeZ;
    s >> m_SwingLimitMode;
    s >> m_SwingLimit;

    s >> m_TwistLimitMode;
    s >> m_LowerTwistLimit;
    s >> m_UpperTwistLimit;
    s >> m_fTwistStiffness;
    s >> m_fTwistDamping;
  }
}

void ezPx6DOFJointComponent::CreateJointType(PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1,
  const PxTransform& localFrame1)
{
  m_pJoint = PxD6JointCreate(*(ezPhysX::GetSingleton()->GetPhysXAPI()), actor0, localFrame0, actor1, localFrame1);
}

void ezPx6DOFJointComponent::ApplySettings()
{
  ezPxJointComponent::ApplySettings();

  PxD6Joint* pJoint = static_cast<PxD6Joint*>(m_pJoint);

  if (m_LinearLimitMode == ezPxJointLimitMode::NoLimit)
  {
    pJoint->setMotion(PxD6Axis::eX, m_FreeLinearAxis.IsSet(ezPxAxis::X) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
    pJoint->setMotion(PxD6Axis::eY, m_FreeLinearAxis.IsSet(ezPxAxis::Y) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
    pJoint->setMotion(PxD6Axis::eZ, m_FreeLinearAxis.IsSet(ezPxAxis::Z) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  }
  else
  {
    auto freeAxis = m_FreeLinearAxis;

    if (m_LinearLimitMode == ezPxJointLimitMode::HardLimit)
    {
      if (ezMath::IsEqual(m_vLinearRangeX.x, m_vLinearRangeX.y, 0.05f))
        freeAxis.Remove(ezPxAxis::X);
      if (ezMath::IsEqual(m_vLinearRangeY.x, m_vLinearRangeY.y, 0.05f))
        freeAxis.Remove(ezPxAxis::Y);
      if (ezMath::IsEqual(m_vLinearRangeZ.x, m_vLinearRangeZ.y, 0.05f))
        freeAxis.Remove(ezPxAxis::Z);
    }

    pJoint->setMotion(PxD6Axis::eX, freeAxis.IsSet(ezPxAxis::X) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
    pJoint->setMotion(PxD6Axis::eY, freeAxis.IsSet(ezPxAxis::Y) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
    pJoint->setMotion(PxD6Axis::eZ, freeAxis.IsSet(ezPxAxis::Z) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);

    PxJointLinearLimitPair l(0, 0, PxSpring(0, 0));

    if (m_LinearLimitMode == ezPxJointLimitMode::SoftLimit)
    {
      l.stiffness = m_fLinearStiffness;
      l.damping = m_fLinearDamping;
    }
    else
    {
      l.restitution = m_fLinearStiffness;
      l.bounceThreshold = m_fLinearDamping;
    }

    if (freeAxis.IsSet(ezPxAxis::X))
    {
      l.lower = m_vLinearRangeX.x;
      l.upper = m_vLinearRangeX.y;

      if (l.lower > l.upper)
        ezMath::Swap(l.lower, l.upper);

      pJoint->setLinearLimit(PxD6Axis::eX, l);
    }

    if (freeAxis.IsSet(ezPxAxis::Y))
    {
      l.lower = m_vLinearRangeY.x;
      l.upper = m_vLinearRangeY.y;

      if (l.lower > l.upper)
        ezMath::Swap(l.lower, l.upper);

      pJoint->setLinearLimit(PxD6Axis::eY, l);
    }

    if (freeAxis.IsSet(ezPxAxis::Z))
    {
      l.lower = m_vLinearRangeZ.x;
      l.upper = m_vLinearRangeZ.y;

      if (l.lower > l.upper)
        ezMath::Swap(l.lower, l.upper);

      pJoint->setLinearLimit(PxD6Axis::eZ, l);
    }
  }


  if (m_SwingLimitMode == ezPxJointLimitMode::NoLimit)
  {
    pJoint->setMotion(PxD6Axis::eSWING1, m_FreeAngularAxis.IsSet(ezPxAxis::Y) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
    pJoint->setMotion(PxD6Axis::eSWING2, m_FreeAngularAxis.IsSet(ezPxAxis::Z) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  }
  else
  {
    auto freeAxis = m_FreeAngularAxis;

    if (m_SwingLimitMode == ezPxJointLimitMode::HardLimit)
    {
      if (ezMath::IsZero(m_SwingLimit.GetDegree(), 1.0f))
      {
        freeAxis.Remove(ezPxAxis::Y);
        freeAxis.Remove(ezPxAxis::Z);
      }
    }

    pJoint->setMotion(PxD6Axis::eSWING1, freeAxis.IsSet(ezPxAxis::Y) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
    pJoint->setMotion(PxD6Axis::eSWING2, freeAxis.IsSet(ezPxAxis::Z) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);

    if (freeAxis.IsAnySet(ezPxAxis::Y | ezPxAxis::Z))
    {
      PxJointLimitCone l(m_SwingLimit.GetRadian(), m_SwingLimit.GetRadian());

      if (m_SwingLimitMode == ezPxJointLimitMode::SoftLimit)
      {
        l.stiffness = m_fSwingStiffness;
        l.damping = m_fSwingDamping;
      }
      else
      {
        l.restitution = m_fSwingStiffness;
        l.bounceThreshold = m_fSwingDamping;
      }

      pJoint->setSwingLimit(l);
    }
  }

  if (m_TwistLimitMode == ezPxJointLimitMode::NoLimit)
  {
    pJoint->setMotion(PxD6Axis::eTWIST, m_FreeAngularAxis.IsSet(ezPxAxis::X) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  }
  else
  {
    auto freeAxis = m_FreeAngularAxis;

    if (m_SwingLimitMode == ezPxJointLimitMode::HardLimit)
    {
      if (ezMath::IsEqual(m_LowerTwistLimit.GetDegree(), m_UpperTwistLimit.GetDegree(), 1.0f))
      {
        freeAxis.Remove(ezPxAxis::X);
      }
    }

    pJoint->setMotion(PxD6Axis::eTWIST, freeAxis.IsSet(ezPxAxis::X) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);

    if (freeAxis.IsSet(ezPxAxis::X))
    {
      PxJointAngularLimitPair l(m_LowerTwistLimit.GetRadian(), m_UpperTwistLimit.GetRadian());

      if (l.lower > l.upper)
      {
        ezMath::Swap(l.lower, l.upper);
      }

      if (m_TwistLimitMode == ezPxJointLimitMode::SoftLimit)
      {
        l.stiffness = m_fTwistStiffness;
        l.damping = m_fTwistDamping;
      }
      else
      {
        l.restitution = m_fTwistStiffness;
        l.bounceThreshold = m_fTwistDamping;
      }

      pJoint->setTwistLimit(l);
    }
  }

  // the linear and angular springs appear to have the exact same effect
  // it is unclear from the PhysX documentation, whether there is any difference in behavior
  //pJoint->setDrive(PxD6Drive::eX, PxD6JointDrive(m_fLinearStiffness, m_fLinearDamping, PX_MAX_F32, true));
  //pJoint->setDrive(PxD6Drive::eY, PxD6JointDrive(m_fLinearStiffness, m_fLinearDamping, PX_MAX_F32, true));
  //pJoint->setDrive(PxD6Drive::eZ, PxD6JointDrive(m_fLinearStiffness, m_fLinearDamping, PX_MAX_F32, true));
  //pJoint->setDrive(PxD6Drive::eSLERP, PxD6JointDrive(m_fAngularStiffness, m_fAngularDamping, PX_MAX_F32, true));
}

void ezPx6DOFJointComponent::SetLinearLimitMode(ezPxJointLimitMode::Enum mode)
{
  m_LinearLimitMode = mode;
  QueueApplySettings();
}

void ezPx6DOFJointComponent::SetLinearRangeX(const ezVec2& value)
{
  m_vLinearRangeX = value;
  QueueApplySettings();
}

void ezPx6DOFJointComponent::SetLinearRangeY(const ezVec2& value)
{
  m_vLinearRangeY = value;
  QueueApplySettings();
}

void ezPx6DOFJointComponent::SetLinearRangeZ(const ezVec2& value)
{
  m_vLinearRangeZ = value;
  QueueApplySettings();
}

void ezPx6DOFJointComponent::SetLinearStiffness(float f)
{
  m_fLinearStiffness = f;
  QueueApplySettings();
}

void ezPx6DOFJointComponent::SetLinearDamping(float f)
{
  m_fLinearDamping = f;
  QueueApplySettings();
}

void ezPx6DOFJointComponent::SetSwingLimitMode(ezPxJointLimitMode::Enum mode)
{
  m_SwingLimitMode = mode;
  QueueApplySettings();
}

void ezPx6DOFJointComponent::SetSwingLimit(ezAngle f)
{
  m_SwingLimit = f;
  QueueApplySettings();
}

void ezPx6DOFJointComponent::SetSwingStiffness(float f)
{
  m_fSwingStiffness = f;
  QueueApplySettings();
}

void ezPx6DOFJointComponent::SetSwingDamping(float f)
{
  m_fSwingDamping = f;
  QueueApplySettings();
}

void ezPx6DOFJointComponent::SetTwistLimitMode(ezPxJointLimitMode::Enum mode)
{
  m_TwistLimitMode = mode;
  QueueApplySettings();
}

void ezPx6DOFJointComponent::SetLowerTwistLimit(ezAngle f)
{
  m_LowerTwistLimit = f;
  QueueApplySettings();
}

void ezPx6DOFJointComponent::SetUpperTwistLimit(ezAngle f)
{
  m_UpperTwistLimit = f;
  QueueApplySettings();
}

void ezPx6DOFJointComponent::SetTwistStiffness(float f)
{
  m_fTwistStiffness = f;
  QueueApplySettings();
}

void ezPx6DOFJointComponent::SetTwistDamping(float f)
{
  m_fTwistDamping = f;
  QueueApplySettings();
}

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Joints_Implementation_Px6DOFJointComponent);
