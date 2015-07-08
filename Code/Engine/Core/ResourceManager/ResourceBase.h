#pragma once

#include <Core/Basics.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/Timestamp.h>
#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezResourceTypeLoader;

/// \brief The case class for all resources.
///
/// \note Never derive directly from ezResourceBase, but derive from ezResource instead.
class EZ_CORE_DLL ezResourceBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezResourceBase);

protected:
  enum class DoUpdate
  {
    OnMainThread,
    OnAnyThread
  };

  enum class Unload
  {
    AllQualityLevels,
    OneQualityLevel
  };

  /// \brief Default constructor.
  ezResourceBase(DoUpdate ResourceUpdateThread, ezUInt8 uiQualityLevelsLoadable);

  /// \brief virtual destructor.
  virtual ~ezResourceBase() { }

public:

  struct MemoryUsage
  {
    MemoryUsage()
    {
      m_uiMemoryCPU = 0;
      m_uiMemoryGPU = 0;
    }

    ezUInt32 m_uiMemoryCPU;
    ezUInt32 m_uiMemoryGPU;
  };

  /// \brief Returns the unique ID that identifies this resource. On a file resource this might be a path. Can also be a GUID or any other scheme that uniquely identifies the resource.
  const ezString& GetResourceID() const { return m_UniqueID; }

  /// \brief The resource description allows to store an additional string that might be more descriptive during debugging, than the unique ID.
  void SetResourceDescription(const char* szDescription);

  /// \brief The resource description allows to store an additional string that might be more descriptive during debugging, than the unique ID.
  const ezString& GetResourceDescription() const { return m_sResourceDescription; }

  /// \brief Returns the current state in which this resource is in.
  ezResourceState GetLoadingState() const { return m_LoadingState; }

  /// \brief Returns the current maximum quality level that the resource could have.
  ///
  /// This is used to scale the amount data used. Once a resource is in the 'Loaded' state, it can still have different
  /// quality levels. E.g. a texture can be fully used with n mipmap levels, but there might be more that could be loaded.
  /// On the other hand a resource could have a higher 'loaded quality level' then the 'max quality level', if the user
  /// just changed settings and reduced the maximum quality level that should be used. In this case the resource manager
  /// will instruct the resource to unload some of its data soon.
  ///
  /// The quality level is a purely logical concept that can be handled very different by different resource types.
  /// E.g. a texture resource could theoretically use one quality level per available mipmap level. However, since
  /// the resource should generally be able to load and unload each quality level separately, it might make more sense
  /// for a texture resource, to use one quality level for everything up to 64*64, and then one quality level for each
  /// mipmap above that, which would result in 5 quality levels for a 1024*1024 texture.
  ///
  /// Most resource will have zero or one quality levels (which is the same) as they are either loaded or not.
  ezUInt8 GetNumQualityLevelsDiscardable() const { return m_uiQualityLevelsDiscardable; }

  /// \brief Returns how many quality levels the resource may additionally load.
  ezUInt8 GetNumQualityLevelsLoadable() const { return m_uiQualityLevelsLoadable; }

  /// \brief Sets the current priority of this resource.
  ///
  /// This is one way for the engine to specify how important this resource is, in relation to others.
  /// The runtime can use any arbitrary scheme to compute the priority for resources, e.g. it could use
  /// distance to the camera, on screen size, or random chance.
  /// However, it should be consistent with the priority computation of other resources, to prevent
  /// preferring or penalizing other resources too much.
  ///
  /// Also make sure to always update the priority of resources when it becomes unimportant.
  /// If a resource is set to high priority and then never changed back, it will be kept loaded
  /// longer than others.
  ///
  /// The due date is an absolute deadline, whereas the priority is a relative value compared to other resources.
  /// Both can be combined. The due date always take precedence when it approaches, however as long as it is further away, priority has the most influence.
  ///
  /// \sa SetDueDate
  void SetPriority(ezResourcePriority priority);

  /// \brief Returns the currently user-specified priority of this resource. \see SetPriority
  ezResourcePriority GetPriority() const { return m_Priority; }

  /// \brief Specifies the time (usually in the future) at which this resource is needed and should be fully loaded.
  ///
  /// This is another way in which the loading priority of the resource can be influenced by the runtime.
  /// By specifying a 'due date' or 'deadline', the resource manager is instructed to make sure that this resource
  /// gets loaded in due time. The closer that the due date is, the higher the priority for loading this resource becomes.
  ///
  /// Calling this function without parameters 'resets' the due date to a date into the far future, which practically disables it.
  ///
  /// The due date is an absolute deadline, whereas the priority is a relative value compared to other resources.
  /// Both can be combined. The due date always take precedence when it approaches, however as long as it is further away, priority has the most influence.
  ///
  /// \sa SetPriority
  void SetDueDate(ezTime date = ezTime::Seconds(60.0 * 60.0 * 24.0 * 365.0 * 1000.0));

  /// \brief Returns the deadline (tNow + x) at which this resource is required to be loaded.
  ///
  /// This represents the final priority that is used by the resource manager to determine which resource to load next.
  /// \note It is fully valid to return a time in the past.
  ///
  /// \note Although it is possible to override this function, it is advised not to do so.
  /// The default algorithm is tweaked well enough already, it should not be necessary to modify it.
  virtual ezTime GetLoadingDeadline(ezTime tNow) const;

  /// \brief Returns the basic flags for the resource type. Mostly used the resource manager.
  const ezBitflags<ezResourceFlags>& GetBaseResourceFlags() const { return m_Flags; }

  /// \brief Returns the information about the current memory usage of the resource.
  const MemoryUsage& GetMemoryUsage() const { return m_MemoryUsage; }

  /// \brief Returns the time at which the resource was (tried to be) acquired last.
  /// If a resource is acquired using ezResourceAcquireMode::PointerOnly, this does not update the last acquired time, since the resource is not acquired for full use.
  ezTime GetLastAcquireTime() const { return m_LastAcquire; }

  /// \brief Returns the reference count of this resource.
  ezInt32 GetReferenceCount() const { return m_iReferenceCount; }

  /// \brief Returns the modification date of the file from which this resource was loaded.
  ///
  /// The date may be invalid, if it cannot be retrieved or the resource was created and not loaded.
  const ezTimestamp& GetLoadedFileModificationTime() const { return m_LoadedFileModificationTime; }

