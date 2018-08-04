#pragma once

#include <Core/ResourceManager/Resource.h>
#include <RendererCore/Basics.h>

struct EZ_RENDERERCORE_DLL ezAnimationClipResourceDescriptor
{
public:
  void Configure(ezUInt16 uiNumBones, ezUInt16 uiNumFrames, ezUInt8 uiFramesPerSecond = 30);

  ezUInt16 GetNumBones() const { return m_uiNumBones; }
  ezUInt16 GetNumFrames() const { return m_uiNumFrames; }
  ezUInt8 GetFramesPerSecond() const { return m_uiFramesPerSecond; }
  ezTime GetDuration() const;

  ezUInt16 GetFrameAt(ezTime time, double& out_fLerpToNext) const;

  const ezMat4* GetFirstBones(ezUInt16 uiFrame) const;
  ezMat4* GetFirstBones(ezUInt16 uiFrame);

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);

  ezUInt64 GetHeapMemoryUsage() const;

private:
  ezUInt16 m_uiNumBones = 0;
  ezUInt16 m_uiNumFrames = 0;
  ezUInt8 m_uiFramesPerSecond = 0;

  ezTime m_Duration;

  ezDynamicArray<ezMat4> m_BoneTransforms;
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
