#include <Core/PCH.h>
#include <Core/World/World.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezGameObject, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
    EZ_ACCESSOR_PROPERTY("GlobalKey", GetGlobalKey, SetGlobalKey),
    EZ_ACCESSOR_PROPERTY("LocalPosition", GetLocalPosition, SetLocalPosition)->AddAttributes(new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("LocalRotation", GetLocalRotation, SetLocalRotation),
    EZ_ACCESSOR_PROPERTY("LocalScaling", GetLocalScaling, SetLocalScaling)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f, 1.0f, 1.0f))),
    EZ_ACCESSOR_PROPERTY("LocalUniformScaling", GetLocalUniformScaling, SetLocalUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_SET_MEMBER_PROPERTY("Tags", m_Tags)->AddAttributes(new ezTagSetWidgetAttribute("Default")),
    EZ_SET_ACCESSOR_PROPERTY("Children", Reflection_GetChildren, Reflection_AddChild, Reflection_DetachChild)->AddFlags(ezPropertyFlags::PointerOwner | ezPropertyFlags::Hidden),
    EZ_SET_ACCESSOR_PROPERTY("Components", Reflection_GetComponents, Reflection_AddComponent, Reflection_RemoveComponent)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezDeleteObjectMessage, OnDeleteObject),
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_STATIC_REFLECTED_TYPE

void ezGameObject::Reflection_AddChild(ezGameObject* pChild)
{
  AddChild(pChild->GetHandle(), TransformPreservation::PreserveLocal);
}

void ezGameObject::Reflection_DetachChild(ezGameObject* pChild)
{
  DetachChild(pChild->GetHandle(), TransformPreservation::PreserveLocal);
}

ezHybridArray<ezGameObject*, 8> ezGameObject::Reflection_GetChildren() const
{
  ConstChildIterator it = GetChildren();

  ezHybridArray<ezGameObject*, 8> all;
  all.Reserve(GetChildCount());

  while (it.IsValid())
  {
    all.PushBack(it.m_pObject);
    ++it;
  }

  return all;
}

void ezGameObject::ConstChildIterator::Next()
{
  m_pObject = m_pObject->m_pWorld->GetObjectUnchecked(m_pObject->m_NextSiblingIndex);
}

void ezGameObject::operator=(const ezGameObject& other)
{
  EZ_ASSERT_DEV(m_pWorld == other.m_pWorld, "Cannot copy between worlds.");

  m_InternalId = other.m_InternalId;
  m_Flags = other.m_Flags;
  m_sName = other.m_sName;

  m_ParentIndex = other.m_ParentIndex;
  m_FirstChildIndex = other.m_FirstChildIndex;
  m_LastChildIndex = other.m_LastChildIndex;

  m_NextSiblingIndex = other.m_NextSiblingIndex;
  m_PrevSiblingIndex = other.m_PrevSiblingIndex;
  m_ChildCount = other.m_ChildCount;

  m_uiHierarchyLevel = other.m_uiHierarchyLevel;
  m_uiTransformationDataIndex = other.m_uiTransformationDataIndex;
  m_pTransformationData = other.m_pTransformationData;
  m_pTransformationData->m_pObject = this;

  m_Components = other.m_Components;
  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    ezComponent* pComponent = m_Components[i];
    EZ_ASSERT_DEV(pComponent->m_pOwner == &other, "");
    pComponent->m_pOwner = this;
  }

  m_Tags = other.m_Tags;
}

void ezGameObject::Activate()
{
  m_Flags.Add(ezObjectFlags::Active);

  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    m_Components[i]->Activate();
  }
}

void ezGameObject::Deactivate()
{
  m_Flags.Remove(ezObjectFlags::Active);

  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    m_Components[i]->Deactivate();
  }
}

void ezGameObject::SetGlobalKey(const ezHashedString& sName)
{
  m_pWorld->SetObjectGlobalKey(this, sName);
}

const char* ezGameObject::GetGlobalKey() const
{
  return m_pWorld->GetObjectGlobalKey(this);
}

void ezGameObject::SetParent(const ezGameObjectHandle& parent, ezGameObject::TransformPreservation preserve)
{
  ezGameObject* pParent = nullptr;
  m_pWorld->TryGetObject(parent, pParent);
  m_pWorld->SetParent(this, pParent, preserve);
}

ezGameObject* ezGameObject::GetParent()
{
  return m_pWorld->GetObjectUnchecked(m_ParentIndex);
}

const ezGameObject* ezGameObject::GetParent() const
{
  return m_pWorld->GetObjectUnchecked(m_ParentIndex);
}

void ezGameObject::AddChild(const ezGameObjectHandle& child, ezGameObject::TransformPreservation preserve)
{
  ezGameObject* pChild = nullptr;
  if (m_pWorld->TryGetObject(child, pChild))
  {
    m_pWorld->SetParent(pChild, this, preserve);
  }
}

