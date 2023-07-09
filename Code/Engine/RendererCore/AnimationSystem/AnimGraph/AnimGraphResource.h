#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>

class ezAnimGraphInstance;
class ezAnimGraphNode;

//////////////////////////////////////////////////////////////////////////

using ezAnimGraphResourceHandle = ezTypedResourceHandle<class ezAnimGraphResource>;

class EZ_RENDERERCORE_DLL ezAnimGraphResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezAnimGraphResource);

public:
  ezAnimGraphResource();
  ~ezAnimGraphResource();

  const ezAnimGraph& GetAnimationGraph() const { return m_AnimGraph; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezAnimGraph m_AnimGraph;
};
