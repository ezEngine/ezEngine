#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/JointOverrideComponent.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJointOverrideComponent, 1, ezComponentMode::Dynamic);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("JointName", GetJointName, SetJointName),
    EZ_MEMBER_PROPERTY("OverridePosition", m_bOverridePosition)->AddAttributes(new ezDefaultValueAttribute(false)),
    EZ_MEMBER_PROPERTY("OverrideRotation", m_bOverrideRotation)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("OverrideScale", m_bOverrideScale)->AddAttributes(new ezDefaultValueAttribute(false)),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgAnimationPosePreparing, OnAnimationPosePreparing)
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezJointOverrideComponent::ezJointOverrideComponent() = default;
ezJointOverrideComponent::~ezJointOverrideComponent() = default;

void ezJointOverrideComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sJointToOverride;
  s << m_bOverridePosition;
  s << m_bOverrideRotation;
  s << m_bOverrideScale;
}

void ezJointOverrideComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_sJointToOverride;
  s >> m_bOverridePosition;
  s >> m_bOverrideRotation;
  s >> m_bOverrideScale;

  m_uiJointIndex = ezInvalidJointIndex;
}

void ezJointOverrideComponent::SetJointName(const char* szName)
{
  m_sJointToOverride.Assign(szName);
  m_uiJointIndex = ezInvalidJointIndex;
}

const char* ezJointOverrideComponent::GetJointName() const
{
  return m_sJointToOverride.GetData();
}

void ezJointOverrideComponent::OnAnimationPosePreparing(ezMsgAnimationPosePreparing& msg) const
{
  using namespace ozz::math;

  if (m_uiJointIndex == ezInvalidJointIndex)
  {
    m_uiJointIndex = msg.m_pSkeleton->FindJointByName(m_sJointToOverride);
  }

  if (m_uiJointIndex == ezInvalidJointIndex)
    return;

  const int soaIdx = m_uiJointIndex / 4;
  const int soaSubIdx = m_uiJointIndex % 4;

  const ezTransform t = GetOwner()->GetLocalTransform();

  if (m_bOverridePosition)
  {
    SimdFloat4 vx = ozz::math::simd_float4::Load1(t.m_vPosition.x);
    SimdFloat4 vy = ozz::math::simd_float4::Load1(t.m_vPosition.y);
    SimdFloat4 vz = ozz::math::simd_float4::Load1(t.m_vPosition.z);

    auto val = msg.m_LocalTransforms[soaIdx].translation;

    val.x = ozz::math::SetI(val.x, vx, soaSubIdx);
    val.y = ozz::math::SetI(val.y, vy, soaSubIdx);
    val.z = ozz::math::SetI(val.z, vz, soaSubIdx);

    msg.m_LocalTransforms[soaIdx].translation = val;
  }

  if (m_bOverrideRotation)
  {
    SimdFloat4 vx = ozz::math::simd_float4::Load1(t.m_qRotation.x);
    SimdFloat4 vy = ozz::math::simd_float4::Load1(t.m_qRotation.y);
    SimdFloat4 vz = ozz::math::simd_float4::Load1(t.m_qRotation.z);
    SimdFloat4 vw = ozz::math::simd_float4::Load1(t.m_qRotation.w);

    SoaQuaternion val = msg.m_LocalTransforms[soaIdx].rotation;

    val.x = ozz::math::SetI(val.x, vx, soaSubIdx);
    val.y = ozz::math::SetI(val.y, vy, soaSubIdx);
    val.z = ozz::math::SetI(val.z, vz, soaSubIdx);
    val.w = ozz::math::SetI(val.w, vw, soaSubIdx);

    msg.m_LocalTransforms[soaIdx].rotation = val;
  }

  if (m_bOverrideScale)
  {
    SimdFloat4 vx = ozz::math::simd_float4::Load1(t.m_vScale.x);
    SimdFloat4 vy = ozz::math::simd_float4::Load1(t.m_vScale.y);
    SimdFloat4 vz = ozz::math::simd_float4::Load1(t.m_vScale.z);

    auto val = msg.m_LocalTransforms[soaIdx].scale;

    val.x = ozz::math::SetI(val.x, vx, soaSubIdx);
    val.y = ozz::math::SetI(val.y, vy, soaSubIdx);
    val.z = ozz::math::SetI(val.z, vz, soaSubIdx);

    msg.m_LocalTransforms[soaIdx].scale = val;
  }
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_JointOverrideComponent);
