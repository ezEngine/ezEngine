#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

class ezAnimGraphInstance;
class ezAnimGraphNode;

//////////////////////////////////////////////////////////////////////////

using ezAnimGraphResourceHandle = ezTypedResourceHandle<class ezAnimGraphResource>;

struct EZ_RENDERERCORE_DLL ezAnimationClipMapping : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipMapping, ezReflectedClass);

  ezHashedString m_sClipName;
  ezAnimationClipResourceHandle m_hClip;

  const char* GetClipName() const { return m_sClipName.GetData(); }
  void SetClipName(const char* szName) { m_sClipName.Assign(szName); }
};

class EZ_RENDERERCORE_DLL ezAnimGraphResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezAnimGraphResource);

public:
  ezAnimGraphResource();
  ~ezAnimGraphResource();

  const ezAnimGraph& GetAnimationGraph() const { return m_AnimGraph; }

  ezArrayPtr<const ezString> GetIncludeGraphs() const { return m_IncludeGraphs; }
  const ezDynamicArray<ezAnimationClipMapping>& GetAnimationClipMapping() const { return m_AnimationClipMapping; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezDynamicArray<ezString> m_IncludeGraphs;
  ezDynamicArray<ezAnimationClipMapping> m_AnimationClipMapping;
  ezAnimGraph m_AnimGraph;
};
