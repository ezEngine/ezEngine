#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/RendererCoreDLL.h>

class ezAnimationPose;
class ezSkeleton;

struct EZ_RENDERERCORE_DLL ezAnimationClipResourceDescriptor
{
public:
  void Configure(ezUInt16 uiNumJoints, ezUInt16 uiNumFrames, ezUInt8 uiFramesPerSecond, bool bIncludeRootMotion);

  ezUInt16 GetNumJoints() const { return m_uiNumJoints; }
  ezUInt16 GetNumFrames() const { return m_uiNumFrames; }
  ezUInt8 GetFramesPerSecond() const { return m_uiFramesPerSecond; }
  ezTime GetDuration() const;

  ezUInt16 GetFrameAt(ezTime time, double& out_fLerpToNext) const;

  ezUInt16 AddJointName(const ezHashedString& sJointName);

  /// \brief returns ezInvalidJointIndex if no joint with the given name is known
  ezUInt16 FindJointIndexByName(const ezTempHashedString& sJointName) const;

  ezArrayPtr<const ezTransform> GetJointKeyframes(ezUInt16 uiJoint) const;
  ezArrayPtr<ezTransform> GetJointKeyframes(ezUInt16 uiJoint);

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);

  ezUInt64 GetHeapMemoryUsage() const;

  const ezArrayMap<ezHashedString, ezUInt16>& GetAllJointIndices() const { return m_JointNameToIndex; }

  bool HasRootMotion() const;

  ezUInt16 GetRootMotionJoint() const;

  void SetPoseToKeyframe(ezAnimationPose& pose, const ezSkeleton& skeleton, ezUInt16 uiKeyframe) const;
  void SetPoseToBlendedKeyframe(ezAnimationPose& pose, const ezSkeleton& skeleton, ezUInt16 uiKeyframe0, float fBlendToKeyframe1) const;

private:
  ezUInt16 m_uiNumJoints = 0;
  ezUInt16 m_uiNumFrames = 0;
  ezUInt8 m_uiFramesPerSecond = 0;

  ezTime m_Duration;

  ezDynamicArray<ezTransform> m_JointTransforms;
  ezArrayMap<ezHashedString, ezUInt16> m_JointNameToIndex;
};

typedef ezTypedResourceHandle<class ezAnimationClipResource> ezAnimationClipResourceHandle;

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

