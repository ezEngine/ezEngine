#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/JointOverrideComponent.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJointOverrideComponent, 2, ezComponentMode::Dynamic);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("JointName", GetJointName, SetJointName),
    EZ_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
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
  s << m_fWeight;
}

void ezJointOverrideComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_sJointToOverride;
  s >> m_bOverridePosition;
  s >> m_bOverrideRotation;
  s >> m_bOverrideScale;

  if (uiVersion >= 2)
  {
    s >> m_fWeight;
  }

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

  if (m_fWeight <= 0.0f)
    return;

  if (m_uiJointIndex == ezInvalidJointIndex)
  {
    m_uiJointIndex = msg.m_pSkeleton->FindJointByName(m_sJointToOverride);
  }

  if (m_uiJointIndex == ezInvalidJointIndex)
    return;

  const int soaIdx = m_uiJointIndex / 4;
  const int soaSubIdx = m_uiJointIndex % 4;

  const ezTransform t = GetOwner()->GetLocalTransform();
  const float fWeight = ezMath::Clamp(m_fWeight, 0.0f, 1.0f);

  if (m_bOverridePosition)
  {
    auto val = msg.m_LocalTransforms[soaIdx].translation;

    // Extract current position for this joint
    float currentPos[4];
    ozz::math::StorePtrU(val.x, currentPos);
    const float currentX = currentPos[soaSubIdx];
    ozz::math::StorePtrU(val.y, currentPos);
    const float currentY = currentPos[soaSubIdx];
    ozz::math::StorePtrU(val.z, currentPos);
    const float currentZ = currentPos[soaSubIdx];

    // Blend between current and override position
    const float blendedX = ezMath::Lerp(currentX, t.m_vPosition.x, fWeight);
    const float blendedY = ezMath::Lerp(currentY, t.m_vPosition.y, fWeight);
    const float blendedZ = ezMath::Lerp(currentZ, t.m_vPosition.z, fWeight);

    SimdFloat4 vx = ozz::math::simd_float4::Load1(blendedX);
    SimdFloat4 vy = ozz::math::simd_float4::Load1(blendedY);
    SimdFloat4 vz = ozz::math::simd_float4::Load1(blendedZ);

    val.x = ozz::math::SetI(val.x, vx, soaSubIdx);
    val.y = ozz::math::SetI(val.y, vy, soaSubIdx);
    val.z = ozz::math::SetI(val.z, vz, soaSubIdx);

    msg.m_LocalTransforms[soaIdx].translation = val;
  }

  if (m_bOverrideRotation)
  {
    SoaQuaternion val = msg.m_LocalTransforms[soaIdx].rotation;

    // Extract current rotation for this joint
    float currentRot[4];
    ozz::math::StorePtrU(val.x, currentRot);
    const float currentX = currentRot[soaSubIdx];
    ozz::math::StorePtrU(val.y, currentRot);
    const float currentY = currentRot[soaSubIdx];
    ozz::math::StorePtrU(val.z, currentRot);
    const float currentZ = currentRot[soaSubIdx];
    ozz::math::StorePtrU(val.w, currentRot);
    const float currentW = currentRot[soaSubIdx];

    ezQuat currentQuat(currentX, currentY, currentZ, currentW);
    ezQuat overrideQuat = t.m_qRotation;

    // Slerp between current and override rotation
    ezQuat blendedQuat = ezQuat::MakeSlerp(currentQuat, overrideQuat, fWeight);

    SimdFloat4 vx = ozz::math::simd_float4::Load1(blendedQuat.x);
    SimdFloat4 vy = ozz::math::simd_float4::Load1(blendedQuat.y);
    SimdFloat4 vz = ozz::math::simd_float4::Load1(blendedQuat.z);
    SimdFloat4 vw = ozz::math::simd_float4::Load1(blendedQuat.w);

    val.x = ozz::math::SetI(val.x, vx, soaSubIdx);
    val.y = ozz::math::SetI(val.y, vy, soaSubIdx);
    val.z = ozz::math::SetI(val.z, vz, soaSubIdx);
    val.w = ozz::math::SetI(val.w, vw, soaSubIdx);

    msg.m_LocalTransforms[soaIdx].rotation = val;
  }

  if (m_bOverrideScale)
  {
    auto val = msg.m_LocalTransforms[soaIdx].scale;

    // Extract current scale for this joint
    float currentScale[4];
    ozz::math::StorePtrU(val.x, currentScale);
    const float currentX = currentScale[soaSubIdx];
    ozz::math::StorePtrU(val.y, currentScale);
    const float currentY = currentScale[soaSubIdx];
    ozz::math::StorePtrU(val.z, currentScale);
    const float currentZ = currentScale[soaSubIdx];

    // Blend between current and override scale
    const float blendedX = ezMath::Lerp(currentX, t.m_vScale.x, fWeight);
    const float blendedY = ezMath::Lerp(currentY, t.m_vScale.y, fWeight);
    const float blendedZ = ezMath::Lerp(currentZ, t.m_vScale.z, fWeight);

    SimdFloat4 vx = ozz::math::simd_float4::Load1(blendedX);
    SimdFloat4 vy = ozz::math::simd_float4::Load1(blendedY);
    SimdFloat4 vz = ozz::math::simd_float4::Load1(blendedZ);

    val.x = ozz::math::SetI(val.x, vx, soaSubIdx);
    val.y = ozz::math::SetI(val.y, vy, soaSubIdx);
    val.z = ozz::math::SetI(val.z, vz, soaSubIdx);

    msg.m_LocalTransforms[soaIdx].scale = val;
  }
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_JointOverrideComponent);
