#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/TagSet.h>

#include <Core/World/ComponentManager.h>
#include <Core/World/GameObjectDesc.h>

struct ezDeleteObjectMessage;

/// \brief This class represents an object inside the world.
///
/// Game objects only consists of hierarchical data like transformation and a list of components.
/// You cannot derive from the game object class. To add functionality to an object you have to attach components to it.
/// To create an object instance call CreateObject on the world. Never store a direct pointer to an object but store an
/// object handle instead.
/// \see ezWorld
/// \see ezComponent
///
/// \todo Implement Clone
/// \todo Implement switching dynamic and static

class EZ_CORE_DLL ezGameObject
{
private:
  enum
  {
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
    NUM_INPLACE_COMPONENTS = 12
#else
    NUM_INPLACE_COMPONENTS = 6
#endif
  };

  friend class ezWorld;
  friend class ezInternal::WorldData;
  friend class ezMemoryUtils;

  ezGameObject();
  ezGameObject(const ezGameObject& other);
  ~ezGameObject();

  void operator=(const ezGameObject& other);

public:
  /// \brief Iterates over all children of one object.
  class EZ_CORE_DLL ConstChildIterator
  {
  public:
    const ezGameObject& operator*() const;
    const ezGameObject* operator->() const;

    operator const ezGameObject*() const;

    /// \brief Advances the iterator to the next child object. The iterator will not be valid anymore, if the last child is reached.
    void Next();

    /// \brief Checks whether this iterator points to a valid object.
    bool IsValid() const;

    /// \brief Returns the current object
    const ezGameObject& Current() const { return *m_pObject; }

    /// \brief Shorthand for 'Next'
    void operator++();

  private:
    friend class ezGameObject;

    ConstChildIterator(ezGameObject* pObject);

    ezGameObject* m_pObject;
  };

  class EZ_CORE_DLL ChildIterator : public ConstChildIterator
  {
  public:
    ezGameObject& operator*();
    ezGameObject* operator->();

    /// \brief Returns the current object
    ezGameObject& Current() { return *m_pObject; }

    operator ezGameObject*();

  private:
    friend class ezGameObject;

    ChildIterator(ezGameObject* pObject);
  };

  /// \brief Returns a handle to this object.
  ezGameObjectHandle GetHandle() const;

  //ezGameObjectHandle Clone() const;

  //void MakeDynamic();
  //void MakeStatic();
  /// \brief Returns whether this object is dynamic.
  bool IsDynamic() const;

  /// \brief Returns whether this object is static.
  bool IsStatic() const;


  /// \brief Activates the object and all its components.
  void Activate();

  /// \brief Deactivates the object and all its components.
  void Deactivate();

  /// \brief Returns whether this object is active.
  bool IsActive() const;

  /// \brief Sets the name to identify this object. Does not have to be a unique name.
  void SetName(const char* szName);
  void SetName(const ezHashedString& sName);
  const char* GetName() const;

  /// \brief Sets the global key to identify this object. Global keys must be unique within a world.
  void SetGlobalKey(const char* szGlobalKey);
  void SetGlobalKey(const ezHashedString& sGlobalKey);
  const char* GetGlobalKey() const;

  /// \brief Defines during re-parenting what transform is going to be preserved.
  enum class TransformPreservation
  {
    PreserveLocal,
    PreserveGlobal
  };

