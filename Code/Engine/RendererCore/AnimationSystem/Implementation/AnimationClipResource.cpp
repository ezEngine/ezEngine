#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/animation_optimizer.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipResource, 1, ezRTTIDefaultAllocator<ezAnimationClipResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezAnimationClipResource);
// clang-format on

ezAnimationClipResource::ezAnimationClipResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezAnimationClipResource, ezAnimationClipResourceDescriptor)
{
  m_pDescriptor = EZ_DEFAULT_NEW(ezAnimationClipResourceDescriptor);
  *m_pDescriptor = std::move(descriptor);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

ezResourceLoadDesc ezAnimationClipResource::UnloadData(Unload WhatToUnload)
{
  m_pDescriptor.Clear();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezAnimationClipResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezAnimationClipResource::UpdateContent", GetResourceDescription().GetData());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_pDescriptor = EZ_DEFAULT_NEW(ezAnimationClipResourceDescriptor);
  m_pDescriptor->Deserialize(*Stream).IgnoreResult();

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezAnimationClipResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezAnimationClipResource);

  if (m_pDescriptor)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += static_cast<ezUInt32>(m_pDescriptor->GetHeapMemoryUsage());
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct ezAnimationClipResourceDescriptor::OzzImpl
{
  struct CachedAnim
  {
    ezUInt32 m_uiResourceChangeCounter = 0;
    ozz::unique_ptr<ozz::animation::Animation> m_pAnim;
  };

  ezMap<const ezSkeletonResource*, CachedAnim> m_MappedOzzAnimations;
};

ezAnimationClipResourceDescriptor::ezAnimationClipResourceDescriptor()
{
  m_OzzImpl = EZ_DEFAULT_NEW(OzzImpl);
}

ezAnimationClipResourceDescriptor::ezAnimationClipResourceDescriptor(ezAnimationClipResourceDescriptor&& rhs)
{
  *this = std::move(rhs);
}

ezAnimationClipResourceDescriptor::~ezAnimationClipResourceDescriptor() = default;

void ezAnimationClipResourceDescriptor::operator=(ezAnimationClipResourceDescriptor&& rhs) noexcept
{
  m_OzzImpl = std::move(rhs.m_OzzImpl);

  m_JointInfos = std::move(rhs.m_JointInfos);
  m_Transforms = std::move(rhs.m_Transforms);
  m_uiNumTotalPositions = rhs.m_uiNumTotalPositions;
  m_uiNumTotalRotations = rhs.m_uiNumTotalRotations;
  m_uiNumTotalScales = rhs.m_uiNumTotalScales;
  m_Duration = rhs.m_Duration;
}

ezResult ezAnimationClipResourceDescriptor::Serialize(ezStreamWriter& stream) const
{
  stream.WriteVersion(9);

  const ezUInt16 uiNumJoints = static_cast<ezUInt16>(m_JointInfos.GetCount());
  stream << uiNumJoints;
  for (ezUInt32 i = 0; i < m_JointInfos.GetCount(); ++i)
  {
    const auto& val = m_JointInfos.GetValue(i);

    stream << m_JointInfos.GetKey(i);
    stream << val.m_uiPositionIdx;
    stream << val.m_uiPositionCount;
    stream << val.m_uiRotationIdx;
    stream << val.m_uiRotationCount;
    stream << val.m_uiScaleIdx;
    stream << val.m_uiScaleCount;
  }

  stream << m_Duration;
  stream << m_uiNumTotalPositions;
  stream << m_uiNumTotalRotations;
  stream << m_uiNumTotalScales;

  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_Transforms));

  stream << m_vConstantRootMotion;

  m_EventTrack.Save(stream);

  stream << m_bAdditive;

  return EZ_SUCCESS;
}

