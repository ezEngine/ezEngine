#pragma once

#include <RendererCore/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Containers/HashTable.h>

typedef ezTypedResourceHandle<class ezRenderPipelineResource> ezRenderPipelineResourceHandle;
class ezRenderPipeline;

struct ezRenderPipelineResourceDescriptor
{
  void Clear()
  {
  }

  void CreateFromRenderPipeline(const ezRenderPipeline* pPipeline);

  ezDynamicArray<ezUInt8> m_SerializedPipeline;
  ezString m_sPath;
};

class EZ_RENDERERCORE_DLL ezRenderPipelineResource : public ezResource<ezRenderPipelineResource, ezRenderPipelineResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelineResource, ezResourceBase);

public:
  ezRenderPipelineResource();

  EZ_FORCE_INLINE const ezRenderPipelineResourceDescriptor& GetDescriptor()
  {
    return m_Desc;
  }

  ezInternal::NewInstance<ezRenderPipeline> CreateRenderPipeline() const;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezRenderPipelineResourceDescriptor& descriptor) override;

private:
  ezRenderPipelineResourceDescriptor m_Desc;
};


