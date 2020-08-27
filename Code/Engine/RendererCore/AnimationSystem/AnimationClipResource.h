#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>

class ezSkeletonResource;

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

  void Configure(ezUInt16 uiNumJoints, ezUInt16 uiNumFrames, ezTime duration /*, bool bIncludeRootMotion*/);

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);

  ezUInt64 GetHeapMemoryUsage() const;

  ezUInt16 GetNumJoints() const;
  ezUInt16 GetNumFrames() const;
  ezTime GetDuration() const;

  ezUInt16 AddJointName(const ezHashedString& sJointName);

  /// \brief returns ezInvalidJointIndex if no joint with the given name is known
  ezUInt16 FindJointIndexByName(const ezTempHashedString& sJointName) const;

  const ezArrayMap<ezHashedString, ezUInt16>& GetAllJointIndices() const;

  //ezUInt16 GetFrameAt(ezTime time, double& out_fLerpToNext) const;
  ezArrayPtr<const ezTransform> GetJointKeyframes(ezUInt16 uiJoint) const;
  ezArrayPtr<ezTransform> GetJointKeyframes(ezUInt16 uiJoint);
  //bool HasRootMotion() const;
  //ezUInt16 GetRootMotionJoint() const;
  //void SetPoseToKeyframe(ezAnimationPose& pose, const ezSkeleton& skeleton, ezUInt16 uiKeyframe) const;
  //void SetPoseToBlendedKeyframe(ezAnimationPose& pose, const ezSkeleton& skeleton, ezUInt16 uiKeyframe0, float fBlendToKeyframe1) const;

  const ozz::animation::Animation& GetMappedOzzAnimation(const ezSkeletonResource& skeleton) const;

private:
  struct Impl;
  ezUniquePtr<Impl> m_Impl;
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

  const ezAnimationClipResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezAnimationClipResourceDescriptor m_Descriptor;
};
