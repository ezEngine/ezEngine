#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Basics.h>

struct EZ_RENDERERCORE_DLL ezAnimationClipResourceDescriptor
{
public:
  void Configure(ezUInt16 uiNumBones, ezUInt16 uiNumFrames, ezUInt8 uiFramesPerSecond);

  ezUInt16 GetNumBones() const { return m_uiNumBones; }
  ezUInt16 GetNumFrames() const { return m_uiNumFrames; }
  ezUInt8 GetFramesPerSecond() const { return m_uiFramesPerSecond; }
  ezTime GetDuration() const;

  ezUInt16 GetFrameAt(ezTime time, double& out_fLerpToNext) const;

  ezUInt16 AddBoneName(const ezHashedString& sBoneName);

  /// \brief returns 0xFFFF if no bone with the given name is known
  ezUInt16 FindBoneIndexByName(const ezTempHashedString& sBoneName) const;

  const ezMat4* GetBoneKeyframes(ezUInt16 uiBone) const;
  ezMat4* GetBoneKeyframes(ezUInt16 uiBone);

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);

  ezUInt64 GetHeapMemoryUsage() const;

  const ezArrayMap<ezHashedString, ezUInt32>& GetAllBoneIndices() const { return m_NameToFirstKeyframe; }

private:
  ezUInt16 m_uiNumBones = 0;
  ezUInt16 m_uiNumFrames = 0;
  ezUInt8 m_uiFramesPerSecond = 0;

  ezTime m_Duration;

  ezDynamicArray<ezMat4> m_BoneTransforms;
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
