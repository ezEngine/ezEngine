
EZ_FORCE_INLINE ezGameObject::ChildIterator::ChildIterator() : m_pObject(NULL)
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
  return m_pObject != NULL;
}

EZ_FORCE_INLINE void ezGameObject::ChildIterator::operator++()
{
  Next();
}
    

////////////////////////////////////////////////////////////////////////////////////////////////////

EZ_FORCE_INLINE ezGameObject::ezGameObject()
{
}

EZ_FORCE_INLINE ezGameObject::~ezGameObject()
{
}

EZ_FORCE_INLINE ezGameObjectHandle ezGameObject::GetHandle() const
{
  return ezGameObjectHandle(m_InternalId);
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
  m_pHierarchicalData->m_localPosition = position.GetAsPositionVec4();
}

EZ_FORCE_INLINE const ezVec3& ezGameObject::GetLocalPosition() const
{
  return *reinterpret_cast<const ezVec3*>(&m_pHierarchicalData->m_localPosition);
}

EZ_FORCE_INLINE void ezGameObject::SetLocalRotation(const ezQuat& rotation)
{
  m_pHierarchicalData->m_localRotation = rotation;
}

EZ_FORCE_INLINE const ezQuat& ezGameObject::GetLocalRotation() const
{
  return m_pHierarchicalData->m_localRotation;
}

EZ_FORCE_INLINE void ezGameObject::SetLocalScaling(const ezVec3& scaling)
{
  m_pHierarchicalData->m_localScaling = scaling.GetAsDirectionVec4();
}

EZ_FORCE_INLINE const ezVec3& ezGameObject::GetLocalScaling() const
{
  return *reinterpret_cast<const ezVec3*>(&m_pHierarchicalData->m_localScaling);
}

EZ_FORCE_INLINE ezMat4 ezGameObject::GetWorldTransform() const
{
  return ezMat4(m_pHierarchicalData->m_worldRotation, m_pHierarchicalData->m_worldPosition);
}

EZ_FORCE_INLINE const ezVec3& ezGameObject::GetVelocity() const
{
  return *reinterpret_cast<const ezVec3*>(&m_pHierarchicalData->m_velocity);
}

template <typename T>
T* ezGameObject::GetComponentOfType() const
{
  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    ezComponentHandle component = m_Components[i];
    if (m_pWorld->IsComponentOfType<T>(component))
    {
      return static_cast<T*>(m_pWorld->GetComponent(component));
    }
  }

  return NULL;
}

//template <typename T>
//ezResult ezGameObject::GetComponentsOfType(ezArrayPtr<ezComponentHandle> out_components) const
//{
//}

EZ_FORCE_INLINE ezArrayPtr<ezComponentHandle> ezGameObject::GetComponents() const
{
  return m_Components;
}

EZ_FORCE_INLINE void ezGameObject::SendMessage(ezMessage& msg, MsgRouting::Enum routing /*= MsgRouting::ToChildren*/)
{
  OnMessage(msg, routing);
}