  /// \brief Sets the parent of this object to the given.
  void SetParent(const ezGameObjectHandle& parent, ezGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Gets the parent of this object or nullptr if this is a top-level object.
  ezGameObject* GetParent();

  /// \brief Gets the parent of this object or nullptr if this is a top-level object.
  const ezGameObject* GetParent() const;

  /// \brief Adds the given object as a child object.
  void AddChild(const ezGameObjectHandle& child, ezGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Adds the given objects as child objects.
  void AddChildren(const ezArrayPtr<const ezGameObjectHandle>& children, ezGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Detaches the given child object from this object and makes it a top-level object.
  void DetachChild(const ezGameObjectHandle& child, ezGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Detaches the given child objects from this object and makes them top-level objects.
  void DetachChildren(const ezArrayPtr<const ezGameObjectHandle>& children, ezGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Returns the number of children.
  ezUInt32 GetChildCount() const;

  /// \brief Returns an iterator over all children of this object.
  ChildIterator GetChildren();

  /// \brief Returns an iterator over all children of this object.
  ConstChildIterator GetChildren() const;


  ezWorld* GetWorld();
  const ezWorld* GetWorld() const;


  /// \brief Changes the position of the object local to its parent.
  /// \note The rotation of the object itself does not affect the final global position!
  /// The local position is always in the space of the parent object. If there is no parent, local position and global position are identical.
  void SetLocalPosition(const ezVec3& position);
  const ezVec3& GetLocalPosition() const;

  void SetLocalRotation(const ezQuat& rotation);
  const ezQuat& GetLocalRotation() const;

  void SetLocalScaling(const ezVec3& scaling);
  const ezVec3& GetLocalScaling() const;

  void SetLocalUniformScaling(float scaling);
  float GetLocalUniformScaling() const;

  void SetGlobalPosition(const ezVec3& position);
  const ezVec3& GetGlobalPosition() const;

  void SetGlobalRotation(const ezQuat rotation);
  const ezQuat GetGlobalRotation() const;

  void SetGlobalScaling(const ezVec3 scaling);
  const ezVec3 GetGlobalScaling() const;

  void SetGlobalTransform(const ezTransform& transform);
  const ezTransform& GetGlobalTransform() const;

  ezVec3 GetDirForwards() const;
  ezVec3 GetDirRight() const;
  ezVec3 GetDirUp() const;

  void SetVelocity(const ezVec3& vVelocity);

  /// \brief Returns the velocity of the object in units per second. This is not only the diff between last frame's position and this frame's position, but
  ///        also the time difference is divided out.
  const ezVec3& GetVelocity() const;

  /// \brief Updates the global transform immediately. Usually this done during the world update after the "Post-async" phase.
  void UpdateGlobalTransform();


  const ezBoundingBoxSphere& GetLocalBounds() const;
  const ezBoundingBoxSphere& GetGlobalBounds() const;

  /// \brief Invalidates the local bounds and sends a message to all components so they can add their bounds.
  void UpdateLocalBounds();

  /// \brief Updates the global transform and bounds immediately. Usually this done during the world update after the "Post-async" phase.
  void UpdateGlobalTransformAndBounds();


  /// \brief Attaches the component to the object. Calls the OnAttachedToObject method on the component.
  ezResult AttachComponent(const ezComponentHandle& component);

  /// \brief Attaches the component to the object. Calls the OnAttachedToObject method on the component.
  ezResult AttachComponent(ezComponent* pComponent);

  /// \brief Detaches the component from this object. Calls the OnDetachedFromObject method on the component. The component is still valid afterwards.
  ezResult DetachComponent(const ezComponentHandle& component);

  /// \brief Detaches the component from this object. Calls the OnDetachedFromObject method on the component. The component is still valid afterwards.
  ezResult DetachComponent(ezComponent* pComponent);

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  template <typename T>
  bool TryGetComponentOfBaseType(T*& out_pComponent) const;

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  bool TryGetComponentOfBaseType(const ezRTTI* pType, ezComponent*& out_pComponent) const;

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  template <typename T>
  void TryGetComponentsOfBaseType(ezHybridArray<T*, 8>& out_components) const;

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  void TryGetComponentsOfBaseType(const ezRTTI* pType, ezHybridArray<ezComponent*, 8>& out_components) const;

  /// \brief Returns a list of all components attached to this object.
  ezArrayPtr<ezComponent* const> GetComponents();

  /// \brief Returns a list of all components attached to this object.
  ezArrayPtr<const ezComponent* const> GetComponents() const;


  /// \brief Sends a message to all components of this object. Depending on the routing options the message is also send to parents or children.
  void SendMessage(ezMessage& msg, ezObjectMsgRouting::Enum routing = ezObjectMsgRouting::Default);

  /// \brief Sends a message to all components of this object. Depending on the routing options the message is also send to parents or children.
  void SendMessage(ezMessage& msg, ezObjectMsgRouting::Enum routing = ezObjectMsgRouting::Default) const;

  /// \brief Queues the message for the given phase and processes it later in that phase.
  void PostMessage(ezMessage& msg, ezObjectMsgQueueType::Enum queueType,
    ezObjectMsgRouting::Enum routing = ezObjectMsgRouting::Default) const;

  /// \brief Queues the message for the given phase. The message is processed after the given delay in the corresponding phase.
  void PostMessage(ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay,
    ezObjectMsgRouting::Enum routing = ezObjectMsgRouting::Default) const;


  /// \brief Returns the tag set associated with this object.
  ezTagSet& GetTags();
  const ezTagSet& GetTags() const;

private:
  friend class ezGameObjectTest;

  EZ_ALLOW_PRIVATE_PROPERTIES(ezGameObject);

  // Add / Detach child used by the reflected property keep their local transform as
  // updating that is handled by the editor.
  void Reflection_AddChild(ezGameObject* pChild);
  void Reflection_DetachChild(ezGameObject* pChild);
  ezHybridArray<ezGameObject*, 8> Reflection_GetChildren() const;
  void Reflection_AddComponent(ezComponent* pComponent) { AttachComponent(pComponent); }
  void Reflection_RemoveComponent(ezComponent* pComponent) { DetachComponent(pComponent); }
  const ezHybridArray<ezComponent*, NUM_INPLACE_COMPONENTS>& Reflection_GetComponents() const { return m_Components; }

  void OnDeleteObject(ezDeleteObjectMessage& msg);

  void FixComponentPointer(ezComponent* pOldPtr, ezComponent* pNewPtr);

  struct EZ_CORE_DLL EZ_ALIGN_16(TransformationData)
  {
    EZ_DECLARE_POD_TYPE();

    ezGameObject* m_pObject;
    TransformationData* m_pParentData;

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
    ezUInt64 m_uiPadding;
#endif

    ezVec4 m_localPosition;
    ezQuat m_localRotation;
    ezVec4 m_localScaling; // x,y,z = non-uniform scaling, w = uniform scaling

    ezTransform m_globalTransform;
    ezVec4 m_lastGlobalPosition;
    ezVec4 m_velocity; // w = 1 indicates custom velocity

    ezBoundingBoxSphere m_localBounds;
    ezBoundingBoxSphere m_globalBounds;

    ezSpatialDataHandle m_hSpatialData;

    ezUInt32 m_uiPadding2[13];

    void ConditionalUpdateGlobalTransform();
    void UpdateGlobalTransform();
    void UpdateGlobalTransformWithParent();

    void ConditionalUpdateGlobalBounds();
    void UpdateGlobalBounds();

    void UpdateSpatialData();
  };

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
  ezUInt16 m_uiReserved2;

  ezUInt16 m_uiHierarchyLevel;
  TransformationData* m_pTransformationData;

  ezWorld* m_pWorld;

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezUInt64 m_uiPadding;
#endif

  /// \todo small array class to reduce memory overhead
  ezHybridArray<ezComponent*, NUM_INPLACE_COMPONENTS> m_Components;

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezUInt64 m_uiPadding2;
#endif

  /// \todo somehow make this more compact
  ezTagSet m_Tags;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezGameObject);

#include <Core/World/Implementation/GameObject_inl.h>