ezResult ezAnimationClipResourceDescriptor::Deserialize(ezStreamReader& stream)
{
  const ezTypeVersion uiVersion = stream.ReadVersion(9);

  if (uiVersion < 6)
    return EZ_FAILURE;

  ezUInt16 uiNumJoints = 0;
  stream >> uiNumJoints;

  m_JointInfos.Reserve(uiNumJoints);

  ezHashedString hs;

  for (ezUInt16 i = 0; i < uiNumJoints; ++i)
  {
    stream >> hs;

    JointInfo ji;
    stream >> ji.m_uiPositionIdx;
    stream >> ji.m_uiPositionCount;
    stream >> ji.m_uiRotationIdx;
    stream >> ji.m_uiRotationCount;
    stream >> ji.m_uiScaleIdx;
    stream >> ji.m_uiScaleCount;

    m_JointInfos.Insert(hs, ji);
  }

  m_JointInfos.Sort();

  stream >> m_Duration;
  stream >> m_uiNumTotalPositions;
  stream >> m_uiNumTotalRotations;
  stream >> m_uiNumTotalScales;

  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_Transforms));

  if (uiVersion >= 7)
  {
    stream >> m_vConstantRootMotion;
  }

  if (uiVersion >= 8)
  {
    m_EventTrack.Load(stream);
  }

  if (uiVersion >= 9)
  {
    stream >> m_bAdditive;
  }

  return EZ_SUCCESS;
}

ezUInt64 ezAnimationClipResourceDescriptor::GetHeapMemoryUsage() const
{
  return m_Transforms.GetHeapMemoryUsage() + m_JointInfos.GetHeapMemoryUsage() + m_OzzImpl->m_MappedOzzAnimations.GetHeapMemoryUsage();
}

ezUInt16 ezAnimationClipResourceDescriptor::GetNumJoints() const
{
  return static_cast<ezUInt16>(m_JointInfos.GetCount());
}

ezTime ezAnimationClipResourceDescriptor::GetDuration() const
{
  return m_Duration;
}

void ezAnimationClipResourceDescriptor::SetDuration(ezTime duration)
{
  m_Duration = duration;
}

EZ_FORCE_INLINE void ez2ozz(const ezVec3& in, ozz::math::Float3& out)
{
  out.x = in.x;
  out.y = in.y;
  out.z = in.z;
}

EZ_FORCE_INLINE void ez2ozz(const ezQuat& in, ozz::math::Quaternion& out)
{
  out.x = in.v.x;
  out.y = in.v.y;
  out.z = in.v.z;
  out.w = in.w;
}

