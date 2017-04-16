#pragma once

#include <GameEngine/Basics.h>
#include <Core/ResourceManager/Resource.h>

struct EZ_GAMEENGINE_DLL ezCollectionEntry
{
  ezString m_sLookupName;
  ezString m_sRedirectionName;
  ezHashedString m_sResourceTypeName;
};

struct EZ_GAMEENGINE_DLL ezCollectionResourceDescriptor
{
  ezDynamicArray<ezCollectionEntry> m_Resources;

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);
};

typedef ezTypedResourceHandle<class ezCollectionResource> ezCollectionResourceHandle;

class EZ_GAMEENGINE_DLL ezCollectionResource : public ezResource<ezCollectionResource, ezCollectionResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCollectionResource, ezResourceBase);

public:
  ezCollectionResource();

  /// \brief Registers the named resources in the collection with the ezResourceManager, such that they can be loaded by those names.
  ///
  /// \note This has to be called MANUALLY on collection resources, they do NOT do this automatically when loaded.
  /// Since resources are streamed, there is no guaranteed point in time when those names would be registered, which would introduce timing issues,
  /// where sometimes the name is registered when it is used and sometimes not.
  /// By calling this manually, the point in time where this is done is fixed and guaranteed.
  ///
  /// Calling this twice has no effect.
  void RegisterNames();

  /// \brief Removes the registered names from the ezResourceManager.
  ///
  /// Calling this twice has no effect.
  void UnregisterNames();

  /// \brief Puts every resource for which a resource type could be found into the preload queue of the ezResourceManager
  void PreloadResources(ezTime tShouldBeAvailableIn);


private:
  virtual ezResourceLoadDesc CreateResource(const ezCollectionResourceDescriptor& descriptor) override;
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  bool m_bRegistered;
  ezCollectionResourceDescriptor m_Collection;
  ezDynamicArray<ezTypelessResourceHandle> m_hPreloadedResources;
};


