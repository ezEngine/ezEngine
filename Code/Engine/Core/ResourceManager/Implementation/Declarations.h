#pragma once

#include <Core/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Bitflags.h>

class ezResourceManager;

// resource base classes
class ezResourceBase;

template<typename SELF, typename SELF_DESCRIPTOR>
class ezResource;

// resource handle type
template<typename ResourceType>
class ezResourceHandle;

/// \brief The flags of an ezResourceBase instance.
struct EZ_CORE_DLL ezResourceFlags
{
  typedef ezUInt16 StorageType;

  /// \brief The flags of an ezResourceBase instance.
  enum Enum
  {
    UpdateOnMainThread    = EZ_BIT(0),  ///< After loading the resource data on a thread, it must be uploaded on the main thread. Use this for GPU resources etc. which require a context that is only available on the main thread.
    NoFileAccessRequired  = EZ_BIT(1),  ///< The resource 'loading' does not require file accesses and can therefore be done on one or several non-file-loading threads. Use this for procedurally generated data.
    /// \todo implement NoFileAccessRequired
    ResourceHasFallback   = EZ_BIT(2),  ///< Specifies whether this resource has a valid fallback resource that could be used. Automatically updated in ezResource::SetFallbackResource.
    IsReloadable          = EZ_BIT(3),  ///< The resource was created, not loaded from file
    IsPreloading          = EZ_BIT(4),
    Default               = 0,
  };

  struct Bits
  {
    StorageType UpdateOnMainThread    : 1;
    StorageType NoFileAccessRequired  : 1;
    StorageType ResourceHasFallback   : 1;
    StorageType IsReloadable          : 1;
    StorageType IsPreloading          : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezResourceFlags);

/// \brief Describes the state in which a resource can be in.
enum class ezResourceState
{
  Invalid,
  Unloaded,                 ///< The resource instance has been created, but no meta info about the resource is available and no data is loaded.
  LoadedResourceMissing,    ///< The resource could not be loaded, use a 'Missing Resource' fallback if available
  UnloadedMetaInfoAvailable,///< Meta information about the resource is available (e.g. texture sizes, etc.) but no data is loaded so far.
  Loaded,                   ///< The resource is fully loaded.
  // no other state can follow, Loaded must be the highest value
};

/// \brief Describes in which loading state a resource currently is, and how many different quality levels there are
struct ezResourceLoadDesc
{
  ezResourceLoadDesc()
  {
    m_State = ezResourceState::Invalid;
    m_uiQualityLevelsDiscardable = 0xFF; // invalid
    m_uiQualityLevelsLoadable = 0xFF; // invalid
  }

  ezResourceState m_State;
  ezUInt8 m_uiQualityLevelsDiscardable;
  ezUInt8 m_uiQualityLevelsLoadable;
};

/// \brief The priority of a resource. Determines how soon it will be loaded and how late it will be unloaded.
enum class ezResourcePriority
{
  Highest,
  High,
  Normal,
  Low,
  Lowest,
  Unchanged   ///< When a function might adjust the priority of a resource, this means it should keep it unchanged.
};

/// \brief Describes what data of a resource needs to be accessed and thus how much of the resource needs to be loaded.
enum class ezResourceAcquireMode
{
  PointerOnly,    ///< We really only want the pointer (maybe to update some state), no data needs to be loaded. This will never block.
  AllowFallback,  ///< We want to use the resource, but if it has a fallback, using that is fine as well. This should be the default usage.
  MetaInfo,       ///< The actual resource data is not needed, but its meta data is required (e.g. texture dimensions). Thus a fallback resource cannot be used. This will block until at least the meta info is available.
  NoFallback,     ///< The full resource data is required. The loader will block until the resource is in the 'Loaded' state. This does NOT mean that all quality levels are loaded.
};

