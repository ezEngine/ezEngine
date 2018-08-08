#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Basics.h>

struct EZ_RENDERERCORE_DLL ezAnimationClipResourceDescriptor
{
public:
  void Configure(ezUInt16 uiNumJoints, ezUInt16 uiNumFrames, ezUInt8 uiFramesPerSecond);

  ezUInt16 GetNumJoints() const { return m_uiNumJoints; }
  ezUInt16 GetNumFrames() const { return m_uiNumFrames; }
  ezUInt8 GetFramesPerSecond() const { return m_uiFramesPerSecond; }
  ezTime GetDuration() const;

  ezUInt16 GetFrameAt(ezTime time, double& out_fLerpToNext) const;

  ezUInt16 AddJointName(const ezHashedString& sJointName);

  /// \brief returns 0xFFFF if no joint with the given name is known
  ezUInt16 FindJointIndexByName(const ezTempHashedString& sJointName) const;

  const ezTransform* GetJointKeyframes(ezUInt16 uiJoint) const;
  ezTransform* GetJointKeyframes(ezUInt16 uiJoint);

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);

  ezUInt64 GetHeapMemoryUsage() const;

  const ezArrayMap<ezHashedString, ezUInt32>& GetAllJointIndices() const { return m_NameToFirstKeyframe; }

private:
  ezUInt16 m_uiNumJoints = 0;
  ezUInt16 m_uiNumFrames = 0;
  ezUInt8 m_uiFramesPerSecond = 0;

  ezTime m_Duration;

  ezDynamicArray<ezTransform> m_JointTransforms;
  ezArrayMap<ezHashedString, ezUInt32> m_NameToFirstKeyframe;
};

typedef ezTypedResourceHandle<class ezAnimationClipResource> ezAnimationClipResourceHandle;

class EZ_RENDERERCORE_DLL ezAnimationClipResource : public ezResource<ezAnimationClipResource, ezAnimationClipResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipResource, ezResourceBase);

public:
  ezAnimationClipResource();

  const ezAnimationClipResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual ezResourceLoadDesc CreateResource(const ezAnimationClipResourceDescriptor& descriptor) override;
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezAnimationClipResourceDescriptor m_Descriptor;
};
