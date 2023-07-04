#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>

struct EZ_GAMEENGINE_DLL ezBlackboardTemplateResourceDescriptor
{
  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);

  ezDynamicArray<ezBlackboardEntry> m_Entries;
};

class EZ_GAMEENGINE_DLL ezBlackboardTemplateResource : public ezResource
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

using ezBlackboardTemplateResourceHandle = ezTypedResourceHandle<class ezBlackboardTemplateResource>;
