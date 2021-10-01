
EZ_ALWAYS_INLINE ezGameObject::ConstChildIterator::ConstChildIterator(ezGameObject* pObject, const ezWorld* pWorld)
  : m_pObject(pObject)
  , m_pWorld(pWorld)
{
}

EZ_ALWAYS_INLINE const ezGameObject& ezGameObject::ConstChildIterator::operator*() const
{
  return *m_pObject;
}

EZ_ALWAYS_INLINE const ezGameObject* ezGameObject::ConstChildIterator::operator->() const
{
  return m_pObject;
}

EZ_ALWAYS_INLINE ezGameObject::ConstChildIterator::operator const ezGameObject*() const
{
  return m_pObject;
}

EZ_ALWAYS_INLINE bool ezGameObject::ConstChildIterator::IsValid() const
{
  return m_pObject != nullptr;
}

EZ_ALWAYS_INLINE void ezGameObject::ConstChildIterator::operator++()
{
  Next();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

EZ_ALWAYS_INLINE ezGameObject::ChildIterator::ChildIterator(ezGameObject* pObject, const ezWorld* pWorld)
  : ConstChildIterator(pObject, pWorld)
{
}

EZ_ALWAYS_INLINE ezGameObject& ezGameObject::ChildIterator::operator*()
{
  return *m_pObject;
}

EZ_ALWAYS_INLINE ezGameObject* ezGameObject::ChildIterator::operator->()
{
  return m_pObject;
}

EZ_ALWAYS_INLINE ezGameObject::ChildIterator::operator ezGameObject*()
{
  return m_pObject;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline ezGameObject::ezGameObject() = default;

EZ_ALWAYS_INLINE ezGameObject::ezGameObject(const ezGameObject& other)
{
  *this = other;
}

EZ_ALWAYS_INLINE ezGameObjectHandle ezGameObject::GetHandle() const
{
  return ezGameObjectHandle(m_InternalId);
}

EZ_ALWAYS_INLINE bool ezGameObject::IsDynamic() const
{
  return m_Flags.IsSet(ezObjectFlags::Dynamic);
}

EZ_ALWAYS_INLINE bool ezGameObject::IsStatic() const
{
  return !m_Flags.IsSet(ezObjectFlags::Dynamic);
}

EZ_ALWAYS_INLINE bool ezGameObject::GetActiveFlag() const
{
  return m_Flags.IsSet(ezObjectFlags::ActiveFlag);
}

EZ_ALWAYS_INLINE bool ezGameObject::IsActive() const
{
  return m_Flags.IsSet(ezObjectFlags::ActiveState);
}

EZ_ALWAYS_INLINE void ezGameObject::SetName(const char* szName)
{
  m_sName.Assign(szName);
}

EZ_ALWAYS_INLINE void ezGameObject::SetName(const ezHashedString& sName)
{
  m_sName = sName;
}

EZ_ALWAYS_INLINE void ezGameObject::SetGlobalKey(const char* szGlobalKey)
{
  ezHashedString sGlobalKey;
  sGlobalKey.Assign(szGlobalKey);
  SetGlobalKey(sGlobalKey);
}

EZ_ALWAYS_INLINE const char* ezGameObject::GetName() const
{
  return m_sName.GetString().GetData();
}

EZ_ALWAYS_INLINE bool ezGameObject::HasName(const ezTempHashedString& name) const
{
  return m_sName == name;
}

EZ_ALWAYS_INLINE void ezGameObject::EnableChildChangesNotifications()
{
  m_Flags.Add(ezObjectFlags::ChildChangesNotifications);
}

EZ_ALWAYS_INLINE void ezGameObject::DisableChildChangesNotifications()
{
  m_Flags.Remove(ezObjectFlags::ChildChangesNotifications);
}

EZ_ALWAYS_INLINE void ezGameObject::AddChildren(const ezArrayPtr<const ezGameObjectHandle>& children, ezGameObject::TransformPreservation preserve)
{
  for (ezUInt32 i = 0; i < children.GetCount(); ++i)
  {
    AddChild(children[i], preserve);
  }
}

EZ_ALWAYS_INLINE void ezGameObject::DetachChildren(const ezArrayPtr<const ezGameObjectHandle>& children, ezGameObject::TransformPreservation preserve)
{
  for (ezUInt32 i = 0; i < children.GetCount(); ++i)
  {
    DetachChild(children[i], preserve);
  }
}

EZ_ALWAYS_INLINE ezUInt32 ezGameObject::GetChildCount() const
{
  return m_ChildCount;
}


EZ_ALWAYS_INLINE void ezGameObject::SetLocalPosition(ezVec3 position)
{
  SetLocalPosition(ezSimdConversion::ToVec3(position));
}

EZ_ALWAYS_INLINE ezVec3 ezGameObject::GetLocalPosition() const
{
  return ezSimdConversion::ToVec3(m_pTransformationData->m_localPosition);
}


EZ_ALWAYS_INLINE void ezGameObject::SetLocalRotation(ezQuat rotation)
{
  SetLocalRotation(ezSimdConversion::ToQuat(rotation));
}

EZ_ALWAYS_INLINE ezQuat ezGameObject::GetLocalRotation() const
{
  return ezSimdConversion::ToQuat(m_pTransformationData->m_localRotation);
}


EZ_ALWAYS_INLINE void ezGameObject::SetLocalScaling(ezVec3 scaling)
{
  SetLocalScaling(ezSimdConversion::ToVec3(scaling));
}

EZ_ALWAYS_INLINE ezVec3 ezGameObject::GetLocalScaling() const
{
  return ezSimdConversion::ToVec3(m_pTransformationData->m_localScaling);
}


EZ_ALWAYS_INLINE void ezGameObject::SetLocalUniformScaling(float scaling)
{
  SetLocalUniformScaling(ezSimdFloat(scaling));
}

EZ_ALWAYS_INLINE float ezGameObject::GetLocalUniformScaling() const
{
  return m_pTransformationData->m_localScaling.w();
}

EZ_ALWAYS_INLINE ezTransform ezGameObject::GetLocalTransform() const
{
  return ezSimdConversion::ToTransform(GetLocalTransformSimd());
}


EZ_ALWAYS_INLINE void ezGameObject::SetGlobalPosition(const ezVec3& position)
{
  SetGlobalPosition(ezSimdConversion::ToVec3(position));
}

EZ_ALWAYS_INLINE ezVec3 ezGameObject::GetGlobalPosition() const
{
  return ezSimdConversion::ToVec3(m_pTransformationData->m_globalTransform.m_Position);
}


EZ_ALWAYS_INLINE void ezGameObject::SetGlobalRotation(const ezQuat rotation)
{
  SetGlobalRotation(ezSimdConversion::ToQuat(rotation));
}

EZ_ALWAYS_INLINE ezQuat ezGameObject::GetGlobalRotation() const
{
  return ezSimdConversion::ToQuat(m_pTransformationData->m_globalTransform.m_Rotation);
}


EZ_ALWAYS_INLINE void ezGameObject::SetGlobalScaling(const ezVec3 scaling)
{
  SetGlobalScaling(ezSimdConversion::ToVec3(scaling));
}

EZ_ALWAYS_INLINE ezVec3 ezGameObject::GetGlobalScaling() const
{
  return ezSimdConversion::ToVec3(m_pTransformationData->m_globalTransform.m_Scale);
}


EZ_ALWAYS_INLINE void ezGameObject::SetGlobalTransform(const ezTransform& transform)
{
  SetGlobalTransform(ezSimdConversion::ToTransform(transform));
}

EZ_ALWAYS_INLINE ezTransform ezGameObject::GetGlobalTransform() const
{
  return ezSimdConversion::ToTransform(m_pTransformationData->m_globalTransform);
}


EZ_ALWAYS_INLINE void ezGameObject::SetLocalPosition(const ezSimdVec4f& position, UpdateBehaviorIfStatic updateBehavior)
{
  m_pTransformationData->m_localPosition = position;

  if (IsStatic() && updateBehavior == UpdateBehaviorIfStatic::UpdateImmediately)
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

EZ_ALWAYS_INLINE const ezSimdVec4f& ezGameObject::GetLocalPositionSimd() const
{
  return m_pTransformationData->m_localPosition;
}


EZ_ALWAYS_INLINE void ezGameObject::SetLocalRotation(const ezSimdQuat& rotation, UpdateBehaviorIfStatic updateBehavior)
{
  m_pTransformationData->m_localRotation = rotation;

  if (IsStatic() && updateBehavior == UpdateBehaviorIfStatic::UpdateImmediately)
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

EZ_ALWAYS_INLINE const ezSimdQuat& ezGameObject::GetLocalRotationSimd() const
{
  return m_pTransformationData->m_localRotation;
}


EZ_ALWAYS_INLINE void ezGameObject::SetLocalScaling(const ezSimdVec4f& scaling, UpdateBehaviorIfStatic updateBehavior)
{
  ezSimdFloat uniformScale = m_pTransformationData->m_localScaling.w();
  m_pTransformationData->m_localScaling = scaling;
  m_pTransformationData->m_localScaling.SetW(uniformScale);

  if (IsStatic() && updateBehavior == UpdateBehaviorIfStatic::UpdateImmediately)
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

EZ_ALWAYS_INLINE const ezSimdVec4f& ezGameObject::GetLocalScalingSimd() const
{
  return m_pTransformationData->m_localScaling;
}


EZ_ALWAYS_INLINE void ezGameObject::SetLocalUniformScaling(const ezSimdFloat& scaling, UpdateBehaviorIfStatic updateBehavior)
{
  m_pTransformationData->m_localScaling.SetW(scaling);

  if (IsStatic() && updateBehavior == UpdateBehaviorIfStatic::UpdateImmediately)
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

EZ_ALWAYS_INLINE ezSimdFloat ezGameObject::GetLocalUniformScalingSimd() const
{
  return m_pTransformationData->m_localScaling.w();
}

EZ_ALWAYS_INLINE ezSimdTransform ezGameObject::GetLocalTransformSimd() const
{
  const ezSimdVec4f vScale = m_pTransformationData->m_localScaling * m_pTransformationData->m_localScaling.w();
  return ezSimdTransform(m_pTransformationData->m_localPosition, m_pTransformationData->m_localRotation, vScale);
}


EZ_ALWAYS_INLINE void ezGameObject::SetGlobalPosition(const ezSimdVec4f& position)
{
  m_pTransformationData->m_globalTransform.m_Position = position;

  m_pTransformationData->UpdateLocalTransform();

  if (IsStatic())
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

EZ_ALWAYS_INLINE const ezSimdVec4f& ezGameObject::GetGlobalPositionSimd() const
{
  return m_pTransformationData->m_globalTransform.m_Position;
}


EZ_ALWAYS_INLINE void ezGameObject::SetGlobalRotation(const ezSimdQuat& rotation)
{
  m_pTransformationData->m_globalTransform.m_Rotation = rotation;

  m_pTransformationData->UpdateLocalTransform();

  if (IsStatic())
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

EZ_ALWAYS_INLINE const ezSimdQuat& ezGameObject::GetGlobalRotationSimd() const
{
  return m_pTransformationData->m_globalTransform.m_Rotation;
}


EZ_ALWAYS_INLINE void ezGameObject::SetGlobalScaling(const ezSimdVec4f& scaling)
{
  m_pTransformationData->m_globalTransform.m_Scale = scaling;

  m_pTransformationData->UpdateLocalTransform();

  if (IsStatic())
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

EZ_ALWAYS_INLINE const ezSimdVec4f& ezGameObject::GetGlobalScalingSimd() const
{
  return m_pTransformationData->m_globalTransform.m_Scale;
}


EZ_ALWAYS_INLINE void ezGameObject::SetGlobalTransform(const ezSimdTransform& transform)
{
  m_pTransformationData->m_globalTransform = transform;

  // ezTransformTemplate<Type>::SetLocalTransform will produce NaNs in w components
  // of pos and scale if scale.w is not set to 1 here. This only affects builds that
  // use EZ_SIMD_IMPLEMENTATION_FPU, e.g. arm atm.
  m_pTransformationData->m_globalTransform.m_Scale.SetW(1.0f);
  m_pTransformationData->UpdateLocalTransform();

  if (IsStatic())
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

EZ_ALWAYS_INLINE const ezSimdTransform& ezGameObject::GetGlobalTransformSimd() const
{
  return m_pTransformationData->m_globalTransform;
}

#if EZ_ENABLED(EZ_GAMEOBJECT_VELOCITY)
EZ_ALWAYS_INLINE void ezGameObject::SetVelocity(const ezVec3& vVelocity)
{
  m_pTransformationData->m_velocity = ezSimdVec4f(vVelocity.x, vVelocity.y, vVelocity.z, 1.0f);
}

EZ_ALWAYS_INLINE ezVec3 ezGameObject::GetVelocity() const
{
  return ezSimdConversion::ToVec3(m_pTransformationData->m_velocity);
}
#endif

EZ_ALWAYS_INLINE void ezGameObject::UpdateGlobalTransform()
{
  m_pTransformationData->UpdateGlobalTransformRecursive();
}

EZ_ALWAYS_INLINE void ezGameObject::EnableStaticTransformChangesNotifications()
{
  m_Flags.Add(ezObjectFlags::StaticTransformChangesNotifications);
}

EZ_ALWAYS_INLINE void ezGameObject::DisableStaticTransformChangesNotifications()
{
  m_Flags.Remove(ezObjectFlags::StaticTransformChangesNotifications);
}

EZ_ALWAYS_INLINE ezBoundingBoxSphere ezGameObject::GetLocalBounds() const
{
  return ezSimdConversion::ToBBoxSphere(m_pTransformationData->m_localBounds);
}

EZ_ALWAYS_INLINE ezBoundingBoxSphere ezGameObject::GetGlobalBounds() const
{
  return ezSimdConversion::ToBBoxSphere(m_pTransformationData->m_globalBounds);
}

EZ_ALWAYS_INLINE const ezSimdBBoxSphere& ezGameObject::GetLocalBoundsSimd() const
{
  return m_pTransformationData->m_localBounds;
}

EZ_ALWAYS_INLINE const ezSimdBBoxSphere& ezGameObject::GetGlobalBoundsSimd() const
{
  return m_pTransformationData->m_globalBounds;
}

EZ_ALWAYS_INLINE ezSpatialDataHandle ezGameObject::GetSpatialData() const
{
  return m_pTransformationData->m_hSpatialData;
}

EZ_ALWAYS_INLINE void ezGameObject::EnableComponentChangesNotifications()
{
  m_Flags.Add(ezObjectFlags::ComponentChangesNotifications);
}

EZ_ALWAYS_INLINE void ezGameObject::DisableComponentChangesNotifications()
{
  m_Flags.Remove(ezObjectFlags::ComponentChangesNotifications);
}

template <typename T>
EZ_ALWAYS_INLINE bool ezGameObject::TryGetComponentOfBaseType(T*& out_pComponent)
{
  return TryGetComponentOfBaseType(ezGetStaticRTTI<T>(), (ezComponent*&)out_pComponent);
}

template <typename T>
EZ_ALWAYS_INLINE bool ezGameObject::TryGetComponentOfBaseType(const T*& out_pComponent) const
{
  return TryGetComponentOfBaseType(ezGetStaticRTTI<T>(), (const ezComponent*&)out_pComponent);
}

template <typename T>
void ezGameObject::TryGetComponentsOfBaseType(ezDynamicArray<T*>& out_components)
{
  out_components.Clear();

  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    ezComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf<T>())
    {
      out_components.PushBack(static_cast<T*>(pComponent));
    }
  }
}

template <typename T>
void ezGameObject::TryGetComponentsOfBaseType(ezDynamicArray<const T*>& out_components) const
{
  out_components.Clear();

  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    ezComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf<T>())
    {
      out_components.PushBack(static_cast<const T*>(pComponent));
    }
  }
}

EZ_ALWAYS_INLINE ezArrayPtr<ezComponent* const> ezGameObject::GetComponents()
{
  return m_Components;
}

EZ_ALWAYS_INLINE ezArrayPtr<const ezComponent* const> ezGameObject::GetComponents() const
{
  return ezMakeArrayPtr(const_cast<const ezComponent* const*>(m_Components.GetData()), m_Components.GetCount());
}

EZ_ALWAYS_INLINE ezUInt16 ezGameObject::GetComponentVersion() const
{
  return m_Components.GetUserData<ComponentUserData>().m_uiVersion;
}

EZ_ALWAYS_INLINE bool ezGameObject::SendMessage(ezMessage& msg)
{
  return SendMessageInternal(msg, false);
}

EZ_ALWAYS_INLINE bool ezGameObject::SendMessage(ezMessage& msg) const
{
  return SendMessageInternal(msg, false);
}

EZ_ALWAYS_INLINE bool ezGameObject::SendMessageRecursive(ezMessage& msg)
{
  return SendMessageRecursiveInternal(msg, false);
}

EZ_ALWAYS_INLINE bool ezGameObject::SendMessageRecursive(ezMessage& msg) const
{
  return SendMessageRecursiveInternal(msg, false);
}

EZ_ALWAYS_INLINE const ezTagSet& ezGameObject::GetTags() const
{
  return m_Tags;
}

EZ_ALWAYS_INLINE ezUInt32 ezGameObject::GetStableRandomSeed() const
{
  return m_pTransformationData->m_uiStableRandomSeed;
}

EZ_ALWAYS_INLINE void ezGameObject::SetStableRandomSeed(ezUInt32 seed)
{
  m_pTransformationData->m_uiStableRandomSeed = seed;
}

//////////////////////////////////////////////////////////////////////////

EZ_ALWAYS_INLINE void ezGameObject::TransformationData::UpdateGlobalTransformWithoutParent()
{
  m_globalTransform.m_Position = m_localPosition;
  m_globalTransform.m_Rotation = m_localRotation;
  m_globalTransform.m_Scale = m_localScaling * m_localScaling.w();
}

EZ_ALWAYS_INLINE void ezGameObject::TransformationData::UpdateGlobalTransformWithParent()
{
  const ezSimdVec4f vScale = m_localScaling * m_localScaling.w();
  const ezSimdTransform localTransform(m_localPosition, m_localRotation, vScale);
  m_globalTransform.SetGlobalTransform(m_pParentData->m_globalTransform, localTransform);
}

EZ_FORCE_INLINE void ezGameObject::TransformationData::UpdateGlobalBounds()
{
  m_globalBounds = m_localBounds;
  m_globalBounds.Transform(m_globalTransform);
}

EZ_ALWAYS_INLINE void ezGameObject::TransformationData::UpdateVelocity(const ezSimdFloat& fInvDeltaSeconds)
{
#if EZ_ENABLED(EZ_GAMEOBJECT_VELOCITY)
  // A w value != 0 indicates a custom velocity, don't overwrite it.
  ezSimdVec4b customVel = (m_velocity.Get<ezSwizzle::WWWW>() != ezSimdVec4f::ZeroVector());
  ezSimdVec4f newVel = (m_globalTransform.m_Position - m_lastGlobalPosition) * fInvDeltaSeconds;
  m_velocity = ezSimdVec4f::Select(customVel, m_velocity, newVel);

  m_lastGlobalPosition = m_globalTransform.m_Position;
  m_velocity.SetW(ezSimdFloat::Zero());
#endif
}
