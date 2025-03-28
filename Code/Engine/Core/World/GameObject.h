#pragma once

/// \file

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/TagSet.h>

#include <Core/World/ComponentManager.h>
#include <Core/World/GameObjectDesc.h>
#include <Core/World/SpatialData.h>

// Avoid conflicts with windows.h
#ifdef SendMessage
#  undef SendMessage
#endif

/// \brief Defines during re-parenting what transform is going to be preserved.
struct ezTransformPreservation
{
  using StorageType = ezUInt8;

  enum Enum : StorageType
  {
    PreserveLocal,
    PreserveGlobal,

    Default = PreserveLocal
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezTransformPreservation);

/// \brief This class represents an object inside the world.
///
/// Game objects only consists of hierarchical data like transformation and a list of components.
/// You cannot derive from the game object class. To add functionality to an object you have to attach components to it.
/// To create an object instance call CreateObject on the world. Never store a direct pointer to an object but store an
/// ezGameObjectHandle instead.
///
/// \see ezWorld
/// \see ezComponent
/// \see ezGameObjectHandle
class EZ_CORE_DLL ezGameObject final
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

    /// \brief Shorthand for 'Next'
    void operator++();

  private:
    friend class ezGameObject;

    ConstChildIterator(ezGameObject* pObject, const ezWorld* pWorld);

