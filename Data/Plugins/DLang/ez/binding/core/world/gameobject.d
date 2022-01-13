module ez.core.world.gameobject;

public import ez.foundation.math.vec3;
public import ez.foundation.math.quat;
public import ez.foundation.math.transform;
public import ez.foundation.reflection.reflectedclass;
public import ez.core.world.world;
public import ez.core.world.component;

extern(C++, struct) struct ezGameObjectHandle
{
  // CODEGEN-BEGIN: Struct("ezGameObjectHandle")
  // Operator: %
  // constructor - unsupported argument 'internalId'
  // Operator: ==
  // Operator: !=
  // Operator: <
  void Invalidate();
  bool IsInvalidated() const;
  // method 'GetInternalID' - unsupported return type
  // Typedef 'IdType' has unsupported type
  // Field 'm_InternalId' has unsupported type
  // converter (not implemented)
  // converter (not implemented)
  // Operator: =
  // CODEGEN-END
}

extern(C++, class) class ezGameObject
{
  // CODEGEN-BEGIN: Class("ezGameObject")
private:
  enum  : int
  {
    NUM_INPLACE_COMPONENTS = 12,
  }
  // Operator: =
  // sub-class (not implemented)
  // sub-class (not implemented)
public:
  final ezGameObjectHandle GetHandle() const;
  final void MakeDynamic();
  final void MakeStatic();
  final bool IsDynamic() const;
  final bool IsStatic() const;
  final void SetActiveFlag(bool bEnabled);
  final bool GetActiveFlag() const;
  final bool IsActive() const;
  final void SetName(const(char)* szName);
  // method 'SetName' - unsupported argument 'sName'
  final const(char)* GetName() const;
  // method 'HasName' - unsupported argument 'name'
  final void SetGlobalKey(const(char)* szGlobalKey);
  // method 'SetGlobalKey' - unsupported argument 'sGlobalKey'
  final const(char)* GetGlobalKey() const;
  final void EnableChildChangesNotifications();
  final void DisableChildChangesNotifications();
  enum TransformPreservation : int
  {
    PreserveLocal = 0,
    PreserveGlobal = 1,
  }
  // method 'SetParent' - unsupported argument 'preserve'
  final ezGameObject GetParent();
  final const(ezGameObject) GetParent() const;
  // method 'AddChild' - unsupported argument 'preserve'
  // method 'AddChildren' - unsupported argument 'children'
  // method 'DetachChild' - unsupported argument 'preserve'
  // method 'DetachChildren' - unsupported argument 'children'
  final uint GetChildCount() const;
  // method 'GetChildren' - unsupported return type
  // method 'GetChildren' - unsupported return type
  // method 'FindChildByName' - unsupported argument 'name'
  final ezGameObject FindChildByPath(const(char)* path);
  final ezGameObject SearchForChildByNameSequence(const(char)* szObjectSequence, const(ezRTTI)* pExpectedComponent = null);
  // method 'SearchForChildrenByNameSequence' - unsupported argument 'out_Objects'
  final ezWorld* GetWorld();
  final const(ezWorld)* GetWorld() const;
  enum UpdateBehaviorIfStatic : int
  {
    None = 0,
    UpdateImmediately = 1,
  }
  final void SetLocalPosition(ezVec3Template!(float) position);
  final ezVec3Template!(float) GetLocalPosition() const;
  final void SetLocalRotation(ezQuatTemplate!(float) rotation);
  final ezQuatTemplate!(float) GetLocalRotation() const;
  final void SetLocalScaling(ezVec3Template!(float) scaling);
  final ezVec3Template!(float) GetLocalScaling() const;
  final void SetLocalUniformScaling(float scaling);
  final float GetLocalUniformScaling() const;
  final ezTransformTemplate!(float) GetLocalTransform() const;
  final void SetGlobalPosition(ref const(ezVec3Template!(float)) position);
  final ezVec3Template!(float) GetGlobalPosition() const;
  final void SetGlobalRotation(const(ezQuatTemplate!(float)) rotation);
  final ezQuatTemplate!(float) GetGlobalRotation() const;
  final void SetGlobalScaling(const(ezVec3Template!(float)) scaling);
  final ezVec3Template!(float) GetGlobalScaling() const;
  final void SetGlobalTransform(ref const(ezTransformTemplate!(float)) transform);
  final ezTransformTemplate!(float) GetGlobalTransform() const;
  // method 'SetLocalPosition' - unsupported argument 'position'
  // method 'GetLocalPositionSimd' - unsupported return type
  // method 'SetLocalRotation' - unsupported argument 'rotation'
  // method 'GetLocalRotationSimd' - unsupported return type
  // method 'SetLocalScaling' - unsupported argument 'scaling'
  // method 'GetLocalScalingSimd' - unsupported return type
  // method 'SetLocalUniformScaling' - unsupported argument 'scaling'
  // method 'GetLocalUniformScalingSimd' - unsupported return type
  // method 'GetLocalTransformSimd' - unsupported return type
  // method 'SetGlobalPosition' - unsupported argument 'position'
  // method 'GetGlobalPositionSimd' - unsupported return type
  // method 'SetGlobalRotation' - unsupported argument 'rotation'
  // method 'GetGlobalRotationSimd' - unsupported return type
  // method 'SetGlobalScaling' - unsupported argument 'scaling'
  // method 'GetGlobalScalingSimd' - unsupported return type
  // method 'SetGlobalTransform' - unsupported argument 'transform'
  // method 'GetGlobalTransformSimd' - unsupported return type
  final ezVec3Template!(float) GetGlobalDirForwards() const;
  final ezVec3Template!(float) GetGlobalDirRight() const;
  final ezVec3Template!(float) GetGlobalDirUp() const;
  final void SetVelocity(ref const(ezVec3Template!(float)) vVelocity);
  final ezVec3Template!(float) GetVelocity() const;
  final void UpdateGlobalTransform();
  final void EnableStaticTransformChangesNotifications();
  final void DisableStaticTransformChangesNotifications();
  // method 'GetLocalBounds' - unsupported return type
  // method 'GetGlobalBounds' - unsupported return type
  // method 'GetLocalBoundsSimd' - unsupported return type
  // method 'GetGlobalBoundsSimd' - unsupported return type
  final void UpdateLocalBounds();
  final void UpdateGlobalBounds();
  final void UpdateGlobalTransformAndBounds();
  // method 'GetSpatialData' - unsupported return type
  final void EnableComponentChangesNotifications();
  final void DisableComponentChangesNotifications();
  final bool TryGetComponentOfBaseType(const(ezRTTI)* pType, ref ezComponent out_pComponent);
  final bool TryGetComponentOfBaseType(const(ezRTTI)* pType, ref const(ezComponent) out_pComponent) const;
  // method 'TryGetComponentsOfBaseType' - unsupported argument 'out_components'
  // method 'TryGetComponentsOfBaseType' - unsupported argument 'out_components'
  // method 'GetComponents' - unsupported return type
  // method 'GetComponents' - unsupported return type
  final ushort GetComponentVersion() const;
  // method 'SendMessage' - unsupported argument 'msg'
  // method 'SendMessage' - unsupported argument 'msg'
  // method 'SendMessageRecursive' - unsupported argument 'msg'
  // method 'SendMessageRecursive' - unsupported argument 'msg'
  // method 'PostMessage' - unsupported argument 'msg'
  // method 'PostMessageRecursive' - unsupported argument 'msg'
  // method 'SendEventMessage' - unsupported argument 'msg'
  // method 'SendEventMessage' - unsupported argument 'msg'
  // method 'PostEventMessage' - unsupported argument 'msg'
  // method 'GetTags' - unsupported return type
  // method 'SetTags' - unsupported argument 'tags'
  // method 'SetTag' - unsupported argument 'tag'
  // method 'RemoveTag' - unsupported argument 'tag'
  final ref const(ushort) GetTeamID() const;
  final void SetTeamID(ushort id);
  final uint GetStableRandomSeed() const;
  final void SetStableRandomSeed(uint seed);
  final ulong GetNumFramesSinceVisible() const;
  struct TransformationData
  {
    // Operator: %
    ezGameObject m_pObject;
    // Field 'm_pParentData' has unsupported type
    ulong m_uiPadding;
    // Field 'm_localPosition' has unsupported type
    // Field 'm_localRotation' has unsupported type
    // Field 'm_localScaling' has unsupported type
    // Field 'm_globalTransform' has unsupported type
    // Field 'm_lastGlobalPosition' has unsupported type
    // Field 'm_velocity' has unsupported type
    // Field 'm_localBounds' has unsupported type
    // Field 'm_globalBounds' has unsupported type
    // Field 'm_hSpatialData' has unsupported type
    uint m_uiSpatialDataCategoryBitmask;
    uint m_uiStableRandomSeed;
    uint[1] m_uiPadding2;
    void UpdateLocalTransform();
    void UpdateGlobalTransformRecursive();
    void UpdateGlobalTransformNonRecursive();
    void UpdateGlobalTransformWithoutParent();
    void UpdateGlobalTransformWithParent();
    // method 'UpdateGlobalBounds' - unsupported argument 'pSpatialSystem'
    void UpdateGlobalBounds();
    // method 'UpdateGlobalBoundsAndSpatialData' - unsupported argument 'spatialSystem'
    // method 'UpdateVelocity' - unsupported argument 'fInvDeltaSeconds'
    // method 'RecreateSpatialData' - unsupported argument 'spatialSystem'
    // Operator: =
  }
  struct ComponentUserData
  {
    ushort m_uiVersion;
    ushort m_uiUnused;
    // Operator: =
  }
  // CODEGEN-END
}
