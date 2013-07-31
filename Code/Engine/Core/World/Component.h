#pragma once

#include <Foundation/Communication/Message.h>

#include <Core/World/Declarations.h>

class EZ_CORE_DLL ezComponent
{
public:
  ezComponent() : m_Flags(ezObjectFlags::Default) { }
  virtual ~ezComponent() { }

  virtual ezUInt16 GetTypeId() const = 0;

  EZ_FORCE_INLINE bool IsDynamic() { return m_Flags.IsSet(ezObjectFlags::Dynamic); }

  void Activate();
  void Deactivate();
  EZ_FORCE_INLINE bool IsActive() { return m_Flags.IsSet(ezObjectFlags::Active); }

  virtual ezResult Initialize() { return EZ_SUCCESS; }
  virtual ezResult Deinitialize() { return EZ_SUCCESS; }

  virtual void OnMessage(ezMessage& msg) { }

protected:
  friend class ezGameObject;
  friend class ezComponentManagerBase;

  ezComponentManagerBase* m_pManager;
  ezGameObject* m_pOwner;

  ezGenericComponentId m_InternalId;
  ezBitflags<ezObjectFlags> m_Flags;
};

#define EZ_DECLARE_COMPONENT_TYPE(componentType, managerType) \
  public: \
    ezComponentHandle GetHandle() const; \
    managerType* GetManager() const; \
    EZ_FORCE_INLINE ezWorld* GetWorld() const { return m_pManager->GetWorld(); } \
    virtual ezUInt16 GetTypeId() const EZ_OVERRIDE { return TYPE_ID; } \
    static EZ_FORCE_INLINE ezUInt16 TypeId() { return TYPE_ID; } \
  private: \
    friend managerType; \
    static ezUInt16 TYPE_ID;

#define EZ_IMPLEMENT_COMPONENT_TYPE(componentType, managerType) \
  ezUInt16 componentType::TYPE_ID = ezComponentManagerBase::GetNextTypeId(); \
  ezComponentHandle componentType::GetHandle() const { return managerType::GetHandle(m_InternalId); } \
  managerType* componentType::GetManager() const { return static_cast<managerType*>(m_pManager); }
