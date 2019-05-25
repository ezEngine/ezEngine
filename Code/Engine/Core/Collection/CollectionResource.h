#pragma once

#include <Core/CoreDLL.h>
#include <Core/ResourceManager/Resource.h>

/// \brief Represents one resource to load / preload through an ezCollectionResource
struct EZ_CORE_DLL ezCollectionEntry
{
  ezString m_sOptionalNiceLookupName; ///< Optional, can be used to lookup the resource at runtime with a nice name. E.g. "SkyTexture" instead of some GUID.
  ezString m_sResourceID; ///< The ID / path to the resource to load.
  ezHashedString m_sAssetTypeName;
};

/// \brief Describes a full ezCollectionResource, ie. lists all the resources that the collection contains
struct EZ_CORE_DLL ezCollectionResourceDescriptor
{
  ezDynamicArray<ezCollectionEntry> m_Resources;

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);
};

typedef ezTypedResourceHandle<class ezCollectionResource> ezCollectionResourceHandle;

/// \brief An ezCollectionResource is used to tell the engine about resources that it should preload in the background
///
/// Collection resources can be used to improve the user experience by ensuring data is already (more likely) available when it is needed.
/// For instance when a player walks into a longer corridor, a collection resource can be triggered to preload the data that will be needed
/// when he reaches the door at the end of the corridor.
/// In scenes this can be achieved using an ezCollectionComponent.
///
/// Collection resources can also be used to query how much percent of the references resources are already loaded, which enables building
/// progress bars or allowing functionality only after all necessary data is available.
///
/// Finally, collections can also be used to register resources with a nice name, such that it can be referenced in code by that name.
/// This is useful if you want to reference some asset in code with a readable resource ID, and may want to change which resource is referenced
/// at a later time. For example the nice name "Avatar" could point to some dummy mesh at the beginning of the project, and be updated to point
/// to something else later, without the need to change the code.
///
/// \note Once ezCollectionResource::PreloadResources() has been triggered, the resource will hold a handle to all referenced resources,
///       meaning those resources cannot get unloaded anymore. Make sure to discard the collection resource if the data it references
///       should get freed up.
class EZ_CORE_DLL ezCollectionResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCollectionResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezCollectionResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezCollectionResource, ezCollectionResourceDescriptor);

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
  ///
  /// This has to be called manually (or triggered by an ezCollectionComponent).
  void PreloadResources();

  /// \brief Returns a value between 0.0 and 1.0 representing how many of the preloaded resources are in a loaded state at the moment.
  ///
  /// This will always return 1.0 if the collection contains no resources, or PreloadResources() was not triggered previously.
  float GetPreloadProgress() const;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  mutable ezMutex m_preloadMutex;
  bool m_bRegistered = false;
  ezCollectionResourceDescriptor m_Collection;
  ezDynamicArray<ezTypelessResourceHandle> m_hPreloadedResources;
};


