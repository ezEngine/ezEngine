#pragma once

#include <Core/ResourceManager/Resource.h>
#include <ProcGenPlugin/Declarations.h>

typedef ezTypedResourceHandle<class ezProcGenGraphResource> ezProcGenGraphResourceHandle;

struct EZ_PROCGENPLUGIN_DLL ezProcGenGraphResourceDescriptor
{
  // empty, these types of resources must be loaded from file
};

class EZ_PROCGENPLUGIN_DLL ezProcGenGraphResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcGenGraphResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezProcGenGraphResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezProcGenGraphResource, ezProcGenGraphResourceDescriptor);

public:
  ezProcGenGraphResource();
  ~ezProcGenGraphResource();

  const ezDynamicArray<ezSharedPtr<const ezProcGenInternal::PlacementOutput>>& GetPlacementOutputs() const;
  const ezDynamicArray<ezSharedPtr<const ezProcGenInternal::VertexColorOutput>>& GetVertexColorOutputs() const;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezDynamicArray<ezExpressionByteCode> m_ByteCode;
  ezDynamicArray<ezSharedPtr<const ezProcGenInternal::PlacementOutput>> m_PlacementOutputs;
  ezDynamicArray<ezSharedPtr<const ezProcGenInternal::VertexColorOutput>> m_VertexColorOutputs;
};
