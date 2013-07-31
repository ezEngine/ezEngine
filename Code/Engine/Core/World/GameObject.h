#pragma once

#include <Foundation/Containers/HybridArray.h>

#include <Core/World/ComponentManager.h>
#include <Core/World/GameObjectDesc.h>

class EZ_CORE_DLL ezGameObject
{
private:
  friend class ezWorld;
  friend struct ezInternal::WorldData;
  friend class ezMemoryUtils;

  ezGameObject();
  ~ezGameObject();
  
  EZ_DISALLOW_COPY_AND_ASSIGN(ezGameObject);

  struct EZ_ALIGN_16(HierarchicalData)
  {
    EZ_DECLARE_POD_TYPE();

    ezGameObject* m_pObject;
    HierarchicalData* m_pParentData;
    
  #if EZ_ENABLED(EZ_PLATFORM_32BIT)
    ezUInt64 m_uiPadding;
  #endif

    ezVec4 m_localPosition;
    ezQuat m_localRotation;
    ezVec4 m_localScaling;

    // todo: simd struct
    ezMat3 m_worldRotation;
    ezVec3 m_worldPosition;

    ezVec4 m_velocity;
  };

public:
  class ChildIterator
  {
  public:
    ChildIterator();

    ezGameObject& operator*() const;
    ezGameObject* operator->() const;

    void Next();
    bool IsValid() const;

    void operator++();

  private:
    friend class ezGameObject;

    ChildIterator(ezGameObject* pObject);

    ezGameObject* m_pObject;
  };

  ezGameObjectHandle GetHandle() const;
  
  ezGameObjectHandle Clone() const;

  void MakeDynamic();
  bool IsDynamic();

  void Activate();
  void Deactivate();
  bool IsActive();

  ezUInt64 GetPersistentId() const;

  void SetName(const char* szName);
  const char* GetName() const;

  void SetParent(const ezGameObjectHandle& parent);
  ezGameObjectHandle GetParent() const;

  void AddChild(const ezGameObjectHandle& child);
  void AddChildren(const ezArrayPtr<const ezGameObjectHandle>& children);

  void DetachChild(const ezGameObjectHandle& child);
  void DetachChildren(const ezArrayPtr<const ezGameObjectHandle>& children);

  ChildIterator GetChildren() const;

  ezWorld* GetWorld() const;

  void SetLocalPosition(const ezVec3& position);
  const ezVec3& GetLocalPosition() const;

  void SetLocalRotation(const ezQuat& rotation);
  const ezQuat& GetLocalRotation() const;

  void SetLocalScaling(const ezVec3& scaling);
  const ezVec3& GetLocalScaling() const;

  void SetWorldPosition(const ezVec3& position);
  const ezVec3& GetWorldPosition() const;

  void SetWorldRotation(const ezQuat& rotation);
  const ezQuat& GetWorldRotation() const;

  void SetWorldScaling(const ezVec3& scaling);
  const ezVec3& GetWorldScaling() const;

  ezMat4 GetWorldTransform() const;

  const ezVec3& GetVelocity() const;

  // components
  ezResult AddComponent(const ezComponentHandle& component);
  ezResult RemoveComponent(const ezComponentHandle& component);

  template <typename T>
  T* GetComponentOfType() const;

  template <typename T>
  ezResult GetComponentsOfType(ezArrayPtr<T*> out_components) const;

  ezArrayPtr<ezComponentHandle> GetComponents() const;
  ezResult GetComponents(ezArrayPtr<ezComponent*> out_components) const;

  // messaging
  struct MsgRouting
  {
    enum Enum
    {
      ToParent   = EZ_BIT(0),
      ToChildren = EZ_BIT(1),
      Broadcast  = ToParent | ToChildren
    };
  };

  void SendMessage(ezMessage& msg, MsgRouting::Enum routing = MsgRouting::ToChildren);
  
private:
  void OnMessage(ezMessage& msg, MsgRouting::Enum routing);

  ezGameObjectId m_InternalId;
  ezBitflags<ezObjectFlags> m_Flags;
  ezUInt64 m_uiPersistentId;
  
  ezGameObjectHandle m_Parent;
  ezGameObjectHandle m_FirstChild;
  ezGameObjectHandle m_NextSibling;

  struct
  {
    ezUInt32 m_uiHierarchicalDataIndex : 20;
    ezUInt32 m_uiHierarchyLevel : 12;
  };
  HierarchicalData* m_pHierarchicalData;

  ezWorld* m_pWorld;

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezUInt64 m_uiPadding;
#endif

  // todo: small array class to reduce memory overhead
  ezHybridArray<ezComponentHandle, 7> m_Components;

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezUInt64 m_uiPadding2;
#endif
};

#include <Core/World/Implementation/GameObject_inl.h>
