#include <JoltPlugin/JoltPluginPCH.h>

#if 0

#  include <Core/WorldSerializer/WorldReader.h>
#  include <Core/WorldSerializer/WorldWriter.h>
#  include <JoltPlugin/Constraints/Jolt6DOFConstraintComponent.h>
#  include <JoltPlugin/System/JoltCore.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezJoltAxis, 1)
  EZ_BITFLAGS_CONSTANT(ezJoltAxis::X),
  EZ_BITFLAGS_CONSTANT(ezJoltAxis::Y),
  EZ_BITFLAGS_CONSTANT(ezJoltAxis::Z),
EZ_END_STATIC_REFLECTED_BITFLAGS;

EZ_BEGIN_COMPONENT_TYPE(ezJolt6DOFConstraintComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_BITFLAGS_ACCESSOR_PROPERTY("FreeLinearAxis", ezJoltAxis, GetFreeLinearAxis, SetFreeLinearAxis),
    EZ_ENUM_ACCESSOR_PROPERTY("LinearLimitMode", ezJoltConstraintLimitMode, GetLinearLimitMode, SetLinearLimitMode),
    EZ_ACCESSOR_PROPERTY("LinearRangeX", GetLinearRangeX, SetLinearRangeX),
    EZ_ACCESSOR_PROPERTY("LinearRangeY", GetLinearRangeY, SetLinearRangeY),
    EZ_ACCESSOR_PROPERTY("LinearRangeZ", GetLinearRangeZ, SetLinearRangeZ),
    EZ_ACCESSOR_PROPERTY("LinearStiffness", GetLinearStiffness, SetLinearStiffness)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("LinearDamping", GetLinearDamping, SetLinearDamping)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_BITFLAGS_ACCESSOR_PROPERTY("FreeAngularAxis", ezJoltAxis, GetFreeAngularAxis, SetFreeAngularAxis),
    EZ_ENUM_ACCESSOR_PROPERTY("SwingLimitMode", ezJoltConstraintLimitMode, GetSwingLimitMode, SetSwingLimitMode),
    EZ_ACCESSOR_PROPERTY("SwingLimit", GetSwingLimit, SetSwingLimit)->AddAttributes(new ezClampValueAttribute(ezAngle(), ezAngle::MakeFromDegree(175))),
    EZ_ACCESSOR_PROPERTY("SwingStiffness", GetSwingStiffness, SetSwingStiffness)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SwingDamping", GetSwingDamping, SetSwingDamping)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ENUM_ACCESSOR_PROPERTY("TwistLimitMode", ezJoltConstraintLimitMode, GetTwistLimitMode, SetTwistLimitMode),
    EZ_ACCESSOR_PROPERTY("LowerTwistLimit", GetLowerTwistLimit, SetLowerTwistLimit)->AddAttributes(new ezClampValueAttribute(-ezAngle::MakeFromDegree(175), ezAngle::MakeFromDegree(175))),
    EZ_ACCESSOR_PROPERTY("UpperTwistLimit", GetUpperTwistLimit, SetUpperTwistLimit)->AddAttributes(new ezClampValueAttribute(-ezAngle::MakeFromDegree(175), ezAngle::MakeFromDegree(175))),
    EZ_ACCESSOR_PROPERTY("TwistStiffness", GetTwistStiffness, SetTwistStiffness)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("TwistDamping", GetTwistDamping, SetTwistDamping)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
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

ezJolt6DOFConstraintComponent::ezJolt6DOFConstraintComponent() = default;
ezJolt6DOFConstraintComponent::~ezJolt6DOFConstraintComponent() = default;

void ezJolt6DOFConstraintComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_FreeLinearAxis;
  s << m_FreeAngularAxis;
  s << m_fLinearStiffness;
  s << m_fLinearDamping;
  s << m_fSwingStiffness;
  s << m_fSwingDamping;

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

void ezJolt6DOFConstraintComponent::DeserializeComponent(ezWorldReader& stream)
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

void ezJolt6DOFConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  //EZ_ASSERT_DEV(localFrame0.isFinite() && localFrame0.isValid() && localFrame0.isSane(), "frame 0");
  //EZ_ASSERT_DEV(localFrame1.isFinite() && localFrame1.isValid() && localFrame1.isSane(), "frame 1");
  //
  //  m_pJoint = PxD6JointCreate(*(ezJolt::GetSingleton()->GetJoltAPI()), actor0, localFrame0, actor1, localFrame1);
}

