#pragma once

#include <Core/ResourceManager/Resource.h>
#include <RendererCore/Components/BlackboardComponent.h>

using ezBlackboardTemplateResourceHandle = ezTypedResourceHandle<class ezBlackboardTemplateResource>;

struct EZ_RENDERERCORE_DLL ezBlackboardTemplateResourceDescriptor
{
  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

  ezDynamicArray<ezBlackboardEntry> m_Entries;
};

/// \brief Describes the initial state of a blackboard.
///
/// Used by ezBlackboardComponent to initialize its blackboard from.
class EZ_RENDERERCORE_DLL ezBlackboardTemplateResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBlackboardTemplateResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezBlackboardTemplateResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezBlackboardTemplateResource, ezBlackboardTemplateResourceDescriptor);

public:
  ezBlackboardTemplateResource();
  ~ezBlackboardTemplateResource();

  const ezBlackboardTemplateResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezBlackboardTemplateResourceDescriptor m_Descriptor;
};