private:

  friend class ezResourceManager;
  friend class ezResourceManagerWorker;
  friend class ezResourceManagerWorkerGPU;

  /// \brief Called by ezResourceManager shortly after resource creation.
  void SetUniqueID(const char* szUniqueID, bool bIsReloadable);

  void CallUnloadData(Unload WhatToUnload);

  /// \brief Requests the resource to unload another quality level. If bFullUnload is true, the resource should unload all data, because it is going to be deleted afterwards.
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) = 0;

  void CallUpdateContent(ezStreamReaderBase* Stream);

  /// \brief Called whenever more data for the resource is available. The resource must read the stream to update it's data.
  virtual ezResourceLoadDesc UpdateContent(ezStreamReaderBase* Stream) = 0;

  /// \brief Returns the resource type loader that should be used for this type of resource, unless it has been overridden on the ezResourceManager.
  ///
  /// By default, this redirects to ezResourceManager::GetDefaultResourceLoader. So there is one global default loader, that can be set
  /// on the resource manager. Overriding this function will then allow to use a different resource loader on a specific type.
  /// Additionally, one can override the resource loader from the outside, by setting it via ezResourceManager::SetResourceTypeLoader.
  /// That last method always takes precedence and allows to modify the behavior without modifying the code for the resource.
  /// But in the default case, the resource defines which loader is used.
  virtual ezResourceTypeLoader* GetDefaultResourceTypeLoader() const;

  volatile ezResourceState m_LoadingState;

  ezUInt8 m_uiQualityLevelsDiscardable;
  ezUInt8 m_uiQualityLevelsLoadable;

protected:

  /// \brief Non-const version for resources that want to write this variable directly.
  MemoryUsage& ModifyMemoryUsage() { return m_MemoryUsage; }

  /// \brief Call this to specify whether a resource is reloadable.
  ///
  /// By default all created resources are flagged as not reloadable.
  /// All resources loaded from file are automatically flagged as reloadable.
  void SetIsReloadable(bool bIsReloadable) { m_Flags.AddOrRemove(ezResourceFlags::IsReloadable, bIsReloadable); }

private:
  template<typename ResourceType>
  friend class ezResourceHandle;

  template<typename SELF, typename SELF_DESCRIPTOR>
  friend class ezResource;

  friend EZ_CORE_DLL void IncreaseResourceRefCount(ezResourceBase* pResource);
  friend EZ_CORE_DLL void DecreaseResourceRefCount(ezResourceBase* pResource);

  /// \brief This function must be overridden by all resource types.
  ///
  /// It has to compute the memory used by this resource.
  /// It is called by the resource manager whenever the resource's data has been loaded or unloaded.
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) = 0;

  ezBitflags<ezResourceFlags> m_Flags;

  ezResourcePriority m_Priority;
  ezAtomicInteger32 m_iReferenceCount;
  ezAtomicInteger32 m_iLockCount;
  ezString m_UniqueID;
  ezString m_sResourceDescription;

  MemoryUsage m_MemoryUsage;

  ezTime m_LastAcquire;
  ezTime m_DueDate;
  ezTimestamp m_LoadedFileModificationTime;
};