void ezGameObject::DetachChild(const ezGameObjectHandle& child, ezGameObject::TransformPreservation preserve)
{
  ezGameObject* pChild = nullptr;
  if (m_pWorld->TryGetObject(child, pChild))
  {
    if (pChild->GetParent() == this)
    {
      m_pWorld->SetParent(pChild, nullptr, preserve);
    }
  }
}

ezGameObject::ChildIterator ezGameObject::GetChildren()
{
  return ChildIterator(m_pWorld->GetObjectUnchecked(m_FirstChildIndex));
}

ezGameObject::ConstChildIterator ezGameObject::GetChildren() const
{
  return ConstChildIterator(m_pWorld->GetObjectUnchecked(m_FirstChildIndex));
}

void ezGameObject::SetGlobalPosition(const ezVec3& position)
{
  /// \test This is not yet tested

  m_pTransformationData->m_globalTransform.m_vPosition = position;
  SetGlobalTransform(m_pTransformationData->m_globalTransform);
}

void ezGameObject::SetGlobalRotation(const ezQuat rotation)
{
  /// \test This is not yet tested

  const ezVec3 vOldScale = m_pTransformationData->m_globalTransform.m_Rotation.GetScalingFactors();
  m_pTransformationData->m_globalTransform.m_Rotation = rotation.GetAsMat3();

  if (!vOldScale.IsZero())
    m_pTransformationData->m_globalTransform.m_Rotation.SetScalingFactors(vOldScale);

  SetGlobalTransform(m_pTransformationData->m_globalTransform);
}

void ezGameObject::SetGlobalScaling(const ezVec3 scaling)
{
  /// \test This is not yet tested

  if (m_pTransformationData->m_globalTransform.m_Rotation.SetScalingFactors(scaling).Failed())
    m_pTransformationData->m_globalTransform.m_Rotation.SetScalingMatrix(scaling);

  SetGlobalTransform(m_pTransformationData->m_globalTransform);
}


void ezGameObject::SetGlobalTransform(const ezTransform& transform)
{
  ezTransform tLocal;

  if (m_pTransformationData->m_pParentData != nullptr)
  {
    tLocal.SetLocalTransform(m_pTransformationData->m_pParentData->m_globalTransform, transform);
  }
  else
  {
    tLocal = transform;
  }

  ezVec3 vPos, vScale;
  tLocal.Decompose(vPos, m_pTransformationData->m_localRotation, vScale);
  m_pTransformationData->m_localPosition = vPos.GetAsVec4(0.0f);
  m_pTransformationData->m_localScaling = vScale.GetAsVec4(1.0f);

  m_pTransformationData->m_globalTransform = transform;
}

void ezGameObject::UpdateLocalBounds()
{
  ezUpdateLocalBoundsMessage msg;
  msg.m_ResultingLocalBounds.SetInvalid();

  SendMessage(msg);

  m_pTransformationData->m_localBounds = msg.m_ResultingLocalBounds;
}

ezResult ezGameObject::AttachComponent(const ezComponentHandle& component)
{
  ezComponent* pComponent = nullptr;
  if (m_pWorld->TryGetComponent(component, pComponent))
  {
    return AttachComponent(pComponent);
  }

  return EZ_FAILURE;
}

ezResult ezGameObject::AttachComponent(ezComponent* pComponent)
{
  EZ_ASSERT_DEV(pComponent->m_pOwner == nullptr, "Component must not be added twice.");
  EZ_ASSERT_DEV(IsDynamic() || !pComponent->IsDynamic(),
                "Cannot attach a dynamic component to a static object. Call MakeDynamic() first.");

  pComponent->m_pOwner = this;
  m_Components.PushBack(pComponent);

  pComponent->OnAfterAttachedToObject();

  return EZ_SUCCESS;
}

ezResult ezGameObject::DetachComponent(const ezComponentHandle& component)
{
  ezComponent* pComponent = nullptr;
  if (m_pWorld->TryGetComponent(component, pComponent))
  {
    return DetachComponent(pComponent);
  }

  return EZ_FAILURE;
}

ezResult ezGameObject::DetachComponent(ezComponent* pComponent)
{
  ezUInt32 uiIndex = m_Components.IndexOf(pComponent);
  if (uiIndex == ezInvalidIndex)
    return EZ_FAILURE;

  if (pComponent->IsInitialized())
  {
    pComponent->OnBeforeDetachedFromObject();
  }

  pComponent->m_pOwner = nullptr;
  m_Components.RemoveAtSwap(uiIndex);

  return EZ_SUCCESS;
}


bool ezGameObject::TryGetComponentOfBaseType(const ezRTTI* pType, ezComponent*& out_pComponent) const
{
  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    ezComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf(pType))
    {
      out_pComponent = pComponent;
      return true;
    }
  }

  out_pComponent = nullptr;
  return false;
}


void ezGameObject::TryGetComponentsOfBaseType(const ezRTTI* pType, ezHybridArray<ezComponent*, 8>& out_components) const
{
  out_components.Clear();

  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    ezComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf(pType))
    {
      out_components.PushBack(pComponent);
    }
  }
}