    ezGameObject* m_pObject = nullptr;
    const ezWorld* m_pWorld = nullptr;
  };

  class EZ_CORE_DLL ChildIterator : public ConstChildIterator
  {
  public:
    ezGameObject& operator*();
    ezGameObject* operator->();

    operator ezGameObject*();

  private:
    friend class ezGameObject;

    ChildIterator(ezGameObject* pObject, const ezWorld* pWorld);
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

  /// \brief Sets the 'active flag' of the game object, which affects its final 'active state'.
  ///
  /// The active flag affects the 'active state' of the game object and all its children and attached components.
  /// When a game object does not have the active flag, it is switched to 'inactive'. The same happens for all its children and
  /// all components attached to those game objects.
  /// Thus removing the active flag from a game object recursively deactivates the entire sub-tree of objects and components.
  ///
  /// When the active flag is set on a game object, and all of its parent nodes have the flag set as well, then the active state
  /// will be set to true on it and all its children and attached components.
  ///
  /// \sa IsActive(), ezComponent::SetActiveFlag()
  void SetActiveFlag(bool bEnabled);

  /// \brief Checks whether the 'active flag' is set on this game object. Note that this does not mean that the game object is also in an 'active
  /// state'.
  ///
  /// \sa IsActive(), SetActiveFlag()
  bool GetActiveFlag() const;

  /// \brief Checks whether this game object is in an active state.
  ///
  /// The active state is determined by the active state of the parent game object and the 'active flag' of this game object.
  /// Only if the parent game object is active (and thus all of its parent objects as well) and this game object has the active flag set,
  /// will this game object be active.
  ///
  /// \sa ezGameObject::SetActiveFlag(), ezComponent::IsActive()
  bool IsActive() const;

  /// \brief Adds ezObjectFlags::CreatedByPrefab to the object. See the flag for details.
  void SetCreatedByPrefab() { m_Flags.Add(ezObjectFlags::CreatedByPrefab); }

  /// \brief Checks whether the ezObjectFlags::CreatedByPrefab flag is set on this object.
  bool WasCreatedByPrefab() const { return m_Flags.IsSet(ezObjectFlags::CreatedByPrefab); }

  /// \brief Adds ezObjectFlags::HideShapeIcon to the object. See the flag for details.
  void SetHideShapeIcon() { m_Flags.Add(ezObjectFlags::HideShapeIcon); }

  /// \brief Checks whether the ezObjectFlags::HideShapeIcon flag is set on this object.
  bool IsShapeIconHidden() const { return m_Flags.IsSet(ezObjectFlags::HideShapeIcon); }

  /// \brief Sets the name to identify this object. Does not have to be a unique name.
  void SetName(ezStringView sName);
  void SetName(const ezHashedString& sName);
  ezStringView GetName() const;
  const ezHashedString& GetNameHashed() const;
  bool HasName(const ezTempHashedString& sName) const;

  /// \brief Sets the global key to identify this object. Global keys must be unique within a world.
  void SetGlobalKey(ezStringView sGlobalKey);
  void SetGlobalKey(const ezHashedString& sGlobalKey);
  ezStringView GetGlobalKey() const;

  /// \brief Enables or disabled notification message 'ezMsgChildrenChanged' when children are added or removed. The message is sent to this object and all its parent objects.
  void EnableChildChangesNotifications();
  void DisableChildChangesNotifications();

  /// \brief Enables or disabled notification message 'ezMsgParentChanged' when the parent changes. The message is sent to this object only.
  void EnableParentChangesNotifications();
  void DisableParentChangesNotifications();

  /// \brief Sets the parent of this object to the given.
  void SetParent(const ezGameObjectHandle& hParent, ezTransformPreservation::Enum preserve = ezTransformPreservation::PreserveGlobal);

  /// \brief Gets the parent of this object or nullptr if this is a top-level object.
  ezGameObject* GetParent();

  /// \brief Gets the parent of this object or nullptr if this is a top-level object.
  const ezGameObject* GetParent() const;

  /// \brief Adds the given object as a child object.
  void AddChild(const ezGameObjectHandle& hChild, ezTransformPreservation::Enum preserve = ezTransformPreservation::PreserveGlobal);

  /// \brief Adds the given objects as child objects.
  void AddChildren(const ezArrayPtr<const ezGameObjectHandle>& children, ezTransformPreservation::Enum preserve = ezTransformPreservation::PreserveGlobal);

  /// \brief Detaches the given child object from this object and makes it a top-level object.
  void DetachChild(const ezGameObjectHandle& hChild, ezTransformPreservation::Enum preserve = ezTransformPreservation::PreserveGlobal);

  /// \brief Detaches the given child objects from this object and makes them top-level objects.
  void DetachChildren(const ezArrayPtr<const ezGameObjectHandle>& children, ezTransformPreservation::Enum preserve = ezTransformPreservation::PreserveGlobal);

  /// \brief Returns the number of children.
  ezUInt32 GetChildCount() const;

  /// \brief Returns an iterator over all children of this object.
  ChildIterator GetChildren();

  /// \brief Returns an iterator over all children of this object.
  ConstChildIterator GetChildren() const;

  /// \brief Searches for a child object with the given name. Optionally traverses the entire hierarchy.
  ezGameObject* FindChildByName(const ezTempHashedString& sName, bool bRecursive = true); // [tested]

  /// \brief Searches for a child object with the given name. Optionally traverses the entire hierarchy.
  const ezGameObject* FindChildByName(const ezTempHashedString& sName, bool bRecursive = true) const; // [tested]

  /// \brief Searches for a child using a path. Every path segment represents a child with a given name.
  ///
  /// Paths are separated with single slashes: /
  /// When an empty path is given, 'this' is returned.
  /// When on any part of the path the next child cannot be found, nullptr is returned.
  /// This function expects an exact path to the destination. It does not search the full hierarchy for
  /// the next child, as SearchChildByNameSequence() does.
  ezGameObject* FindChildByPath(ezStringView sPath); // [tested]

  /// \brief Const overload of FindChildByPath()
  const ezGameObject* FindChildByPath(ezStringView sPath) const; // [tested]

  /// \brief Searches for a child similar to FindChildByName() but allows to search for multiple names in a sequence.
  ///
  /// The names in the sequence are separated with slashes.
  /// For example, calling this with "a/b" will first search the entire hierarchy below this object for a child
  /// named "a". If that is found, the search continues from there for a child called "b".
  /// If such a child is found and pExpectedComponent != nullptr, it is verified that the object
  /// contains a component of that type. If it doesn't the search continues (including back-tracking).
  ezGameObject* SearchForChildByNameSequence(ezStringView sObjectSequence, const ezRTTI* pExpectedComponent = nullptr); // [tested]

  /// \brief Const overload of SearchForChildByNameSequence()
  const ezGameObject* SearchForChildByNameSequence(ezStringView sObjectSequence, const ezRTTI* pExpectedComponent = nullptr) const; // [tested]

  /// \brief Same as SearchForChildByNameSequence but returns ALL matches, in case the given path could mean multiple objects
  void SearchForChildrenByNameSequence(ezStringView sObjectSequence, const ezRTTI* pExpectedComponent, ezHybridArray<ezGameObject*, 8>& out_objects);

  ezWorld* GetWorld();
  const ezWorld* GetWorld() const;


  /// \brief Defines update behavior for global transforms when changing the local transform on a static game object
  enum class UpdateBehaviorIfStatic
  {
    None,              ///< Only sets the local transform, does not update
    UpdateImmediately, ///< Updates the hierarchy underneath the object immediately
  };

  /// \brief Changes the position of the object local to its parent.
  /// \note The rotation of the object itself does not affect the final global position!
  /// The local position is always in the space of the parent object. If there is no parent, local position and global position are
  /// identical.
  void SetLocalPosition(ezVec3 vPosition);
  ezVec3 GetLocalPosition() const;

  void SetLocalRotation(ezQuat qRotation);
  ezQuat GetLocalRotation() const;

  void SetLocalScaling(ezVec3 vScaling);
  ezVec3 GetLocalScaling() const;

  void SetLocalUniformScaling(float fScaling);
  float GetLocalUniformScaling() const;

  ezTransform GetLocalTransform() const;

  void SetGlobalPosition(const ezVec3& vPosition);
  ezVec3 GetGlobalPosition() const;

  void SetGlobalRotation(const ezQuat& qRotation);
  ezQuat GetGlobalRotation() const;

  void SetGlobalScaling(const ezVec3& vScaling);
  ezVec3 GetGlobalScaling() const;

  void SetGlobalTransform(const ezTransform& transform);
  ezTransform GetGlobalTransform() const;

  /// \brief Last frame's global transform (only valid if EZ_GAMEOBJECT_VELOCITY is set, otherwise the same as GetGlobalTransform())
  ezTransform GetLastGlobalTransform() const;

  // Simd variants of above methods
  void SetLocalPosition(const ezSimdVec4f& vPosition, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  const ezSimdVec4f& GetLocalPositionSimd() const;

  void SetLocalRotation(const ezSimdQuat& qRotation, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  const ezSimdQuat& GetLocalRotationSimd() const;

  void SetLocalScaling(const ezSimdVec4f& vScaling, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  const ezSimdVec4f& GetLocalScalingSimd() const;

  void SetLocalUniformScaling(const ezSimdFloat& fScaling, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  ezSimdFloat GetLocalUniformScalingSimd() const;

  ezSimdTransform GetLocalTransformSimd() const;

  void SetGlobalPosition(const ezSimdVec4f& vPosition);
  const ezSimdVec4f& GetGlobalPositionSimd() const;

  void SetGlobalRotation(const ezSimdQuat& qRotation);
  const ezSimdQuat& GetGlobalRotationSimd() const;

  void SetGlobalScaling(const ezSimdVec4f& vScaling);
  const ezSimdVec4f& GetGlobalScalingSimd() const;

  void SetGlobalTransform(const ezSimdTransform& transform);
  const ezSimdTransform& GetGlobalTransformSimd() const;

  const ezSimdTransform& GetLastGlobalTransformSimd() const;

  /// \brief Returns the 'forwards' direction of the world's ezCoordinateSystem, rotated into the object's global space
  ezVec3 GetGlobalDirForwards() const;
  /// \brief Returns the 'right' direction of the world's ezCoordinateSystem, rotated into the object's global space
  ezVec3 GetGlobalDirRight() const;
  /// \brief Returns the 'up' direction of the world's ezCoordinateSystem, rotated into the object's global space
  ezVec3 GetGlobalDirUp() const;

#if EZ_ENABLED(EZ_GAMEOBJECT_VELOCITY)
  /// \brief The last global transform is used to calculate the object's velocity. By default this is set automatically to the global transform of the last frame.
  ///
  /// It might make sense to manually override the last global transform to e.g. indicate an object has been teleported instead of moved.
  void SetLastGlobalTransform(const ezSimdTransform& transform);

  /// \brief Returns the linear velocity of the object in units per second. This is only guaranteed to be correct in the PostTransform phase.
  ezVec3 GetLinearVelocity() const;

  /// \brief Returns the angular velocity of the object in radians per second. This is only guaranteed to be correct in the PostTransform phase.
  ezVec3 GetAngularVelocity() const;
#endif

  /// \brief Updates the global transform immediately. Usually this done during the world update after the "Post-async" phase.
  void UpdateGlobalTransform();

  /// \brief Enables or disabled notification message 'ezMsgTransformChanged' when this object is static and its transform changes.
  /// The notification message is sent to this object and thus also to all its components.
  void EnableStaticTransformChangesNotifications();
  void DisableStaticTransformChangesNotifications();


  ezBoundingBoxSphere GetLocalBounds() const;
  ezBoundingBoxSphere GetGlobalBounds() const;

  const ezSimdBBoxSphere& GetLocalBoundsSimd() const;
  const ezSimdBBoxSphere& GetGlobalBoundsSimd() const;

  /// \brief Invalidates the local bounds and sends a message to all components so they can add their bounds.
  void UpdateLocalBounds();

  /// \brief Updates the global bounds immediately. Usually this done during the world update after the "Post-async" phase.
  /// Note that this function does not ensure that the global transform is up-to-date. Use UpdateGlobalTransformAndBounds if you want to update both.
  void UpdateGlobalBounds();

  /// \brief Updates the global transform and bounds immediately. Usually this done during the world update after the "Post-async" phase.
  void UpdateGlobalTransformAndBounds();


  /// \brief Returns a handle to the internal spatial data.
  ezSpatialDataHandle GetSpatialData() const;

  /// \brief Enables or disabled notification message 'ezMsgComponentsChanged' when components are added or removed. The message is sent to this object and all its parent objects.
  void EnableComponentChangesNotifications();
  void DisableComponentChangesNotifications();

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  template <typename T>
  [[nodiscard]] bool TryGetComponentOfBaseType(T*& out_pComponent);

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  template <typename T>
  [[nodiscard]] bool TryGetComponentOfBaseType(const T*& out_pComponent) const;

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  [[nodiscard]] bool TryGetComponentOfBaseType(const ezRTTI* pType, ezComponent*& out_pComponent);

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  [[nodiscard]] bool TryGetComponentOfBaseType(const ezRTTI* pType, const ezComponent*& out_pComponent) const;

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  template <typename T>
  void TryGetComponentsOfBaseType(ezDynamicArray<T*>& out_components);

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  template <typename T>
  void TryGetComponentsOfBaseType(ezDynamicArray<const T*>& out_components) const;

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  void TryGetComponentsOfBaseType(const ezRTTI* pType, ezDynamicArray<ezComponent*>& out_components);

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  void TryGetComponentsOfBaseType(const ezRTTI* pType, ezDynamicArray<const ezComponent*>& out_components) const;

  /// \brief Returns a list of all components attached to this object.
  ezArrayPtr<ezComponent* const> GetComponents();

  /// \brief Returns a list of all components attached to this object.
  ezArrayPtr<const ezComponent* const> GetComponents() const;

  /// \brief Returns the current version of components attached to this object.
  /// This version is increased whenever components are added or removed and can be used for cache validation.
  ezUInt16 GetComponentVersion() const;


  /// \brief Sends a message to all components of this object.
  ///
  /// Returns true, if there was any recipient for this type of message.
  bool SendMessage(ezMessage& ref_msg);

  /// \brief Sends a message to all components of this object.
  ///
  /// Returns true, if there was any recipient for this type of message.
  bool SendMessage(ezMessage& ref_msg) const;

  /// \brief Sends a message to all components of this object and then recursively to all children.
  ///
  /// Returns true, if there was any recipient for this type of message.
  bool SendMessageRecursive(ezMessage& ref_msg);

  /// \brief Sends a message to all components of this object and then recursively to all children.
  ///
  /// Returns true, if there was any recipient for this type of message.
  bool SendMessageRecursive(ezMessage& ref_msg) const;


  /// \brief Queues the message for the given phase. The message is processed after the given delay in the corresponding phase.
  void PostMessage(const ezMessage& msg, ezTime delay, ezObjectMsgQueueType::Enum queueType = ezObjectMsgQueueType::NextFrame) const;

  /// \brief Queues the message for the given phase. The message is processed after the given delay in the corresponding phase.
  void PostMessageRecursive(const ezMessage& msg, ezTime delay, ezObjectMsgQueueType::Enum queueType = ezObjectMsgQueueType::NextFrame) const;

  /// \brief Delivers an ezEventMessage to the closest (parent) object containing an ezEventMessageHandlerComponent.
  ///
  /// Regular SendMessage() and PostMessage() send a message directly to the target object (and all attached components).
  /// SendMessageRecursive() and PostMessageRecursive() send a message 'down' the graph to the target object and all children.
  ///
  /// In contrast, SendEventMessage() / PostEventMessage() bubble the message 'up' the graph.
  /// They do so by inspecting the chain of parent objects for the existence of an ezEventMessageHandlerComponent
  /// (typically a script component). If such a component is found, the message is delivered to it directly, and no other component.
  /// If it is found, but does not handle this type of message, the message is discarded and NOT tried to be delivered
  /// to anyone else.
  ///
  /// If no such component is found in all parent objects, the message is delivered to one ezEventMessageHandlerComponent
  /// instances that is set to 'handle global events' (typically used for level-logic scripts), no matter where in the graph it resides.
  /// If multiple global event handler component exist that handle the same message type, the result is non-deterministic.
  ///
  /// \param msg The message to deliver.
  /// \param senderComponent The component that triggered the event in the first place. May be nullptr.
  ///        If not null, this information is stored in \a msg as ezEventMessage::m_hSenderObject and ezEventMessage::m_hSenderComponent.
  ///        This information is used to pass through more contextual information for the event handler.
  ///        For instance, a trigger component would pass through itself.
  ///        A projectile component sending a 'take damage event' to the hit object, would also pass through itself (the projectile)
  ///        such that the handling code can detect which object was responsible for the damage (and using the ezGameObject's team-ID,
  ///        it can detect which player fired the projectile).
  bool SendEventMessage(ezMessage& ref_msg, const ezComponent* pSenderComponent);

  /// \copydoc ezGameObject::SendEventMessage()
  bool SendEventMessage(ezMessage& ref_msg, const ezComponent* pSenderComponent) const;

  /// \copydoc ezGameObject::SendEventMessage()
  ///
  /// \param queueType In which update phase to deliver the message.
  /// \param delay An optional delay before delivering the message.
  void PostEventMessage(ezMessage& ref_msg, const ezComponent* pSenderComponent, ezTime delay, ezObjectMsgQueueType::Enum queueType = ezObjectMsgQueueType::NextFrame) const;


  /// \brief Returns the tag set associated with this object.
  const ezTagSet& GetTags() const;

  /// \brief Sets the tag set associated with this object.
  void SetTags(const ezTagSet& tags);

  /// \brief Adds the given tag to the object's tags.
  void SetTag(const ezTag& tag);

  /// \brief Removes the given tag from the object's tags.
  void RemoveTag(const ezTag& tag);

  /// \brief Checks whether this object has the given tag.
  bool HasTag(const ezTempHashedString& sTagName) const;

  /// \brief Returns the 'team ID' that was given during creation (/see ezGameObjectDesc)
  ///
  /// It is automatically passed on to objects created by this object.
  /// This makes it possible to identify which player or team an object belongs to.
  const ezUInt16& GetTeamID() const { return m_uiTeamID; }

  /// \brief Changes the team ID for this object and all children recursively.
  void SetTeamID(ezUInt16 uiId);

  /// \brief Returns a random value that is chosen once during object creation and remains stable even throughout serialization.
  ///
  /// This value is intended to be used for choosing random variations of components. For instance, if a component has two
  /// different meshes it can use for variation, this seed should be used to decide which one to use.
  ///
  /// The stable random seed can also be set from the outside, which is what the editor does, to assign a truly stable seed value.
  /// Therefore, each object placed in the editor will always have the same seed value, and objects won't change their appearance
  /// on every run of the game.
  ///
  /// The stable seed is also propagated through prefab instances, such that every prefab instance gets a different value, but
  /// in a deterministic fashion.
  ezUInt32 GetStableRandomSeed() const;

  /// \brief Overwrites the object's random seed value.
  ///
  /// See \a GetStableRandomSeed() for details.
  ///
  /// It should not be necessary to manually change this value, unless you want to make the seed deterministic according to a custom rule.
  void SetStableRandomSeed(ezUInt32 uiSeed);

  /// \brief Retrieves a state describing how visible the object is.
  ///
  /// An object may be invisible, fully visible, or indirectly visible (through shadows or reflections).
  /// This can be used to adjust the update logic of objects.
  /// An invisible object may stop updating entirely. An indirectly visible object may reduce its update rate.
  ///
  /// \param uiNumFramesBeforeInvisible Used to treat an object that was visible and just became invisible as visible for a few more frames.
  ezVisibilityState::Enum GetVisibilityState(ezUInt32 uiNumFramesBeforeInvisible = 5) const;

private:
  friend class ezComponentManagerBase;
  friend class ezGameObjectTest;

  // only needed until reflection can deal with ezStringView
  void SetNameInternal(const char* szName);
  const char* GetNameInternal() const;
  void SetGlobalKeyInternal(const char* szKey);
  const char* GetGlobalKeyInternal() const;

  bool SendMessageInternal(ezMessage& msg, bool bWasPostedMsg);
  bool SendMessageInternal(ezMessage& msg, bool bWasPostedMsg) const;
  bool SendMessageRecursiveInternal(ezMessage& msg, bool bWasPostedMsg);
  bool SendMessageRecursiveInternal(ezMessage& msg, bool bWasPostedMsg) const;

  EZ_ALLOW_PRIVATE_PROPERTIES(ezGameObject);

  void Reflection_SetTag(const char* szTagName);
  void Reflection_RemoveTag(const char* szTagName);

  // Add / Detach child used by the reflected property keep their local transform as
  // updating that is handled by the editor.
  void Reflection_AddChild(ezGameObject* pChild);
  void Reflection_DetachChild(ezGameObject* pChild);
  ezHybridArray<ezGameObject*, 8> Reflection_GetChildren() const;
  void Reflection_AddComponent(ezComponent* pComponent);
  void Reflection_RemoveComponent(ezComponent* pComponent);
  ezHybridArray<ezComponent*, NUM_INPLACE_COMPONENTS> Reflection_GetComponents() const;

  ezGameObject* Reflection_FindChildByName(const ezTempHashedString& sName, bool bRecursive) { return FindChildByName(sName, bRecursive); }
  ezGameObject* Reflection_FindChildByPath(ezStringView sPath) { return FindChildByPath(sPath); }

  ezObjectMode::Enum Reflection_GetMode() const;
  void Reflection_SetMode(ezObjectMode::Enum mode);

  ezGameObject* Reflection_GetParent() const;
  void Reflection_SetGlobalPosition(const ezVec3& vPosition);
  void Reflection_SetGlobalRotation(const ezQuat& qRotation);
  void Reflection_SetGlobalScaling(const ezVec3& vScaling);
  void Reflection_SetGlobalTransform(const ezTransform& transform);

  bool DetermineDynamicMode(ezComponent* pComponentToIgnore = nullptr) const;
  void ConditionalMakeStatic(ezComponent* pComponentToIgnore = nullptr);
  void MakeStaticInternal();

  void UpdateGlobalTransformAndBoundsRecursive();
  void UpdateLastGlobalTransform();

  void OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg);

  void AddComponent(ezComponent* pComponent);
  void RemoveComponent(ezComponent* pComponent);
  void FixComponentPointer(ezComponent* pOldPtr, ezComponent* pNewPtr);

  // Updates the active state of this object and all children and attached components recursively, depending on the enabled states.
  void UpdateActiveState(bool bParentActive);

  void SendNotificationMessage(ezMessage& msg);

  struct EZ_CORE_DLL alignas(16) TransformationData
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

#if EZ_ENABLED(EZ_GAMEOBJECT_VELOCITY)
    ezSimdTransform m_lastGlobalTransform;
#endif

    ezSimdBBoxSphere m_localBounds; // m_BoxHalfExtents.w != 0 indicates that the object should be always visible
    ezSimdBBoxSphere m_globalBounds;

    ezSpatialDataHandle m_hSpatialData;
    ezUInt32 m_uiSpatialDataCategoryBitmask;

    ezUInt32 m_uiStableRandomSeed = 0;

#if EZ_ENABLED(EZ_GAMEOBJECT_VELOCITY)
    ezUInt32 m_uiLastGlobalTransformUpdateCounter = 0;
#else
    ezUInt32 m_uiPadding2[1];
#endif

    /// \brief Recomputes the local transform from this object's global transform and, if available, the parent's global transform.
    void UpdateLocalTransform();

    /// \brief Calls UpdateGlobalTransformWithoutParent or UpdateGlobalTransformWithParent depending on whether there is a parent transform.
    /// In case there is a parent transform it also recursively calls itself on the parent transform to ensure everything is up-to-date.
    void UpdateGlobalTransformRecursive(ezUInt32 uiUpdateCounter);

    /// \brief Calls UpdateGlobalTransformWithoutParent or UpdateGlobalTransformWithParent depending on whether there is a parent transform.
    /// Assumes that the parent's global transform is already up to date.
    void UpdateGlobalTransformNonRecursive(ezUInt32 uiUpdateCounter);

    /// \brief Updates the global transform by copying the object's local transform into the global transform.
    /// This is for objects that have no parent.
    void UpdateGlobalTransformWithoutParent(ezUInt32 uiUpdateCounter);

    /// \brief Updates the global transform by combining the parents global transform with this object's local transform.
    /// Assumes that the parent's global transform is already up to date.
    void UpdateGlobalTransformWithParent(ezUInt32 uiUpdateCounter);

    void UpdateGlobalBounds(ezSpatialSystem* pSpatialSystem);
    void UpdateGlobalBounds();
    void UpdateGlobalBoundsAndSpatialData(ezSpatialSystem& ref_spatialSystem);

    void UpdateLastGlobalTransform(ezUInt32 uiUpdateCounter);

    void RecreateSpatialData(ezSpatialSystem& ref_spatialSystem);
  };

  ezGameObjectId m_InternalId;
  ezHashedString m_sName;

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezUInt32 m_uiNamePadding;
#endif

  ezBitflags<ezObjectFlags> m_Flags;

  ezUInt32 m_uiParentIndex = 0;
  ezUInt32 m_uiFirstChildIndex = 0;
  ezUInt32 m_uiLastChildIndex = 0;

  ezUInt32 m_uiNextSiblingIndex = 0;
  ezUInt32 m_uiPrevSiblingIndex = 0;
  ezUInt32 m_uiChildCount = 0;

  ezUInt16 m_uiHierarchyLevel = 0;

  /// An int that will be passed on to objects spawned from this one, which allows to identify which team or player it belongs to.
  ezUInt16 m_uiTeamID = 0;

  TransformationData* m_pTransformationData = nullptr;

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezUInt32 m_uiPadding = 0;
#endif

  ezSmallArrayBase<ezComponent*, NUM_INPLACE_COMPONENTS> m_Components;

  struct ComponentUserData
  {
    ezUInt16 m_uiVersion;
    ezUInt16 m_uiUnused;
  };

  ezTagSet m_Tags;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezGameObject);

#include <Core/World/Implementation/GameObject_inl.h>
