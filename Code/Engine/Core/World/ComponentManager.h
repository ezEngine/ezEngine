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
  typedef ezDelegate<void (ezUInt32, ezUInt32)> UpdateFunction;

  struct UpdateFunctionDesc
  {
    enum Phase
    {
      PreAsync,
      Async,
      PostAsync,
      PHASE_COUNT
    };

    UpdateFunctionDesc()
    {
      m_Phase = PreAsync;
      m_uiGranularity = 0;
    }

    UpdateFunction m_Function;
    ezHybridArray<UpdateFunction, 4> m_DependsOn;
    Phase m_Phase;
    ezUInt32 m_uiGranularity;
  };

  virtual ~ezComponentManagerBase();

  virtual ezResult Initialize() { return EZ_SUCCESS; }
  virtual ezResult Deinitialize() { return EZ_SUCCESS; }

  ezWorld* GetWorld() const;

  ezComponent* GetComponent(const ezComponentHandle& component) const;
  ezUInt32 GetComponentCount() const;

  void DeleteComponent(const ezComponentHandle& component);

  static ezUInt16 GetNextTypeId();

protected:
  ezComponentHandle CreateComponent(ezComponent* pComponent, ezUInt16 uiTypeId);  

  ezIAllocator* GetAllocator();
  ezLargeBlockAllocator* GetBlockAllocator();

  void RegisterUpdateFunction(const UpdateFunctionDesc& desc);
  void DeregisterUpdateFunction(const UpdateFunctionDesc& desc);

  static ezComponentId GetIdFromHandle(const ezComponentHandle& component);
  static ezComponentHandle GetHandle(ezGenericComponentId internalId, ezUInt16 uiTypeId);

  ezIdTable<ezGenericComponentId, ezComponent*> m_Components;
    
private:
  ezWorld* m_pWorld;

  static ezUInt16 s_uiNextTypeId;
};

template <typename ComponentType>
class ezComponentManager : public ezComponentManagerBase
{
public:
  typedef ComponentType ComponentType;

  ezComponentManager(ezWorld* pWorld);
  virtual ~ezComponentManager();

  ezComponentHandle CreateComponent();

  ComponentType* GetComponent(const ezComponentHandle& component) const;
  typename ezBlockStorage<ComponentType>::Iterator GetComponents();

protected:
  friend ComponentType;

  void RegisterUpdateFunction(UpdateFunctionDesc& desc);

  static ezComponentHandle GetHandle(ezGenericComponentId internalId);

  ezBlockStorage<ComponentType> m_ComponentStorage;
};

template <typename ComponentType>
class ezComponentManagerSimple : public ezComponentManager<ComponentType>
{
public:
  ezComponentManagerSimple(ezWorld* pWorld);

  virtual ezResult Initialize() EZ_OVERRIDE;

  // simple update function that iterates over all components and calls update
  void SimpleUpdate(ezUInt32 uiStartIndex, ezUInt32 uiCount);
};

#include <Core/World/Implementation/ComponentManager_inl.h>
