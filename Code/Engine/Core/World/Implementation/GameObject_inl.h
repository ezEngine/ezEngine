
EZ_ALWAYS_INLINE ezGameObject::ConstChildIterator::ConstChildIterator(ezGameObject* pObject) :
  m_pObject(pObject)
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

EZ_ALWAYS_INLINE ezGameObject::ChildIterator::ChildIterator(ezGameObject* pObject) :
  ConstChildIterator(pObject)
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

inline ezGameObject::ezGameObject()
{
  m_ParentIndex = 0;
  m_FirstChildIndex = 0;
  m_LastChildIndex = 0;

  m_NextSiblingIndex = 0;
  m_PrevSiblingIndex = 0;
  m_ChildCount = 0;

  m_uiHierarchyLevel = 0;

  m_pTransformationData = nullptr;
  m_pWorld = nullptr;
}

EZ_ALWAYS_INLINE ezGameObject::ezGameObject(const ezGameObject& other)
{
  *this = other;
}

EZ_ALWAYS_INLINE ezGameObject::~ezGameObject()
{
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

EZ_ALWAYS_INLINE bool ezGameObject::IsActive() const
{
  return m_Flags.IsSet(ezObjectFlags::Active);
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

EZ_ALWAYS_INLINE ezWorld* ezGameObject::GetWorld()
{
  return m_pWorld;
}

EZ_ALWAYS_INLINE const ezWorld* ezGameObject::GetWorld() const
{
  return m_pWorld;
}

EZ_ALWAYS_INLINE void ezGameObject::SetLocalPosition(ezVec3 position)
{
  m_pTransformationData->m_localPosition = ezSimdConversion::ToVec3(position);
}

EZ_ALWAYS_INLINE ezVec3 ezGameObject::GetLocalPosition() const
{
  return ezSimdConversion::ToVec3(m_pTransformationData->m_localPosition);
}

EZ_ALWAYS_INLINE void ezGameObject::SetLocalRotation(ezQuat rotation)
{
  m_pTransformationData->m_localRotation = ezSimdConversion::ToQuat(rotation);
}

EZ_ALWAYS_INLINE ezQuat ezGameObject::GetLocalRotation() const
{
  return ezSimdConversion::ToQuat(m_pTransformationData->m_localRotation);
}

EZ_ALWAYS_INLINE void ezGameObject::SetLocalScaling(ezVec3 scaling)
{
  m_pTransformationData->m_localScaling = ezSimdConversion::ToVec4(scaling.GetAsVec4(m_pTransformationData->m_localScaling.w()));
}

EZ_ALWAYS_INLINE ezVec3 ezGameObject::GetLocalScaling() const
{
  return ezSimdConversion::ToVec3(m_pTransformationData->m_localScaling);
}

EZ_ALWAYS_INLINE void ezGameObject::SetLocalUniformScaling(float scaling)
{
  m_pTransformationData->m_localScaling.SetW(scaling);
}

EZ_ALWAYS_INLINE float ezGameObject::GetLocalUniformScaling() const
{
  return m_pTransformationData->m_localScaling.w();
}

EZ_ALWAYS_INLINE void ezGameObject::SetGlobalPosition(const ezVec3& position)
{
  m_pTransformationData->m_globalTransform.m_Position = ezSimdConversion::ToVec3(position);

  m_pTransformationData->UpdateLocalTransform();
}

EZ_ALWAYS_INLINE ezVec3 ezGameObject::GetGlobalPosition() const
{
  return ezSimdConversion::ToVec3(m_pTransformationData->m_globalTransform.m_Position);
}

EZ_ALWAYS_INLINE void ezGameObject::SetGlobalRotation(const ezQuat rotation)
{
  m_pTransformationData->m_globalTransform.m_Rotation = ezSimdConversion::ToQuat(rotation);

  m_pTransformationData->UpdateLocalTransform();
}

EZ_ALWAYS_INLINE ezQuat ezGameObject::GetGlobalRotation() const
{
  return ezSimdConversion::ToQuat(m_pTransformationData->m_globalTransform.m_Rotation);
}

EZ_ALWAYS_INLINE void ezGameObject::SetGlobalScaling(const ezVec3 scaling)
{
  m_pTransformationData->m_globalTransform.m_Scale = ezSimdConversion::ToVec3(scaling);

  m_pTransformationData->UpdateLocalTransform();
}

EZ_ALWAYS_INLINE ezVec3 ezGameObject::GetGlobalScaling() const
{
  return ezSimdConversion::ToVec3(m_pTransformationData->m_globalTransform.m_Scale);
}

EZ_ALWAYS_INLINE void ezGameObject::SetGlobalTransform(const ezTransform& transform)
{
  m_pTransformationData->m_globalTransform = ezSimdConversion::ToTransform(transform);

  m_pTransformationData->UpdateLocalTransform();
}

EZ_ALWAYS_INLINE void ezGameObject::SetGlobalTransform(const ezSimdTransform& transform)
{
  m_pTransformationData->m_globalTransform = transform;

  m_pTransformationData->UpdateLocalTransform();
}

EZ_ALWAYS_INLINE ezTransform ezGameObject::GetGlobalTransform() const
{
  return ezSimdConversion::ToTransform(m_pTransformationData->m_globalTransform);
}

EZ_ALWAYS_INLINE const ezSimdTransform& ezGameObject::GetGlobalTransformSimd() const
{
  return m_pTransformationData->m_globalTransform;
}

EZ_ALWAYS_INLINE void ezGameObject::SetVelocity(const ezVec3& vVelocity)
{
  m_pTransformationData->m_velocity = ezSimdVec4f(vVelocity.x, vVelocity.y, vVelocity.z, 1.0f);
}

EZ_ALWAYS_INLINE ezVec3 ezGameObject::GetVelocity() const
{
  return ezSimdConversion::ToVec3(m_pTransformationData->m_velocity);
}

EZ_ALWAYS_INLINE void ezGameObject::UpdateGlobalTransform()
{
  m_pTransformationData->ConditionalUpdateGlobalTransform();
}

EZ_ALWAYS_INLINE ezBoundingBoxSphere ezGameObject::GetLocalBounds() const
{
  return ezSimdConversion::ToBBoxSphere(m_pTransformationData->m_localBounds);
}

EZ_ALWAYS_INLINE ezBoundingBoxSphere ezGameObject::GetGlobalBounds() const
{
  return ezSimdConversion::ToBBoxSphere(m_pTransformationData->m_globalBounds);
}

EZ_ALWAYS_INLINE void ezGameObject::UpdateGlobalTransformAndBounds()
{
  m_pTransformationData->ConditionalUpdateGlobalBounds();
}

EZ_ALWAYS_INLINE ezSpatialDataHandle ezGameObject::GetSpatialData() const
{
  return m_pTransformationData->m_hSpatialData;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezGameObject::TryGetComponentOfBaseType(T*& out_pComponent) const
{
  return TryGetComponentOfBaseType(ezGetStaticRTTI<T>(), (ezComponent*&)out_pComponent);
}

template <typename T>
void ezGameObject::TryGetComponentsOfBaseType(ezHybridArray<T*, 8>& out_components) const
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

EZ_ALWAYS_INLINE ezArrayPtr<ezComponent* const> ezGameObject::GetComponents()
{
  return m_Components;
}

EZ_ALWAYS_INLINE ezArrayPtr<const ezComponent* const> ezGameObject::GetComponents() const
{
  return ezMakeArrayPtr(const_cast<const ezComponent*const*>(m_Components.GetData()), m_Components.GetCount());
}

EZ_ALWAYS_INLINE ezTagSet& ezGameObject::GetTags()
{
  return m_Tags;
}

EZ_ALWAYS_INLINE const ezTagSet& ezGameObject::GetTags() const
{
  return m_Tags;
}

//////////////////////////////////////////////////////////////////////////

EZ_ALWAYS_INLINE void ezGameObject::TransformationData::UpdateGlobalTransform()
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
  ezSimdBBoxSphere oldGlobalBounds = m_globalBounds;

  m_globalBounds = m_localBounds;
  m_globalBounds.Transform(m_globalTransform);

  ///\todo find a better place for this
  if (m_globalBounds != oldGlobalBounds)
  {
    UpdateSpatialData();
  }
}

EZ_ALWAYS_INLINE void ezGameObject::TransformationData::UpdateVelocity(const ezSimdFloat& fInvDeltaSeconds)
{
  // A w value != 0 indicates a custom velocity, don't overwrite it.
  ezSimdVec4b customVel = (m_velocity.Get<ezSwizzle::WWWW>() != ezSimdVec4f::ZeroVector());
  ezSimdVec4f newVel = (m_globalTransform.m_Position - m_lastGlobalPosition) * fInvDeltaSeconds;
  m_velocity = ezSimdVec4f::Select(newVel, m_velocity, customVel);

  m_lastGlobalPosition = m_globalTransform.m_Position;
  m_velocity.SetW(ezSimdFloat::Zero());
}


