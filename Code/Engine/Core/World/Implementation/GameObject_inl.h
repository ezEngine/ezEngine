
EZ_FORCE_INLINE ezGameObject::ChildIterator::ChildIterator() : m_pObject(nullptr)
{
}

EZ_FORCE_INLINE ezGameObject::ChildIterator::ChildIterator(ezGameObject* pObject) : m_pObject(pObject)
{
}

EZ_FORCE_INLINE ezGameObject& ezGameObject::ChildIterator::operator*() const
{
  return *m_pObject;
}

EZ_FORCE_INLINE ezGameObject* ezGameObject::ChildIterator::operator->() const
{
  return m_pObject;
}

EZ_FORCE_INLINE bool ezGameObject::ChildIterator::IsValid() const
{
  return m_pObject != nullptr;
}

EZ_FORCE_INLINE void ezGameObject::ChildIterator::operator++()
{
  Next();
}
    

////////////////////////////////////////////////////////////////////////////////////////////////////

EZ_FORCE_INLINE ezGameObject::ezGameObject()
{
}

EZ_FORCE_INLINE ezGameObject::ezGameObject(const ezGameObject& other)
{
  *this = other;
}

EZ_FORCE_INLINE ezGameObject::~ezGameObject()
{
}

EZ_FORCE_INLINE ezGameObjectHandle ezGameObject::GetHandle() const
{
  return ezGameObjectHandle(m_InternalId);
}

EZ_FORCE_INLINE bool ezGameObject::IsDynamic() const
{
  return m_Flags.IsSet(ezObjectFlags::Dynamic);
}
  
EZ_FORCE_INLINE bool ezGameObject::IsActive() const
{
  return m_Flags.IsSet(ezObjectFlags::Active);
}

EZ_FORCE_INLINE ezUInt64 ezGameObject::GetPersistentId() const
{
  return m_uiPersistentId;
}

EZ_FORCE_INLINE void ezGameObject::SetParent(const ezGameObjectHandle& parent)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

EZ_FORCE_INLINE ezGameObjectHandle ezGameObject::GetParent() const
{
  return m_Parent;
}

EZ_FORCE_INLINE void ezGameObject::AddChild(const ezGameObjectHandle& child)
{
  AddChildren(ezArrayPtr<const ezGameObjectHandle>(&child, 1));
}

EZ_FORCE_INLINE void ezGameObject::AddChildren(const ezArrayPtr<const ezGameObjectHandle>& children)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

EZ_FORCE_INLINE void ezGameObject::DetachChild(const ezGameObjectHandle& child)
{
  DetachChildren(ezArrayPtr<const ezGameObjectHandle>(&child, 1));
}

EZ_FORCE_INLINE void ezGameObject::DetachChildren(const ezArrayPtr<const ezGameObjectHandle>& children)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

EZ_FORCE_INLINE ezWorld* ezGameObject::GetWorld() const
{
  return m_pWorld;
}

EZ_FORCE_INLINE void ezGameObject::SetLocalPosition(const ezVec3& position)
{
  m_pTransformationData->m_localPosition = position.GetAsPositionVec4();
}

EZ_FORCE_INLINE const ezVec3& ezGameObject::GetLocalPosition() const
{
  return *reinterpret_cast<const ezVec3*>(&m_pTransformationData->m_localPosition);
}

EZ_FORCE_INLINE void ezGameObject::SetLocalRotation(const ezQuat& rotation)
{
  m_pTransformationData->m_localRotation = rotation;
}

EZ_FORCE_INLINE const ezQuat& ezGameObject::GetLocalRotation() const
{
  return m_pTransformationData->m_localRotation;
}

EZ_FORCE_INLINE void ezGameObject::SetLocalScaling(const ezVec3& scaling)
{
  m_pTransformationData->m_localScaling = scaling.GetAsDirectionVec4();
}

EZ_FORCE_INLINE const ezVec3& ezGameObject::GetLocalScaling() const
{
  return *reinterpret_cast<const ezVec3*>(&m_pTransformationData->m_localScaling);
}

EZ_FORCE_INLINE ezMat4 ezGameObject::GetWorldTransform() const
{
  return ezMat4(m_pTransformationData->m_worldRotation, m_pTransformationData->m_worldPosition);
}

EZ_FORCE_INLINE void ezGameObject::SetVelocity(const ezVec3& vVelocity)
{
  *reinterpret_cast<ezVec3*>(&m_pTransformationData->m_velocity) = vVelocity;
}

EZ_FORCE_INLINE const ezVec3& ezGameObject::GetVelocity() const
{
  return *reinterpret_cast<const ezVec3*>(&m_pTransformationData->m_velocity);
}

template <typename T>
bool ezGameObject::TryGetComponentOfType(T*& out_pComponent) const
{
  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    ezComponentHandle component = m_Components[i];
    ezComponent* pComponent = nullptr;
    if (ezComponentManagerBase::IsComponentOfType<T>(component) && TryGetComponent(component, pComponent))
    {
      out_pComponent = static_cast<T*>(pComponent);
      return true;
    }
  }

  return false;
}

template <typename T>
void ezGameObject::TryGetComponentsOfType(ezHybridArray<T*, 8>& out_components) const
{
  out_components.Clear();

  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    ezComponentHandle component = m_Components[i];
    ezComponent* pComponent = nullptr;
    if (ezComponentManagerBase::IsComponentOfType<T>(component) && TryGetComponent(component, pComponent))
    {
      out_components.PushBack(static_cast<T*>(pComponent));
    }
  }
}

EZ_FORCE_INLINE ezArrayPtr<ezComponentHandle> ezGameObject::GetComponents() const
{
  return m_Components;
}