void ezJolt6DOFConstraintComponent::ApplySettings()
{
  ezJoltConstraintComponent::ApplySettings();

  //JoltD6Joint* pJoint = static_cast<PxD6Joint*>(m_pJoint);

  //if (m_LinearLimitMode == ezJoltConstraintLimitMode::NoLimit)
  //{
  //  pJoint->setMotion(PxD6Axis::eX, m_FreeLinearAxis.IsSet(ezJoltAxis::X) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eY, m_FreeLinearAxis.IsSet(ezJoltAxis::Y) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eZ, m_FreeLinearAxis.IsSet(ezJoltAxis::Z) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //}
  //else
  //{
  //  auto freeAxis = m_FreeLinearAxis;

  //  if (m_LinearLimitMode == ezJoltConstraintLimitMode::HardLimit)
  //  {
  //    if (ezMath::IsEqual(m_vLinearRangeX.x, m_vLinearRangeX.y, 0.05f))
  //      freeAxis.Remove(ezJoltAxis::X);
  //    if (ezMath::IsEqual(m_vLinearRangeY.x, m_vLinearRangeY.y, 0.05f))
  //      freeAxis.Remove(ezJoltAxis::Y);
  //    if (ezMath::IsEqual(m_vLinearRangeZ.x, m_vLinearRangeZ.y, 0.05f))
  //      freeAxis.Remove(ezJoltAxis::Z);
  //  }

  //  pJoint->setMotion(PxD6Axis::eX, freeAxis.IsSet(ezJoltAxis::X) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eY, freeAxis.IsSet(ezJoltAxis::Y) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eZ, freeAxis.IsSet(ezJoltAxis::Z) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);

  //  PxJointLinearLimitPair l(0, 0, PxSpring(0, 0));

  //  if (m_LinearLimitMode == ezJoltConstraintLimitMode::SoftLimit)
  //  {
  //    l.stiffness = m_fLinearStiffness;
  //    l.damping = m_fLinearDamping;
  //  }
  //  else
  //  {
  //    l.restitution = m_fLinearStiffness;
  //    l.bounceThreshold = m_fLinearDamping;
  //  }

  //  if (freeAxis.IsSet(ezJoltAxis::X))
  //  {
  //    l.lower = m_vLinearRangeX.x;
  //    l.upper = m_vLinearRangeX.y;

  //    if (l.lower > l.upper)
  //      ezMath::Swap(l.lower, l.upper);

  //    pJoint->setLinearLimit(PxD6Axis::eX, l);
  //  }

  //  if (freeAxis.IsSet(ezJoltAxis::Y))
  //  {
  //    l.lower = m_vLinearRangeY.x;
  //    l.upper = m_vLinearRangeY.y;

  //    if (l.lower > l.upper)
  //      ezMath::Swap(l.lower, l.upper);

  //    pJoint->setLinearLimit(PxD6Axis::eY, l);
  //  }

  //  if (freeAxis.IsSet(ezJoltAxis::Z))
  //  {
  //    l.lower = m_vLinearRangeZ.x;
  //    l.upper = m_vLinearRangeZ.y;

  //    if (l.lower > l.upper)
  //      ezMath::Swap(l.lower, l.upper);

  //    pJoint->setLinearLimit(PxD6Axis::eZ, l);
  //  }
  //}


  //if (m_SwingLimitMode == ezJoltConstraintLimitMode::NoLimit)
  //{
  //  pJoint->setMotion(PxD6Axis::eSWING1, m_FreeAngularAxis.IsSet(ezJoltAxis::Y) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eSWING2, m_FreeAngularAxis.IsSet(ezJoltAxis::Z) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //}
  //else
  //{
  //  auto freeAxis = m_FreeAngularAxis;

  //  if (m_SwingLimitMode == ezJoltConstraintLimitMode::HardLimit)
  //  {
  //    if (ezMath::IsZero(m_SwingLimit.GetDegree(), 1.0f))
  //    {
  //      freeAxis.Remove(ezJoltAxis::Y);
  //      freeAxis.Remove(ezJoltAxis::Z);
  //    }
  //  }

  //  pJoint->setMotion(PxD6Axis::eSWING1, freeAxis.IsSet(ezJoltAxis::Y) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eSWING2, freeAxis.IsSet(ezJoltAxis::Z) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);

  //  if (freeAxis.IsAnySet(ezJoltAxis::Y | ezJoltAxis::Z))
  //  {
  //    const float fSwingLimit = ezMath::Max(ezAngle::MakeFromDegree(0.5f).GetRadian(), m_SwingLimit.GetRadian());

  //    PxJointLimitCone l(fSwingLimit, fSwingLimit);

  //    if (m_SwingLimitMode == ezJoltConstraintLimitMode::SoftLimit)
  //    {
  //      l.stiffness = m_fSwingStiffness;
  //      l.damping = m_fSwingDamping;
  //    }
  //    else
  //    {
  //      l.restitution = m_fSwingStiffness;
  //      l.bounceThreshold = m_fSwingDamping;
  //    }

  //    pJoint->setSwingLimit(l);
  //  }
  //}

  //if (m_TwistLimitMode == ezJoltConstraintLimitMode::NoLimit)
  //{
  //  pJoint->setMotion(PxD6Axis::eTWIST, m_FreeAngularAxis.IsSet(ezJoltAxis::X) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //}
  //else
  //{
  //  auto freeAxis = m_FreeAngularAxis;

  //  if (m_SwingLimitMode == ezJoltConstraintLimitMode::HardLimit)
  //  {
  //    if (ezMath::IsEqual(m_LowerTwistLimit.GetDegree(), m_UpperTwistLimit.GetDegree(), 1.0f))
  //    {
  //      freeAxis.Remove(ezJoltAxis::X);
  //    }
  //  }

  //  pJoint->setMotion(PxD6Axis::eTWIST, freeAxis.IsSet(ezJoltAxis::X) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);

  //  if (freeAxis.IsSet(ezJoltAxis::X))
  //  {
  //    PxJointAngularLimitPair l(m_LowerTwistLimit.GetRadian(), m_UpperTwistLimit.GetRadian());

  //    if (l.lower > l.upper)
  //    {
  //      ezMath::Swap(l.lower, l.upper);
  //    }

  //    if (ezMath::IsEqual(l.lower, l.upper, ezAngle::MakeFromDegree(0.5f).GetRadian()))
  //    {
  //      l.lower -= ezAngle::MakeFromDegree(0.5f).GetRadian();
  //      l.upper += ezAngle::MakeFromDegree(0.5f).GetRadian();
  //    }

  //    if (m_TwistLimitMode == ezJoltConstraintLimitMode::SoftLimit)
  //    {
  //      l.stiffness = m_fTwistStiffness;
  //      l.damping = m_fTwistDamping;
  //    }
  //    else
  //    {
  //      l.restitution = m_fTwistStiffness;
  //      l.bounceThreshold = m_fTwistDamping;
  //    }

  //    pJoint->setTwistLimit(l);
  //  }
  //}
}

