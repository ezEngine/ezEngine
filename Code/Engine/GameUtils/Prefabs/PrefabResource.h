#pragma once

#include <GameUtils/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/WorldSerializer/WorldReader.h>

typedef ezTypedResourceHandle<class ezPrefabResource> ezPrefabResourceHandle;

struct EZ_GAMEUTILS_DLL ezPrefabResourceDescriptor
{

};

class EZ_GAMEUTILS_DLL ezPrefabResource : public ezResource<ezPrefabResource, ezPrefabResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPrefabResource, ezResourceBase);

public:
  ezPrefabResource();

  /// \brief Creates an instance of this prefab in the given world.
  void InstantiatePrefab(ezWorld& world, const ezTransform& rootTransform, ezGameObjectHandle hParent = ezGameObjectHandle(), ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects = nullptr);

private:

  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  virtual ezResourceLoadDesc CreateResource(const ezPrefabResourceDescriptor& descriptor);
private:
  ezWorldReader m_WorldReader;
};


