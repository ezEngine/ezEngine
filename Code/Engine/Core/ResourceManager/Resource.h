#pragma once

#include <Core/Basics.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Time/Time.h>
#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezResourceTypeLoader;

/// \brief The case class for all resources.
///
/// \note Never derive directly from ezResourceBase, but derive from ezResource instead.
class EZ_CORE_DLL ezResourceBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezResourceBase);

public:
  /// \brief Default constructor.
  ezResourceBase();

  /// \brief virtual destructor.
  virtual ~ezResourceBase() { }

  /// \brief Returns the unique ID that identifies this resource. On a file resource this might be a path. Can also be a GUID or any other scheme that uniquely identifies the resource.
  const ezString& GetResourceID() const { return m_UniqueID; }

  /// \brief Returns the current state in which this resource is in.
  ezResourceLoadState::Enum GetLoadingState() const { return m_LoadingState; }

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
  ezUInt8 GetMaxQualityLevel() const { return m_uiMaxQualityLevel; }

  /// \brief Returns the current quality level that is loaded for this resource. \see GetMaxQualityLevel
  ezUInt8 GetLoadedQualityLevel() const { return m_uiLoadedQualityLevel; }

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
  void SetPriority(ezResourcePriority::Enum priority) { m_Priority = priority; }

  /// \brief Returns the currently user-specified priority of this resource. \see SetPriority
  ezResourcePriority::Enum GetPriority() const { return m_Priority; }

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
  void SetDueDate(ezTime date = ezTime::Seconds(60.0 * 60.0 * 24.0 * 365.0 * 1000.0)) { m_DueDate = date; }

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

  /// \brief Returns the CPU RAM used by the resource. This needs to be updated in UpdateMemoryUsage().
  ezUInt32 GetMemoryUsageCPU() const { return m_uiMemoryCPU; }

  /// \brief Returns the GPU memory used by the resource. This needs to be updated in UpdateMemoryUsage().
  ezUInt32 GetMemoryUsageGPU() const { return m_uiMemoryGPU; }

  /// \brief Returns the time at which the resource was (tried to be) acquired last.
  /// If a resource is acquired using ezResourceAcquireMode::PointerOnly, this does not update the last acquired time, since the resource is not acquired for full use.
  ezTime GetLastAcquireTime() const { return m_LastAcquire; }

  /// \brief Returns the reference count of this resource.
  ezInt32 GetReferenceCount() const { return m_iReferenceCount; }

private:

  friend class ezResourceManager;
  friend class ezResourceManagerWorker;
  friend class ezResourceManagerWorkerGPU;

  /// \brief Called by ezResourceManager shortly after resource creation.
  void SetUniqueID(const ezString& UniqueID);

  /// \brief Requests the resource to unload another quality level. If bFullUnload is true, the resource should unload all data, because it is going to be deleted afterwards.
  virtual void UnloadData(bool bFullUnload) = 0;

  /// \brief Called whenever more data for the resource is available. The resource must read the stream to update it's data.
  ///
  /// Afterwards it must update the following data:
  ///  m_uiLoadedQualityLevel and m_uiMaxQualityLevel (both are allowed to change)
  ///  m_LoadingState (should be MetaInfoAvailable or Loaded afterwards)
  virtual void UpdateContent(ezStreamReaderBase& Stream) = 0;

  /// \brief Returns the resource type loader that should be used for this type of resource, unless it has been overridden on the ezResourceManager.
  ///
  /// By default, this redirects to ezResourceManager::GetDefaultResourceLoader. So there is one global default loader, that can be set
  /// on the resource manager. Overriding this function will then allow to use a different resource loader on a specific type.
  /// Additionally, one can override the resource loader from the outside, by setting it via ezResourceManager::SetResourceTypeLoader.
  /// That last method always takes precedence and allows to modify the behavior without modifying the code for the resource.
  /// But in the default case, the resource defines which loader is used.
  virtual ezResourceTypeLoader* GetDefaultResourceTypeLoader() const;

protected:

  // Derived classes MUST set these values properly when they update their data.

  /// \brief Call this inside of UpdateMemoryUsage() to update the current memory usage.
  void SetMemoryUsageCPU(ezUInt32 uiMemory) { m_uiMemoryCPU = uiMemory; }

  /// \brief Call this inside of UpdateMemoryUsage() to update the current memory usage.
  void SetMemoryUsageGPU(ezUInt32 uiMemory) { m_uiMemoryGPU = uiMemory; }

  ezBitflags<ezResourceFlags> m_Flags;

  volatile ezResourceLoadState::Enum m_LoadingState;

  ezUInt8 m_uiMaxQualityLevel;
  ezUInt8 m_uiLoadedQualityLevel;

private:
  template<typename ResourceType>
  friend class ezResourceHandle;

  /// \brief This function must be overridden by all resource types.
  ///
  /// It has to compute the CPU and GPU memory used by this resource, and set that via SetMemoryUsageCPU() and SetMemoryUsageGPU().
  /// It is called by the resource manager whenever the resource's data has been loaded or unloaded.
  virtual void UpdateMemoryUsage() = 0;

  ezResourcePriority::Enum m_Priority;
  ezAtomicInteger32 m_iReferenceCount;
  ezAtomicInteger32 m_iLockCount;
  ezString m_UniqueID;

  ezUInt32 m_uiMemoryCPU;
  ezUInt32 m_uiMemoryGPU;

  bool m_bIsPreloading;
  
  ezTime m_LastAcquire;
  ezTime m_DueDate;
};


/// \brief The class from which all resource types need to derive.
///
/// Pass the resources own type as the first template parameter.
/// Pass the type name of the resource descriptor struct as the second template parameter.
/// This may be any custom struct that stores the required information for creating a resource.
template<typename SELF, typename SELF_DESCRIPTOR>
class ezResource : public ezResourceBase
{
public:
  typedef SELF_DESCRIPTOR DescriptorType;

  /// \brief Sets the fallback resource that can be used while this resource is not yet loaded.
  ///
  /// By default there is no fallback resource, so all resource will block the application when requested for the first time.
  void SetFallbackResource(const ezResourceHandle<SELF>& hResource)
  {
    m_hFallback = hResource;
    m_Flags.AddOrRemove(ezResourceFlags::ResourceHasFallback, m_hFallback.IsValid());
  }
  

private:
  friend class ezResourceManager;

  /// \brief Override this function to implement resource creation. This is called by ezResourceManager::CreateResource.
  ///
  /// 'Creating' a resource is different from 'loading' a resource. Most resource types are never created. For example a mesh
  /// or a sound might not support being created and thus your implementation for such resources should be empty, except for an assert
  /// that always fires.
  /// However, some resources can be created (or even can only be created, but not loaded). For example textures might support to be
  /// created as render targets. In this case the descriptor needs to provide all the necessary information.
  /// Another example would be a procedurally generated mesh. The descriptor needs to provide all the parameters, such that 'CreateResource'
  /// can set up the resource.
  /// Note that created resources should always set its loading state to 'Loaded' and its current and max quality to 1, otherwise
  /// the resource manager might try to load even more into the resource afterwards.
  /// However, since this might be a valid use case for some resource types, it is not enforced by the resource manager.
  virtual void CreateResource(const SELF_DESCRIPTOR& descriptor) = 0;

  ezResourceHandle<SELF> m_hFallback;
};