const ozz::animation::Animation& ezAnimationClipResourceDescriptor::GetMappedOzzAnimation(const ezSkeletonResource& skeleton) const
{
  auto it = m_OzzImpl->m_MappedOzzAnimations.Find(&skeleton);
  if (it.IsValid())
  {
    if (it.Value().m_uiResourceChangeCounter == skeleton.GetCurrentResourceChangeCounter())
    {
      return *it.Value().m_pAnim.get();
    }
  }

  auto pOzzSkeleton = &skeleton.GetDescriptor().m_Skeleton.GetOzzSkeleton();
  const ezUInt32 uiNumJoints = pOzzSkeleton->num_joints();

  ozz::animation::offline::RawAnimation rawAnim;
  rawAnim.duration = ezMath::Max(1.0f / 60.0f, m_Duration.AsFloatInSeconds());
  rawAnim.tracks.resize(uiNumJoints);

  for (ezUInt32 j = 0; j < uiNumJoints; ++j)
  {
    auto& dstTrack = rawAnim.tracks[j];

    const ezTempHashedString sJointName = ezTempHashedString(pOzzSkeleton->joint_names()[j]);

    const JointInfo* pJointInfo = GetJointInfo(sJointName);

    if (pJointInfo == nullptr)
    {
      dstTrack.translations.resize(1);
      dstTrack.rotations.resize(1);
      dstTrack.scales.resize(1);

      const ezUInt16 uiFallbackIdx = skeleton.GetDescriptor().m_Skeleton.FindJointByName(sJointName);

      EZ_ASSERT_DEV(uiFallbackIdx != ezInvalidJointIndex, "");

      const auto& fallbackJoint = skeleton.GetDescriptor().m_Skeleton.GetJointByIndex(uiFallbackIdx);

      const ezTransform& fallbackTransform = fallbackJoint.GetBindPoseLocalTransform();

      auto& dstT = dstTrack.translations[0];
      auto& dstR = dstTrack.rotations[0];
      auto& dstS = dstTrack.scales[0];

      dstT.time = 0.0f;
      dstR.time = 0.0f;
      dstS.time = 0.0f;

      ez2ozz(fallbackTransform.m_vPosition, dstT.value);
      ez2ozz(fallbackTransform.m_qRotation, dstR.value);
      ez2ozz(fallbackTransform.m_vScale, dstS.value);
    }
    else
    {
      // positions
      {
        dstTrack.translations.resize(pJointInfo->m_uiPositionCount);
        const ezArrayPtr<const KeyframeVec3> keyframes = GetPositionKeyframes(*pJointInfo);

        for (ezUInt32 i = 0; i < pJointInfo->m_uiPositionCount; ++i)
        {
          auto& dst = dstTrack.translations[i];

          dst.time = keyframes[i].m_fTimeInSec;
          ez2ozz(keyframes[i].m_Value, dst.value);
        }
      }

      // rotations
      {
        dstTrack.rotations.resize(pJointInfo->m_uiRotationCount);
        const ezArrayPtr<const KeyframeQuat> keyframes = GetRotationKeyframes(*pJointInfo);

        for (ezUInt32 i = 0; i < pJointInfo->m_uiRotationCount; ++i)
        {
          auto& dst = dstTrack.rotations[i];

          dst.time = keyframes[i].m_fTimeInSec;
          ez2ozz(keyframes[i].m_Value, dst.value);
        }
      }

      // scales
      {
        dstTrack.scales.resize(pJointInfo->m_uiScaleCount);
        const ezArrayPtr<const KeyframeVec3> keyframes = GetScaleKeyframes(*pJointInfo);

        for (ezUInt32 i = 0; i < pJointInfo->m_uiScaleCount; ++i)
        {
          auto& dst = dstTrack.scales[i];

          dst.time = keyframes[i].m_fTimeInSec;
          ez2ozz(keyframes[i].m_Value, dst.value);
        }
      }
    }
  }

  ozz::animation::offline::AnimationBuilder animBuilder;

  EZ_ASSERT_DEBUG(rawAnim.Validate(), "Invalid animation data");

  auto& cached = m_OzzImpl->m_MappedOzzAnimations[&skeleton];
  cached.m_pAnim = std::move(animBuilder(rawAnim));
  cached.m_uiResourceChangeCounter = skeleton.GetCurrentResourceChangeCounter();

  return *cached.m_pAnim.get();
}

ezAnimationClipResourceDescriptor::JointInfo ezAnimationClipResourceDescriptor::CreateJoint(const ezHashedString& sJointName, ezUInt16 uiNumPositions, ezUInt16 uiNumRotations, ezUInt16 uiNumScales)
{
  JointInfo ji;
  ji.m_uiPositionIdx = m_uiNumTotalPositions;
  ji.m_uiRotationIdx = m_uiNumTotalRotations;
  ji.m_uiScaleIdx = m_uiNumTotalScales;

  ji.m_uiPositionCount = uiNumPositions;
  ji.m_uiRotationCount = uiNumRotations;
  ji.m_uiScaleCount = uiNumScales;

  m_uiNumTotalPositions += uiNumPositions;
  m_uiNumTotalRotations += uiNumRotations;
  m_uiNumTotalScales += uiNumScales;

  m_JointInfos.Insert(sJointName, ji);

  return ji;
}

const ezAnimationClipResourceDescriptor::JointInfo* ezAnimationClipResourceDescriptor::GetJointInfo(const ezTempHashedString& sJointName) const
{
  ezUInt32 uiIndex = m_JointInfos.Find(sJointName);

  if (uiIndex == ezInvalidIndex)
    return nullptr;

  return &m_JointInfos.GetValue(uiIndex);
}

