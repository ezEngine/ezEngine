#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/TagSet.h>

#include <Core/World/ComponentManager.h>
#include <Core/World/GameObjectDesc.h>

/// \brief This class represents an object inside the world.
///
/// Game objects only consists of hierarchical data like transformation and a list of components.
/// You cannot derive from the game object class. To add functionality to an object you have to attach components to it.
/// To create an object instance call CreateObject on the world. Never store a direct pointer to an object but store an
/// object handle instead.
/// \see ezWorld
/// \see ezComponent

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

  /// \brief Makes this object and all its children dynamic. Dynamic objects might move during runtime.
  void MakeDynamic();

  /// \brief Makes this object static. Static objects don't move during runtime.
  void MakeStatic();

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
  bool HasName(const ezTempHashedString& name) const;

  /// \brief Sets the global key to identify this object. Global keys must be unique within a world.
  void SetGlobalKey(const char* szGlobalKey);
  void SetGlobalKey(const ezHashedString& sGlobalKey);
  const char* GetGlobalKey() const;

  /// \brief Enables or disabled notification message when children are added or removed. The notification message is sent to this object and all its parent objects.
  void EnableChildChangesNotifications();
  void DisableChildChangesNotifications();

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
  void AddChildren(const ezArrayPtr<const ezGameObjectHandle>& children,
                   ezGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Detaches the given child object from this object and makes it a top-level object.
  void DetachChild(const ezGameObjectHandle& child, ezGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Detaches the given child objects from this object and makes them top-level objects.
  void DetachChildren(const ezArrayPtr<const ezGameObjectHandle>& children,
                      ezGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Returns the number of children.
  ezUInt32 GetChildCount() const;

  /// \brief Returns an iterator over all children of this object.
  ChildIterator GetChildren();

  /// \brief Returns an iterator over all children of this object.
  ConstChildIterator GetChildren() const;

  /// \brief Searches for a child object with the given name. Optionally traverses the entire hierarchy.
  ezGameObject* FindChildByName(const ezTempHashedString& name, bool bRecursive = true);

  /// \brief Searches for a child using a path. Every path segment represents a child with a given name.
  ///
  /// Paths are separated with single slashes: /
  /// When an empty path is given, 'this' is returned.
  /// When on any part of the path the next child cannot be found, nullptr is returned.
  /// This function expects an exact path to the destination. It does not search the full hierarchy for
  /// the next child, as SearchChildByNameSequence() does.
  ezGameObject* FindChildByPath(const char* path);

  /// \brief Searches for a child similar to FindChildByName() but allows to search for multiple names in a sequence.
  ///
  /// The names in the sequence are separated with slashes.
  /// For example, calling this with "a/b" will first search the entire hierarchy below this object for a child
  /// named "a". If that is found, the search continues from there for a child called "b".
  /// If such a child is found and pExpectedComponent != nullptr, it is verified that the object
  /// contains a component of that type. If it doesn't the search continues (including back-tracking).
  ezGameObject* SearchForChildByNameSequence(const char* szObjectSequence, const ezRTTI* pExpectedComponent = nullptr);

  /// \brief Same as SearchForChildByNameSequence but returns ALL matches, in case the given path could mean multiple objects
  void SearchForChildrenByNameSequence(const char* szObjectSequence, const ezRTTI* pExpectedComponent,
                                       ezHybridArray<ezGameObject*, 8>& out_Objects);

  ezWorld* GetWorld();
  const ezWorld* GetWorld() const;


  /// \brief Changes the position of the object local to its parent.
  /// \note The rotation of the object itself does not affect the final global position!
  /// The local position is always in the space of the parent object. If there is no parent, local position and global position are
  /// identical.
  void SetLocalPosition(ezVec3 position);
  ezVec3 GetLocalPosition() const;

  void SetLocalRotation(ezQuat rotation);
  ezQuat GetLocalRotation() const;

  void SetLocalScaling(ezVec3 scaling);
  ezVec3 GetLocalScaling() const;

  void SetLocalUniformScaling(float scaling);
  float GetLocalUniformScaling() const;

  void SetGlobalPosition(const ezVec3& position);
  ezVec3 GetGlobalPosition() const;

  void SetGlobalRotation(const ezQuat rotation);
  ezQuat GetGlobalRotation() const;

  void SetGlobalScaling(const ezVec3 scaling);
  ezVec3 GetGlobalScaling() const;

  void SetGlobalTransform(const ezTransform& transform);
  ezTransform GetGlobalTransform() const;

  // Simd variants of above methods
  void SetLocalPosition(const ezSimdVec4f& position);
  const ezSimdVec4f& GetLocalPositionSimd() const;

  void SetLocalRotation(const ezSimdQuat& rotation);
  const ezSimdQuat& GetLocalRotationSimd() const;

  void SetLocalScaling(const ezSimdVec4f& scaling);
  const ezSimdVec4f& GetLocalScalingSimd() const;

  void SetLocalUniformScaling(const ezSimdFloat& scaling);
  ezSimdFloat GetLocalUniformScalingSimd() const;

  void SetGlobalPosition(const ezSimdVec4f& position);
  const ezSimdVec4f& GetGlobalPositionSimd() const;

  void SetGlobalRotation(const ezSimdQuat& rotation);
  const ezSimdQuat& GetGlobalRotationSimd() const;

  void SetGlobalScaling(const ezSimdVec4f& scaling);
  const ezSimdVec4f& GetGlobalScalingSimd() const;

  void SetGlobalTransform(const ezSimdTransform& transform);
  const ezSimdTransform& GetGlobalTransformSimd() const;

  /// \brief Returns the 'forwards' direction of the world's ezCoordinateSystem, rotated into the object's global space
  ezVec3 GetGlobalDirForwards() const;
  /// \brief Returns the 'right' direction of the world's ezCoordinateSystem, rotated into the object's global space
  ezVec3 GetGlobalDirRight() const;
  /// \brief Returns the 'up' direction of the world's ezCoordinateSystem, rotated into the object's global space
  ezVec3 GetGlobalDirUp() const;

  /// \brief Sets the object's velocity.
  ///
  /// This is used for some rendering techniques or for the computation of sound Doppler effect.
  /// It has no effect on the object's subsequent position.
  void SetVelocity(const ezVec3& vVelocity);

  /// \brief Returns the velocity of the object in units per second. This is not only the diff between last frame's position and this
  /// frame's position, but
  ///        also the time difference is divided out.
  ezVec3 GetVelocity() const;

  /// \brief Updates the global transform immediately. Usually this done during the world update after the "Post-async" phase.
  void UpdateGlobalTransform();


  ezBoundingBoxSphere GetLocalBounds() const;
  ezBoundingBoxSphere GetGlobalBounds() const;

  const ezSimdBBoxSphere& GetLocalBoundsSimd() const;
  const ezSimdBBoxSphere& GetGlobalBoundsSimd() const;

  /// \brief Invalidates the local bounds and sends a message to all components so they can add their bounds.
  void UpdateLocalBounds();

  /// \brief Updates the global transform and bounds immediately. Usually this done during the world update after the "Post-async" phase.
  void UpdateGlobalTransformAndBounds();


  /// \brief Returns a handle to the internal spatial data.
  ezSpatialDataHandle GetSpatialData() const;

  /// \brief Enables or disabled notification message when components are added or removed. The notification message is sent to this object and all its parent objects.
  void EnableComponentChangesNotifications();
  void DisableComponentChangesNotifications();

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  template <typename T>
  bool TryGetComponentOfBaseType(T*& out_pComponent);

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  template <typename T>
  bool TryGetComponentOfBaseType(const T*& out_pComponent) const;

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  bool TryGetComponentOfBaseType(const ezRTTI* pType, ezComponent*& out_pComponent);

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  bool TryGetComponentOfBaseType(const ezRTTI* pType, const ezComponent*& out_pComponent) const;

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  template <typename T>
  void TryGetComponentsOfBaseType(ezHybridArray<T*, 8>& out_components);

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  template <typename T>
  void TryGetComponentsOfBaseType(ezHybridArray<const T*, 8>& out_components) const;

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  void TryGetComponentsOfBaseType(const ezRTTI* pType, ezHybridArray<ezComponent*, 8>& out_components);

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  void TryGetComponentsOfBaseType(const ezRTTI* pType, ezHybridArray<const ezComponent*, 8>& out_components) const;

  /// \brief Returns a list of all components attached to this object.
  ezArrayPtr<ezComponent* const> GetComponents();

  /// \brief Returns a list of all components attached to this object.
  ezArrayPtr<const ezComponent* const> GetComponents() const;


  /// \brief Sends a message to all components of this object.
  bool SendMessage(ezMessage& msg);

  /// \brief Sends a message to all components of this object.
  bool SendMessage(ezMessage& msg) const;

  /// \brief Sends a message to all components of this object and then recursively to all children.
  bool SendMessageRecursive(ezMessage& msg);

  /// \brief Sends a message to all components of this object and then recursively to all children.
  bool SendMessageRecursive(ezMessage& msg) const;


  /// \brief Queues the message for the given phase. The message is processed after the given delay in the corresponding phase.
  void PostMessage(const ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay = ezTime()) const;

  /// \brief Queues the message for the given phase. The message is processed after the given delay in the corresponding phase.
  void PostMessageRecursive(const ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay = ezTime()) const;

  /// \brief Returns the tag set associated with this object.
  ezTagSet& GetTags();
  const ezTagSet& GetTags() const;

  /// \brief Returns the 'team ID' that was given during creation (/see ezGameObjectDesc)
  ///
  /// It is automatically passed on to objects created by this object.
  /// This makes it possible to identify which player or team an object belongs to.
  const ezUInt16& GetTeamID() const { return m_uiTeamID; }

  /// \brief Changes the team ID for this object and all children recursively.
  void SetTeamID(ezUInt16 id);

private:
  friend class ezComponentManagerBase;
  friend class ezGameObjectTest;

  EZ_ALLOW_PRIVATE_PROPERTIES(ezGameObject);

  // Add / Detach child used by the reflected property keep their local transform as
  // updating that is handled by the editor.
  void Reflection_AddChild(ezGameObject* pChild);
  void Reflection_DetachChild(ezGameObject* pChild);
  ezHybridArray<ezGameObject*, 8> Reflection_GetChildren() const;
  void Reflection_AddComponent(ezComponent* pComponent);
  void Reflection_RemoveComponent(ezComponent* pComponent);
  const ezHybridArray<ezComponent*, NUM_INPLACE_COMPONENTS>& Reflection_GetComponents() const;

  ezObjectMode::Enum Reflection_GetMode() const;
  void Reflection_SetMode(ezObjectMode::Enum mode);

  bool DetermineDynamicMode(ezComponent* pComponentToIgnore = nullptr) const;
  void ConditionalMakeStatic(ezComponent* pComponentToIgnore = nullptr);
  void MakeStaticInternal();

  void UpdateGlobalTransformAndBoundsRecursive();

  void OnDeleteObject(ezMsgDeleteGameObject& msg);

  void AddComponent(ezComponent* pComponent);
  void RemoveComponent(ezComponent* pComponent);
  void FixComponentPointer(ezComponent* pOldPtr, ezComponent* pNewPtr);

  void SendNotificationMessage(ezMessage& msg);

  struct EZ_CORE_DLL EZ_ALIGN_16(TransformationData)
  {
    EZ_DECLARE_POD_TYPE();

    ezGameObject* m_pObject;
    TransformationData* m_pParentData;

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
    ezUInt64 m_uiPadding;
#endif

    ezSimdVec4f m_localPosition;
    ezSimdQuat m_localRotation;
    ezSimdVec4f m_localScaling; // x,y,z = non-uniform scaling, w = uniform scaling

    ezSimdTransform m_globalTransform;
    ezSimdVec4f m_lastGlobalPosition;
    ezSimdVec4f m_velocity; // w != 0 indicates custom velocity

    ezSimdBBoxSphere m_localBounds; // m_BoxHalfExtents.w != 0 indicates that the object should be always visible
    ezSimdBBoxSphere m_globalBounds;

    ezSpatialDataHandle m_hSpatialData;

    ezUInt32 m_uiPadding2[3];

    void UpdateLocalTransform();

    void ConditionalUpdateGlobalTransform();
    void UpdateGlobalTransform();
    void UpdateGlobalTransformWithParent();

    void ConditionalUpdateGlobalBounds();
    void UpdateGlobalBounds();

    void UpdateVelocity(const ezSimdFloat& fInvDeltaSeconds);

    void UpdateSpatialData(bool bWasAlwaysVisible, bool bIsAlwaysVisible);
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

  /// An int that will be passed on to objects spawned from this one, which allows to identify which team or player it belongs to.
  ezUInt16 m_uiTeamID;

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

