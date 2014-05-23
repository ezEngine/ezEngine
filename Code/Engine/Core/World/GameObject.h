#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Time/Time.h>

#include <Core/World/ComponentManager.h>
#include <Core/World/GameObjectDesc.h>

class EZ_CORE_DLL ezGameObject
{
private:
  friend class ezWorld;
  friend struct ezInternal::WorldData;
  friend class ezMemoryUtils;

  ezGameObject();
  ezGameObject(const ezGameObject& other);
  ~ezGameObject();
  
  void operator=(const ezGameObject& other);

  struct EZ_ALIGN_16(TransformationData)
  {
    EZ_DECLARE_POD_TYPE();

    ezGameObject* m_pObject;
    TransformationData* m_pParentData;
    
  #if EZ_ENABLED(EZ_PLATFORM_32BIT)
    ezUInt64 m_uiPadding;
  #endif

    ezVec4 m_localPosition;
    ezQuat m_localRotation;
    ezVec4 m_localScaling;

    ezTransform m_worldTransform;
    ezVec4 m_velocity;
  };

public:
  class EZ_CORE_DLL ChildIterator
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
  
  /// \todo
  //ezGameObjectHandle Clone() const;

  /// \todo
  //void MakeDynamic();
  //void MakeStatic();
  bool IsDynamic() const;
  bool IsStatic() const;

  void Activate();
  void Deactivate();
  bool IsActive() const;

  /// \todo
  //ezUInt64 GetUniqueId() const;

  void SetName(const char* szName);
  const char* GetName() const;

  void SetParent(const ezGameObjectHandle& parent);
  ezGameObject* GetParent() const;

  void AddChild(const ezGameObjectHandle& child);
  void AddChildren(const ezArrayPtr<const ezGameObjectHandle>& children);

  void DetachChild(const ezGameObjectHandle& child);
  void DetachChildren(const ezArrayPtr<const ezGameObjectHandle>& children);

  ezUInt32 GetChildCount() const;
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
  const ezQuat GetWorldRotation() const;

  void SetWorldScaling(const ezVec3& scaling);
  const ezVec3 GetWorldScaling() const;

  void SetWorldTransform(const ezTransform& transform);
  const ezTransform& GetWorldTransform() const;

  void SetVelocity(const ezVec3& vVelocity);
  const ezVec3& GetVelocity() const;  

  // components
  ezResult AddComponent(const ezComponentHandle& component);
  ezResult RemoveComponent(const ezComponentHandle& component);

  template <typename T>
  bool TryGetComponentOfBaseType(T*& out_pComponent) const;

  template <typename T>
  void TryGetComponentsOfBaseType(ezHybridArray<T*, 8>& out_components) const;

  ezArrayPtr<ezComponentHandle> GetComponents() const;

  // messaging
  void SendMessage(ezMessage& msg, ezBitflags<ezObjectMsgRouting> routing = ezObjectMsgRouting::Default);
  void PostMessage(ezMessage& msg, ezObjectMsgQueueType::Enum queueType, 
    ezBitflags<ezObjectMsgRouting> routing = ezObjectMsgRouting::Default);
  void PostMessage(ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay,
    ezBitflags<ezObjectMsgRouting> routing = ezObjectMsgRouting::Default);
  
private:
  bool TryGetComponent(const ezComponentHandle& component, ezComponent*& out_pComponent) const;
  void OnMessage(ezMessage& msg, ezBitflags<ezObjectMsgRouting> routing);

  ezGameObjectId m_InternalId;
  ezBitflags<ezObjectFlags> m_Flags;
  ezHashedString m_sName;

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezUInt32 m_uiNamePadding;
#endif

  struct
  {
    ezUInt64 m_ParentIndex : 20;
    ezUInt64 m_FirstChildIndex : 20;
    ezUInt64 m_LastChildIndex : 20;
  };

  struct
  {
    ezUInt64 m_NextSiblingIndex : 20;
    ezUInt64 m_PrevSiblingIndex : 20;
    ezUInt64 m_ChildCount : 20;
  };

  ezUInt32 m_uiReserved;

  struct
  {
    ezUInt32 m_uiHierarchyLevel : 12;
    ezUInt32 m_uiTransformationDataIndex : 20;
  };
  TransformationData* m_pTransformationData;

  ezWorld* m_pWorld;

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezUInt64 m_uiPadding;
#endif

  /// \todo small array class to reduce memory overhead
  /// \todo save pointer to component instead?
  ezHybridArray<ezComponentHandle, 6> m_Components;

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezUInt64 m_uiPadding2;
#endif
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezGameObject);

#include <Core/World/Implementation/GameObject_inl.h>

