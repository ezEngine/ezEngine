#pragma once

#include <Foundation/Basics/Types/Delegate.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Memory/BlockStorage.h>

#include <Core/World/Declarations.h>
#include <Core/World/Component.h>

class EZ_CORE_DLL ezComponentManagerBase
{
protected:
  ezComponentManagerBase(ezWorld* pWorld);
  
public:
  virtual ~ezComponentManagerBase();

  virtual ezResult Initialize() { return EZ_SUCCESS; }
  virtual ezResult Deinitialize() { return EZ_SUCCESS; }

  ezWorld* GetWorld() const;

  bool IsValidComponent(const ezComponentHandle& component) const;

  bool TryGetComponent(const ezComponentHandle& component, ezComponent*& out_pComponent) const;
  ezUInt32 GetComponentCount() const;

  virtual ezComponentHandle CreateComponent() = 0;
  void DeleteComponent(const ezComponentHandle& component);

  static ezUInt16 GetNextTypeId();

protected:
  friend class ezWorld;
  friend struct ezInternal::WorldData;

  typedef ezDelegate<void (ezUInt32, ezUInt32)> UpdateFunction;

  struct UpdateFunctionDesc
  {
    enum Phase
    {
      PreAsync,
      Async,
      PostAsync,
      PostTransform,
      PHASE_COUNT
    };

    UpdateFunctionDesc(const UpdateFunction& function, const char* szFunctionName)
    {
      m_Function = function;
      m_szFunctionName = szFunctionName;
      m_Phase = PreAsync;
      m_uiGranularity = 0;
    }

    UpdateFunction m_Function;
    const char* m_szFunctionName;
    ezHybridArray<UpdateFunction, 4> m_DependsOn;
    Phase m_Phase;
    ezUInt32 m_uiGranularity;
  };

  typedef ezBlockStorage<ezComponent>::Entry ComponentStorageEntry;

  ezComponentHandle CreateComponent(ComponentStorageEntry storageEntry, ezUInt16 uiTypeId);
  void DeleteComponent(ComponentStorageEntry storageEntry);
  virtual void DeleteDeadComponent(ComponentStorageEntry storageEntry);

  ezAllocatorBase* GetAllocator();
  ezLargeBlockAllocator* GetBlockAllocator();

  void RegisterUpdateFunction(const UpdateFunctionDesc& desc);
  void DeregisterUpdateFunction(const UpdateFunctionDesc& desc);

  static ezComponentId GetIdFromHandle(const ezComponentHandle& component);
  static ezComponentHandle GetHandle(ezGenericComponentId internalId, ezUInt16 uiTypeId);

  ezIdTable<ezGenericComponentId, ComponentStorageEntry> m_Components;

private:
  ezWorld* m_pWorld;

  static ezUInt16 s_uiNextTypeId;
};

template <typename T>
class ezComponentManager : public ezComponentManagerBase
{
public:
  typedef T ComponentType;

  ezComponentManager(ezWorld* pWorld);
  virtual ~ezComponentManager();

  virtual ezComponentHandle CreateComponent() override;
  ezComponentHandle CreateComponent(ComponentType*& out_pComponent);

  bool TryGetComponent(const ezComponentHandle& component, ComponentType*& out_pComponent) const;
  typename ezBlockStorage<ComponentType>::Iterator GetComponents();

  static ezUInt16 TypeId();

protected:
  friend ComponentType;

  virtual void DeleteDeadComponent(ComponentStorageEntry storageEntry) override;

  void RegisterUpdateFunction(UpdateFunctionDesc& desc);

  ezBlockStorage<ComponentType> m_ComponentStorage;
};

template <typename ComponentType>
class ezComponentManagerNoUpdate : public ezComponentManager<ComponentType>
{
public:
  ezComponentManagerNoUpdate(ezWorld* pWorld);
};

template <typename ComponentType>
class ezComponentManagerSimple : public ezComponentManager<ComponentType>
{
public:
  ezComponentManagerSimple(ezWorld* pWorld);

  virtual ezResult Initialize() override;

  // simple update function that iterates over all components and calls update
  void SimpleUpdate(ezUInt32 uiStartIndex, ezUInt32 uiCount);
};

/// \brief Helper macro to create an update function description with proper name
#define EZ_CREATE_COMPONENT_UPDATE_FUNCTION_DESC(func, instance) \
  ezComponentManagerBase::UpdateFunctionDesc(ezComponentManagerBase::UpdateFunction(&func, instance), #func)

#include <Core/World/Implementation/ComponentManager_inl.h>