void ezJolt6DOFConstraintComponent::SetFreeLinearAxis(ezBitflags<ezJoltAxis> flags)
{
  m_FreeLinearAxis = flags;
  QueueApplySettings();
}

void ezJolt6DOFConstraintComponent::SetFreeAngularAxis(ezBitflags<ezJoltAxis> flags)
{
  m_FreeAngularAxis = flags;
  QueueApplySettings();
}

void ezJolt6DOFConstraintComponent::SetLinearLimitMode(ezJoltConstraintLimitMode::Enum mode)
{
  m_LinearLimitMode = mode;
  QueueApplySettings();
}

void ezJolt6DOFConstraintComponent::SetLinearRangeX(const ezVec2& value)
{
  m_vLinearRangeX = value;
  QueueApplySettings();
}

void ezJolt6DOFConstraintComponent::SetLinearRangeY(const ezVec2& value)
{
  m_vLinearRangeY = value;
  QueueApplySettings();
}

void ezJolt6DOFConstraintComponent::SetLinearRangeZ(const ezVec2& value)
{
  m_vLinearRangeZ = value;
  QueueApplySettings();
}

void ezJolt6DOFConstraintComponent::SetLinearStiffness(float f)
{
  m_fLinearStiffness = f;
  QueueApplySettings();
}

void ezJolt6DOFConstraintComponent::SetLinearDamping(float f)
{
  m_fLinearDamping = f;
  QueueApplySettings();
}

void ezJolt6DOFConstraintComponent::SetSwingLimitMode(ezJoltConstraintLimitMode::Enum mode)
{
  m_SwingLimitMode = mode;
  QueueApplySettings();
}

void ezJolt6DOFConstraintComponent::SetSwingLimit(ezAngle f)
{
  m_SwingLimit = f;
  QueueApplySettings();
}

void ezJolt6DOFConstraintComponent::SetSwingStiffness(float f)
{
  m_fSwingStiffness = f;
  QueueApplySettings();
}

void ezJolt6DOFConstraintComponent::SetSwingDamping(float f)
{
  m_fSwingDamping = f;
  QueueApplySettings();
}

void ezJolt6DOFConstraintComponent::SetTwistLimitMode(ezJoltConstraintLimitMode::Enum mode)
{
  m_TwistLimitMode = mode;
  QueueApplySettings();
}

void ezJolt6DOFConstraintComponent::SetLowerTwistLimit(ezAngle f)
{
  m_LowerTwistLimit = f;
  QueueApplySettings();
}

void ezJolt6DOFConstraintComponent::SetUpperTwistLimit(ezAngle f)
{
  m_UpperTwistLimit = f;
  QueueApplySettings();
}

void ezJolt6DOFConstraintComponent::SetTwistStiffness(float f)
{
  m_fTwistStiffness = f;
  QueueApplySettings();
}

void ezJolt6DOFConstraintComponent::SetTwistDamping(float f)
{
  m_fTwistDamping = f;
  QueueApplySettings();
}

#endif


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_Jolt6DOFConstraintComponent);