void ezGameObject::OnDeleteObject(ezDeleteObjectMessage& msg)
{
  m_pWorld->DeleteObjectNow(GetHandle());
}

void ezGameObject::FixComponentPointer(ezComponent* pOldPtr, ezComponent* pNewPtr)
{
  ezUInt32 uiIndex = m_Components.IndexOf(pOldPtr);
  EZ_ASSERT_DEV(uiIndex != ezInvalidIndex, "Memory corruption?");
  m_Components[uiIndex] = pNewPtr;
}

void ezGameObject::PostMessage(ezMessage& msg, ezObjectMsgQueueType::Enum queueType,
                               ezObjectMsgRouting::Enum routing)
{
  m_pWorld->PostMessage(GetHandle(), msg, queueType, routing);
}

void ezGameObject::PostMessage(ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay,
                               ezObjectMsgRouting::Enum routing)
{
  m_pWorld->PostMessage(GetHandle(), msg, queueType, delay, routing);
}

void ezGameObject::SendMessage(ezMessage& msg, ezObjectMsgRouting::Enum routing)
{
  if (routing == ezObjectMsgRouting::ToSubTree)
  {
    // walk up the sub tree and send to children from there to prevent double handling
    ezGameObject* pCurrent = this;
    ezGameObject* pParent = GetParent();
    while (pParent != nullptr)
    {
      pCurrent = pParent;
      pParent = pCurrent->GetParent();
    }

    pCurrent->SendMessage(msg, ezObjectMsgRouting::ToChildren);
    return;
  }

  // all cases, including ToObjectOnly
  const ezRTTI* pRtti = ezGetStaticRTTI<ezGameObject>();
  pRtti->DispatchMessage(this, msg);

  // in all cases other than ToObjectOnly, route message to all components now
  if (routing >= ezObjectMsgRouting::ToComponents)
  {
    bool bSentToAny = false;

    for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
    {
      ezComponent* pComponent = m_Components[i];
      if (pComponent->SendMessage(msg))
        bSentToAny = true;
    }

    if (!bSentToAny)
    {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      if (msg.m_bPleaseTellMeInDetailWhenAndWhyThisMessageDoesNotArrive)
      {
        if (!m_Components.IsEmpty())
        {
          ezLog::Warning("ezGameObject::SendMessage: None of the target object's components had a handler for messages of type %u.", msg.GetId());
        }

        if (routing == ezObjectMsgRouting::ToComponents)
        {
          ezLog::Warning("Message of type  %u was sent 'ToComponents' only. Object with %u components did not handle this. No further message routing will happen.", msg.GetId(), m_Components.GetCount());
        }
      }
#endif
    }
  }

  // route message to parent or children
  if (routing == ezObjectMsgRouting::ToAllParents)
  {
    if (ezGameObject* pParent = GetParent())
    {
      pParent->SendMessage(msg, routing);
    }
  }
  else if (routing == ezObjectMsgRouting::ToChildren)
  {
    for (auto it = GetChildren(); it.IsValid(); ++it)
    {
      it->SendMessage(msg, routing);
    }
  }
}

void ezGameObject::SendMessage(ezMessage& msg, ezObjectMsgRouting::Enum routing) const
{
  if (routing == ezObjectMsgRouting::ToSubTree)
  {
    // walk up the sub tree and send to children form there to prevent double handling
    const ezGameObject* pCurrent = this;
    const ezGameObject* pParent = GetParent();
    while (pParent != nullptr)
    {
      pCurrent = pParent;
      pParent = pCurrent->GetParent();
    }

    pCurrent->SendMessage(msg, ezObjectMsgRouting::ToChildren);
    return;
  }

  const ezRTTI* pRtti = ezGetStaticRTTI<ezGameObject>();
  pRtti->DispatchMessage(this, msg);

  // route message to all components
  if (routing >= ezObjectMsgRouting::ToComponents)
  {
    for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
    {
      const ezComponent* pComponent = m_Components[i];
      pComponent->SendMessage(msg);
    }
  }

  // route message to parent or children
  if (routing == ezObjectMsgRouting::ToAllParents)
  {
    if (const ezGameObject* pParent = GetParent())
    {
      pParent->SendMessage(msg, routing);
    }
  }
  else if (routing == ezObjectMsgRouting::ToChildren)
  {
    for (auto it = GetChildren(); it.IsValid(); ++it)
    {
      it->SendMessage(msg, routing);
    }
  }
}

void ezGameObject::TransformationData::ConditionalUpdateGlobalTransform()
{
  if (m_pParentData != nullptr)
  {
    m_pParentData->ConditionalUpdateGlobalTransform();
    UpdateGlobalTransformWithParent();
  }
  else
  {
    UpdateGlobalTransform();
  }
}

void ezGameObject::TransformationData::ConditionalUpdateGlobalBounds()
{
  // Ensure that global transform is updated
  ConditionalUpdateGlobalTransform();

  UpdateGlobalBounds();
}


EZ_STATICLINK_FILE(Core, Core_World_Implementation_GameObject);

