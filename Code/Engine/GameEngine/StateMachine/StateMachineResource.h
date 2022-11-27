#pragma once

#include <Core/ResourceManager/Resource.h>
#include <GameEngine/StateMachine/StateMachine.h>

using ezStateMachineResourceHandle = ezTypedResourceHandle<class ezStateMachineResource>;

class EZ_GAMEENGINE_DLL ezStateMachineResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezStateMachineResource);

public:
  ezStateMachineResource();
  ~ezStateMachineResource();

  const ezSharedPtr<const ezStateMachineDescription>& GetDescription() const { return m_pDescription; }

  ezUniquePtr<ezStateMachineInstance> CreateInstance(ezReflectedClass& ref_owner);

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezSharedPtr<const ezStateMachineDescription> m_pDescription;
};