void ezAnimationClipResourceDescriptor::AllocateJointTransforms()
{
  const ezUInt32 uiNumBytes = m_uiNumTotalPositions * sizeof(KeyframeVec3) + m_uiNumTotalRotations * sizeof(KeyframeQuat) + m_uiNumTotalScales * sizeof(KeyframeVec3);

  m_Transforms.SetCountUninitialized(uiNumBytes);
}

ezArrayPtr<ezAnimationClipResourceDescriptor::KeyframeVec3> ezAnimationClipResourceDescriptor::GetPositionKeyframes(const JointInfo& jointInfo)
{
  EZ_ASSERT_DEBUG(!m_Transforms.IsEmpty(), "Joint transforms have not been allocated yet.");

  ezUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiPositionIdx;

  return ezArrayPtr<KeyframeVec3>(reinterpret_cast<KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiPositionCount);
}

ezArrayPtr<ezAnimationClipResourceDescriptor::KeyframeQuat> ezAnimationClipResourceDescriptor::GetRotationKeyframes(const JointInfo& jointInfo)
{
  EZ_ASSERT_DEBUG(!m_Transforms.IsEmpty(), "Joint transforms have not been allocated yet.");

  ezUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * jointInfo.m_uiRotationIdx;

  return ezArrayPtr<KeyframeQuat>(reinterpret_cast<KeyframeQuat*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiRotationCount);
}

ezArrayPtr<ezAnimationClipResourceDescriptor::KeyframeVec3> ezAnimationClipResourceDescriptor::GetScaleKeyframes(const JointInfo& jointInfo)
{
  EZ_ASSERT_DEBUG(!m_Transforms.IsEmpty(), "Joint transforms have not been allocated yet.");

  ezUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * m_uiNumTotalRotations;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiScaleIdx;

  return ezArrayPtr<KeyframeVec3>(reinterpret_cast<KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiScaleCount);
}

ezArrayPtr<const ezAnimationClipResourceDescriptor::KeyframeVec3> ezAnimationClipResourceDescriptor::GetPositionKeyframes(const JointInfo& jointInfo) const
{
  ezUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiPositionIdx;

  return ezArrayPtr<const KeyframeVec3>(reinterpret_cast<const KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiPositionCount);
}

ezArrayPtr<const ezAnimationClipResourceDescriptor::KeyframeQuat> ezAnimationClipResourceDescriptor::GetRotationKeyframes(const JointInfo& jointInfo) const
{
  ezUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * jointInfo.m_uiRotationIdx;

  return ezArrayPtr<const KeyframeQuat>(reinterpret_cast<const KeyframeQuat*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiRotationCount);
}

ezArrayPtr<const ezAnimationClipResourceDescriptor::KeyframeVec3> ezAnimationClipResourceDescriptor::GetScaleKeyframes(const JointInfo& jointInfo) const
{
  ezUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * m_uiNumTotalRotations;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiScaleIdx;

  return ezArrayPtr<const KeyframeVec3>(reinterpret_cast<const KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiScaleCount);
}

// bool ezAnimationClipResourceDescriptor::HasRootMotion() const
//{
//  return m_JointNameToIndex.Contains(ezTempHashedString("ezRootMotionTransform"));
//}
//
// ezUInt16 ezAnimationClipResourceDescriptor::GetRootMotionJoint() const
//{
//  ezUInt16 jointIdx = 0;
//
//#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
//
//  const ezUInt32 idx = m_JointNameToIndex.Find(ezTempHashedString("ezRootMotionTransform"));
//  EZ_ASSERT_DEBUG(idx != ezInvalidIndex, "Animation Clip has no root motion transforms");
//
//  jointIdx = m_JointNameToIndex.GetValue(idx);
//  EZ_ASSERT_DEBUG(jointIdx == 0, "The root motion joint should always be at index 0");
//#endif
//
//  return jointIdx;
//}

EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_AnimationClipResource);
