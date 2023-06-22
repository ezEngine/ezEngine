#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <Foundation/Types/UniquePtr.h>

class ezAnimGraph;
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

  void DeserializeAnimGraphState(ezAnimGraph& ref_out);

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezResult AllocNodes();

  ezInstanceDataAllocator m_InstanceDataAllocator;
  ezDataBuffer m_Storage;
  ezDynamicArray<ezUniquePtr<ezAnimGraphNode>> m_Nodes;
};
