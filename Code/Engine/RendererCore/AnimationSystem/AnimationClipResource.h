#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Tracks/EventTrack.h>
#include <ozz/base/memory/unique_ptr.h>

class ezSkeletonResource;
class ezSkeleton;

namespace ozz::animation
{
  class Animation;
}

struct EZ_RENDERERCORE_DLL ezAnimationClipResourceDescriptor
{
public:
  ezAnimationClipResourceDescriptor();
  ezAnimationClipResourceDescriptor(ezAnimationClipResourceDescriptor&& rhs);
  ~ezAnimationClipResourceDescriptor();

  void operator=(ezAnimationClipResourceDescriptor&& rhs) noexcept;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

  ezUInt64 GetHeapMemoryUsage() const;

  ezUInt16 GetNumJoints() const;
  ezTime GetDuration() const;
  void SetDuration(ezTime duration);

  void CreateMappedOzzAnimation(ozz::unique_ptr<ozz::animation::Animation>& out_pOzzAnim, const ezSkeleton& skeleton) const;
  const ozz::animation::Animation& GetMappedOzzAnimation(const ezSkeletonResource& skeleton) const;

  struct JointInfo
  {
    ezUInt32 m_uiPositionIdx = 0;
    ezUInt32 m_uiRotationIdx = 0;
    ezUInt32 m_uiScaleIdx = 0;
    ezUInt16 m_uiPositionCount = 0;
    ezUInt16 m_uiRotationCount = 0;
    ezUInt16 m_uiScaleCount = 0;
  };

  struct KeyframeVec3
  {
    float m_fTimeInSec;
    ezVec3 m_Value;
  };

  struct KeyframeQuat
  {
    float m_fTimeInSec;
    ezQuat m_Value;
  };

  JointInfo CreateJoint(const ezHashedString& sJointName, ezUInt16 uiNumPositions, ezUInt16 uiNumRotations, ezUInt16 uiNumScales);
  const JointInfo* GetJointInfo(const ezTempHashedString& sJointName) const;
  void AllocateJointTransforms();

  ezArrayPtr<KeyframeVec3> GetPositionKeyframes(const JointInfo& jointInfo);
  ezArrayPtr<KeyframeQuat> GetRotationKeyframes(const JointInfo& jointInfo);
  ezArrayPtr<KeyframeVec3> GetScaleKeyframes(const JointInfo& jointInfo);

  ezArrayPtr<const KeyframeVec3> GetPositionKeyframes(const JointInfo& jointInfo) const;
  ezArrayPtr<const KeyframeQuat> GetRotationKeyframes(const JointInfo& jointInfo) const;
  ezArrayPtr<const KeyframeVec3> GetScaleKeyframes(const JointInfo& jointInfo) const;

  ezVec3 m_vConstantRootMotion = ezVec3::MakeZero();

  ezEventTrack m_EventTrack;

  bool m_bAdditive = false;

private:
  ezArrayMap<ezHashedString, JointInfo> m_JointInfos;
  ezDataBuffer m_Transforms;
  ezUInt32 m_uiNumTotalPositions = 0;
  ezUInt32 m_uiNumTotalRotations = 0;
  ezUInt32 m_uiNumTotalScales = 0;
  ezTime m_Duration;

  struct OzzImpl;
  ezUniquePtr<OzzImpl> m_pOzzImpl;
};

//////////////////////////////////////////////////////////////////////////

using ezAnimationClipResourceHandle = ezTypedResourceHandle<class ezAnimationClipResource>;

class EZ_RENDERERCORE_DLL ezAnimationClipResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezAnimationClipResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezAnimationClipResource, ezAnimationClipResourceDescriptor);

public:
  ezAnimationClipResource();

  const ezAnimationClipResourceDescriptor& GetDescriptor() const { return *m_pDescriptor; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezUniquePtr<ezAnimationClipResourceDescriptor> m_pDescriptor;
};
