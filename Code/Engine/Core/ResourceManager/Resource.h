#pragma once

#include <Core/Basics.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Core/ResourceManager/ResourceHandle.h>

template<typename ResourceType>
class ezResourceHandle;

typedef ezString ezResourceID;

class ezResourceBase;

struct EZ_CORE_DLL ezResourceFlags
{
  typedef ezUInt16 StorageType;

  enum Enum
  {
    UpdateOnMainThread = EZ_BIT(0),
    Default = 0,
  };

  struct Bits
  {
    StorageType UpdateOnMainThread : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezResourceFlags);


struct EZ_CORE_DLL ezResourceLoadState
{
  enum Enum
  {
    Uninitialized,
    Unloaded,
    Loaded,
    // missing resource
  };
};

// Resource Flags:
// Category / Group (Texture Sets)
//  Max Loaded Quality (adjustable at runtime)

// Resource Loader
//   Requires File Access -> on File Thread
//   

class ezResourceBase;

struct EZ_CORE_DLL ezResourcePriority
{
  enum Enum
  {
    Highest,
    High,
    Normal,
    Low,
    Lowest,
    Unchanged
  };
};

class EZ_CORE_DLL ezResourceBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezResourceBase);

public:
  ezResourceBase()
  {
    m_iReferenceCount = 0;
    m_LoadingState = ezResourceLoadState::Uninitialized;
    m_uiMaxQualityLevel = 0;
    m_uiLoadedQualityLevel = 0;
    m_Priority = ezResourcePriority::Normal;
    m_bIsPreloading = false;
    SetDueDate();
    m_uiMemoryCPU = 0;
    m_uiMemoryGPU = 0;
    m_bHasFallback = false;
  }

  virtual ~ezResourceBase() { }

  const ezResourceID& GetResourceID() const { return m_UniqueID; }

  ezResourceLoadState::Enum GetLoadingState() const { return m_LoadingState; }

  ezUInt8 GetMaxQualityLevel() const { return m_uiMaxQualityLevel; }
  ezUInt8 GetLoadedQualityLevel() const { return m_uiLoadedQualityLevel; }

  void SetPriority(ezResourcePriority::Enum priority) { m_Priority = priority; }
  ezResourcePriority::Enum GetPriority() const { return m_Priority; }

  

  void SetDueDate(ezTime date = ezTime::Seconds(60.0 * 60.0 * 24.0 * 365.0 * 1000.0)) { m_DueDate = date; }

  ezTime GetLoadingDeadline(ezTime tNow) const;

  const ezBitflags<ezResourceFlags>& GetResourceFlags() const { return m_Flags; }

  ezUInt32 GetMemoryUsageCPU() const { return m_uiMemoryCPU; }
  ezUInt32 GetMemoryUsageGPU() const { return m_uiMemoryGPU; }

private:

  friend class ezResourceManager;
  friend class ezResourceManagerWorker;
  friend class ezResourceManagerWorkerGPU;

  void SetUniqueID(const ezResourceID& UniqueID)
  {
    m_UniqueID = UniqueID;
  }

  virtual void UnloadData(bool bFullUnload) = 0;

  

  virtual void UpdateContent(ezStreamReaderBase& Stream) = 0;

protected:

  void SetMemoryUsageCPU(ezUInt32 uiMemory) { m_uiMemoryCPU = uiMemory; }
  void SetMemoryUsageGPU(ezUInt32 uiMemory) { m_uiMemoryGPU = uiMemory; }

  ezBitflags<ezResourceFlags> m_Flags;

  ezResourceLoadState::Enum m_LoadingState;

  ezUInt8 m_uiMaxQualityLevel;    // for textures the number of mipmaps, for meshes the number of LODs
  ezUInt8 m_uiLoadedQualityLevel;
  bool m_bHasFallback;

private:
  template<typename ResourceType>
  friend class ezResourceHandle;

  virtual void UpdateMemoryUsage() = 0;

  ezResourcePriority::Enum m_Priority;
  ezAtomicInteger32 m_iReferenceCount;
  ezAtomicInteger32 m_iLockCount;
  ezResourceID m_UniqueID;

  ezUInt32 m_uiMemoryCPU;
  ezUInt32 m_uiMemoryGPU;

  bool m_bIsPreloading;
  
  ezTime m_LastAcquire;
  ezTime m_DueDate;

  
};


template<typename SELF, typename SELF_DESCRIPTOR>
class ezResource : public ezResourceBase
{
public:
  typedef SELF_DESCRIPTOR DescriptorType;

  void SetFallbackResource(const ezResourceHandle<SELF>& hResource) { m_hFallback = hResource; m_bHasFallback = m_hFallback.IsValid(); }

  

private:
  friend class ezResourceManager;

  virtual void CreateResource(const SELF_DESCRIPTOR& descriptor) = 0;

  ezResourceHandle<SELF> m_hFallback;
};


