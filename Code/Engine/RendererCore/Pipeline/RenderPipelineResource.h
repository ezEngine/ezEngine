#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Containers/HashTable.h>
#include <RendererCore/RendererCoreDLL.h>

using ezRenderPipelineResourceHandle = ezTypedResourceHandle<class ezRenderPipelineResource>;
class ezRenderPipeline;

/// Descriptor for creating a render pipeline resource.
///
/// Contains the serialized pipeline configuration including passes, extractors, and connections.
struct ezRenderPipelineResourceDescriptor
{
  void Clear() {}

  /// Serializes a render pipeline into this descriptor.
  void CreateFromRenderPipeline(const ezRenderPipeline* pPipeline);

  ezDynamicArray<ezUInt8> m_SerializedPipeline;
  ezString m_sPath;
};

/// Runtime resource representing a render pipeline configuration.
///
/// Stores a serialized render pipeline that can be instantiated to create runtime ezRenderPipeline objects.
/// Multiple views can share the same pipeline resource but each creates its own pipeline instance.
class EZ_RENDERERCORE_DLL ezRenderPipelineResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelineResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezRenderPipelineResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezRenderPipelineResource, ezRenderPipelineResourceDescriptor);

public:
  ezRenderPipelineResource();

  EZ_ALWAYS_INLINE const ezRenderPipelineResourceDescriptor& GetDescriptor() { return m_Desc; }

  /// Creates a new runtime render pipeline instance from this resource.
  ezInternal::NewInstance<ezRenderPipeline> CreateRenderPipeline() const;

public:
  /// Returns a fallback pipeline resource used when the requested pipeline cannot be loaded.
  static ezRenderPipelineResourceHandle CreateMissingPipeline();

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezRenderPipelineResourceDescriptor m_Desc;
};
