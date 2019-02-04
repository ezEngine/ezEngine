#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Containers/HashTable.h>
#include <RendererCore/Basics.h>

typedef ezTypedResourceHandle<class ezRenderPipelineResource> ezRenderPipelineResourceHandle;
class ezRenderPipeline;

struct ezRenderPipelineResourceDescriptor
{
  void Clear() {}

  void CreateFromRenderPipeline(const ezRenderPipeline* pPipeline);

  ezDynamicArray<ezUInt8> m_SerializedPipeline;
  ezString m_sPath;
};

class EZ_RENDERERCORE_DLL ezRenderPipelineResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelineResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezRenderPipelineResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezRenderPipelineResource, ezRenderPipelineResourceDescriptor);

public:
  ezRenderPipelineResource();

  EZ_ALWAYS_INLINE const ezRenderPipelineResourceDescriptor& GetDescriptor() { return m_Desc; }

  ezInternal::NewInstance<ezRenderPipeline> CreateRenderPipeline() const;

public:
  static ezRenderPipelineResourceHandle CreateMissingPipeline();

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezRenderPipelineResourceDescriptor m_Desc;
};

