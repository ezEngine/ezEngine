#include <CorePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>

namespace
{
  static ezVariantArray GetDefaultTags()
  {
    ezVariantArray value(ezStaticAllocatorWrapper::GetAllocator());
    value.PushBack(ezStringView("CastShadow"));
    value.PushBack(ezStringView("AutoColMesh")); // TODO: keep this ?
    return value;
  }
} // namespace

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezGameObject, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
    EZ_ACCESSOR_PROPERTY("Active", GetActiveFlag, SetActiveFlag)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("GlobalKey", GetGlobalKey, SetGlobalKey),
    EZ_ENUM_ACCESSOR_PROPERTY("Mode", ezObjectMode, Reflection_GetMode, Reflection_SetMode),
    EZ_ACCESSOR_PROPERTY("LocalPosition", GetLocalPosition, SetLocalPosition)->AddAttributes(new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("LocalRotation", GetLocalRotation, SetLocalRotation),
    EZ_ACCESSOR_PROPERTY("LocalScaling", GetLocalScaling, SetLocalScaling)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f, 1.0f, 1.0f))),
    EZ_ACCESSOR_PROPERTY("LocalUniformScaling", GetLocalUniformScaling, SetLocalUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_SET_MEMBER_PROPERTY("Tags", m_Tags)->AddAttributes(new ezTagSetWidgetAttribute("Default"), new ezDefaultValueAttribute(GetDefaultTags())),
    EZ_SET_ACCESSOR_PROPERTY("Children", Reflection_GetChildren, Reflection_AddChild, Reflection_DetachChild)->AddFlags(ezPropertyFlags::PointerOwner | ezPropertyFlags::Hidden),
    EZ_SET_ACCESSOR_PROPERTY("Components", Reflection_GetComponents, Reflection_AddComponent, Reflection_RemoveComponent)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

void ezGameObject::Reflection_AddChild(ezGameObject* pChild)
{
  if (IsDynamic())
  {
    pChild->MakeDynamic();
  }

  AddChild(pChild->GetHandle(), TransformPreservation::PreserveLocal);

  // Check whether the child object was only dynamic because of its old parent
  // If that's the case make it static now.
  pChild->ConditionalMakeStatic();
}

void ezGameObject::Reflection_DetachChild(ezGameObject* pChild)
{
  DetachChild(pChild->GetHandle(), TransformPreservation::PreserveLocal);

  // The child object is now a top level object, check whether it should be static now.
  pChild->ConditionalMakeStatic();
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

void ezGameObject::Reflection_AddComponent(ezComponent* pComponent)
{
  if (pComponent == nullptr)
    return;

  if (pComponent->IsDynamic())
  {
    MakeDynamic();
  }

  AddComponent(pComponent);
}

void ezGameObject::Reflection_RemoveComponent(ezComponent* pComponent)
{
  if (pComponent == nullptr)
    return;

  /*Don't call RemoveComponent here, Component is automatically removed when deleted.*/

  if (pComponent->IsDynamic())
  {
    ConditionalMakeStatic(pComponent);
  }
}

ezHybridArray<ezComponent*, ezGameObject::NUM_INPLACE_COMPONENTS> ezGameObject::Reflection_GetComponents() const
{
  return ezHybridArray<ezComponent*, ezGameObject::NUM_INPLACE_COMPONENTS>(m_Components);
}

ezObjectMode::Enum ezGameObject::Reflection_GetMode() const
{
  return m_Flags.IsSet(ezObjectFlags::ForceDynamic) ? ezObjectMode::ForceDynamic : ezObjectMode::Automatic;
}

void ezGameObject::Reflection_SetMode(ezObjectMode::Enum mode)
{
  if (Reflection_GetMode() == mode)
  {
    return;
  }

  if (mode == ezObjectMode::ForceDynamic)
  {
    m_Flags.Add(ezObjectFlags::ForceDynamic);
    MakeDynamic();
  }
  else
  {
    m_Flags.Remove(ezObjectFlags::ForceDynamic);
    ConditionalMakeStatic();
  }
}

bool ezGameObject::DetermineDynamicMode(ezComponent* pComponentToIgnore /*= nullptr*/) const
{
  if (m_Flags.IsSet(ezObjectFlags::ForceDynamic))
  {
    return true;
  }

  const ezGameObject* pParent = GetParent();
  if (pParent != nullptr && pParent->IsDynamic())
  {
    return true;
  }

  for (auto pComponent : m_Components)
  {
    if (pComponent != pComponentToIgnore && pComponent->IsDynamic())
    {
      return true;
    }
  }

  return false;
}

void ezGameObject::ConditionalMakeStatic(ezComponent* pComponentToIgnore /*= nullptr*/)
{
  if (!DetermineDynamicMode(pComponentToIgnore))
  {
    MakeStaticInternal();

    for (auto it = GetChildren(); it.IsValid(); ++it)
    {
      it->ConditionalMakeStatic();
    }
  }
}

void ezGameObject::MakeStaticInternal()
{
  if (IsStatic())
  {
    return;
  }

  m_Flags.Remove(ezObjectFlags::Dynamic);

  GetWorld()->RecreateHierarchyData(this, true);
}

void ezGameObject::SetTeamID(ezUInt16 id)
{
  m_uiTeamID = id;

  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    it->SetTeamID(id);
  }
}

void ezGameObject::UpdateGlobalTransformAndBoundsRecursive()
{
  if (IsStatic() && GetWorld()->ReportErrorWhenStaticObjectMoves())
  {
    ezLog::Error("Static object '{0}' was moved during runtime.", GetName());
  }

  ezSimdTransform oldGlobalTransform = GetGlobalTransformSimd();

  if (m_pTransformationData->m_pParentData != nullptr)
  {
    m_pTransformationData->UpdateGlobalTransformWithParent();
  }
  else
  {
    m_pTransformationData->UpdateGlobalTransform();
  }

  if (ezSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
  {
    m_pTransformationData->UpdateGlobalBoundsAndSpatialData(*pSpatialSystem);
  }
  else
  {
    m_pTransformationData->UpdateGlobalBounds();
  }

  if (IsStatic() && m_Flags.IsSet(ezObjectFlags::StaticTransformChangesNotifications) && oldGlobalTransform != GetGlobalTransformSimd())
  {
    ezMsgTransformChanged msg;
    msg.m_OldGlobalTransform = ezSimdConversion::ToTransform(oldGlobalTransform);
    msg.m_NewGlobalTransform = GetGlobalTransform();

    SendMessage(msg);
  }

  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    it->UpdateGlobalTransformAndBoundsRecursive();
  }
}

void ezGameObject::ConstChildIterator::Next()
{
  m_pObject = m_pWorld->GetObjectUnchecked(m_pObject->m_NextSiblingIndex);
}

ezGameObject::~ezGameObject()
{
  m_Components.Clear();
  m_Components.Compact(GetWorld()->GetAllocator());
}

void ezGameObject::operator=(const ezGameObject& other)
{
  EZ_ASSERT_DEV(m_InternalId.m_WorldIndex == other.m_InternalId.m_WorldIndex, "Cannot copy between worlds.");

  m_InternalId = other.m_InternalId;
  m_Flags = other.m_Flags;
  m_sName = other.m_sName;

  m_ParentIndex = other.m_ParentIndex;
  m_FirstChildIndex = other.m_FirstChildIndex;
  m_LastChildIndex = other.m_LastChildIndex;

  m_NextSiblingIndex = other.m_NextSiblingIndex;
  m_PrevSiblingIndex = other.m_PrevSiblingIndex;
  m_ChildCount = other.m_ChildCount;

  m_uiTeamID = other.m_uiTeamID;

  m_uiHierarchyLevel = other.m_uiHierarchyLevel;
  m_pTransformationData = other.m_pTransformationData;
  m_pTransformationData->m_pObject = this;

  if (!m_pTransformationData->m_hSpatialData.IsInvalidated())
  {
    if (ezSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
    {
      pSpatialSystem->UpdateSpatialData(
        m_pTransformationData->m_hSpatialData, m_pTransformationData->m_globalBounds, this, m_pTransformationData->m_uiSpatialDataCategoryBitmask);
    }
  }

  m_Components.CopyFrom(other.m_Components, GetWorld()->GetAllocator());
  for (ezComponent* pComponent : m_Components)
  {
    EZ_ASSERT_DEV(pComponent->m_pOwner == &other, "");
    pComponent->m_pOwner = this;
  }

  m_Tags = other.m_Tags;
}

void ezGameObject::MakeDynamic()
{
  if (IsDynamic())
  {
    return;
  }

  m_Flags.Add(ezObjectFlags::Dynamic);

  GetWorld()->RecreateHierarchyData(this, false);

  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    it->MakeDynamic();
  }
}

void ezGameObject::MakeStatic()
{
  EZ_ASSERT_DEV(!DetermineDynamicMode(), "This object can't be static because it has a dynamic parent or dynamic component(s) attached.");

  MakeStaticInternal();
}

void ezGameObject::SetActiveFlag(bool bEnabled)
{
  if (m_Flags.IsSet(ezObjectFlags::ActiveFlag) == bEnabled)
    return;

  m_Flags.AddOrRemove(ezObjectFlags::ActiveFlag, bEnabled);

  UpdateActiveState(GetParent() == nullptr ? true : GetParent()->IsActive());
}

void ezGameObject::UpdateActiveState(bool bParentActive)
{
  const bool bSelfActive = bParentActive && m_Flags.IsSet(ezObjectFlags::ActiveFlag);

  if (bSelfActive != m_Flags.IsSet(ezObjectFlags::ActiveState))
  {
    m_Flags.AddOrRemove(ezObjectFlags::ActiveState, bSelfActive);

    for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
    {
      m_Components[i]->UpdateActiveState(bSelfActive);
    }

    // recursively update all children
    for (auto it = GetChildren(); it.IsValid(); ++it)
    {
      it->UpdateActiveState(bSelfActive);
    }
  }
}

void ezGameObject::SetGlobalKey(const ezHashedString& sName)
{
  GetWorld()->SetObjectGlobalKey(this, sName);
}

const char* ezGameObject::GetGlobalKey() const
{
  return GetWorld()->GetObjectGlobalKey(this);
}

void ezGameObject::SetParent(const ezGameObjectHandle& parent, ezGameObject::TransformPreservation preserve)
{
  ezWorld* pWorld = GetWorld();

  ezGameObject* pParent = nullptr;
  pWorld->TryGetObject(parent, pParent);
  pWorld->SetParent(this, pParent, preserve);
}

ezGameObject* ezGameObject::GetParent()
{
  return GetWorld()->GetObjectUnchecked(m_ParentIndex);
}

const ezGameObject* ezGameObject::GetParent() const
{
  return GetWorld()->GetObjectUnchecked(m_ParentIndex);
}

void ezGameObject::AddChild(const ezGameObjectHandle& child, ezGameObject::TransformPreservation preserve)
{
  ezWorld* pWorld = GetWorld();

  ezGameObject* pChild = nullptr;
  if (pWorld->TryGetObject(child, pChild))
  {
    pWorld->SetParent(pChild, this, preserve);
  }
}

void ezGameObject::DetachChild(const ezGameObjectHandle& child, ezGameObject::TransformPreservation preserve)
{
  ezWorld* pWorld = GetWorld();

  ezGameObject* pChild = nullptr;
  if (pWorld->TryGetObject(child, pChild))
  {
    if (pChild->GetParent() == this)
    {
      pWorld->SetParent(pChild, nullptr, preserve);
    }
  }
}

ezGameObject::ChildIterator ezGameObject::GetChildren()
{
  ezWorld* pWorld = GetWorld();
  return ChildIterator(pWorld->GetObjectUnchecked(m_FirstChildIndex), pWorld);
}

ezGameObject::ConstChildIterator ezGameObject::GetChildren() const
{
  const ezWorld* pWorld = GetWorld();
  return ConstChildIterator(pWorld->GetObjectUnchecked(m_FirstChildIndex), pWorld);
}

ezGameObject* ezGameObject::FindChildByName(const ezTempHashedString& name, bool bRecursive /*= true*/)
{
  /// \test Needs a unit test

  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName == name)
    {
      return &(*it);
    }
  }

  if (bRecursive)
  {
    for (auto it = GetChildren(); it.IsValid(); ++it)
    {
      ezGameObject* pChild = it->FindChildByName(name, bRecursive);

      if (pChild != nullptr)
        return pChild;
    }
  }

  return nullptr;
}

ezGameObject* ezGameObject::FindChildByPath(const char* path)
{
  /// \test Needs a unit test

  if (ezStringUtils::IsNullOrEmpty(path))
    return this;

  const char* szSep = ezStringUtils::FindSubString(path, "/");
  ezUInt32 uiNameHash = 0;

  if (szSep == nullptr)
    uiNameHash = ezHashingUtils::xxHash32String(path);
  else
    uiNameHash = ezHashingUtils::xxHash32(path, szSep - path);

  ezGameObject* pNextChild = FindChildByName(ezTempHashedString(uiNameHash), false);

  if (szSep == nullptr || pNextChild == nullptr)
    return pNextChild;

  return pNextChild->FindChildByPath(szSep + 1);
}


ezGameObject* ezGameObject::SearchForChildByNameSequence(const char* szObjectSequence, const ezRTTI* pExpectedComponent /*= nullptr*/)
{
  /// \test Needs a unit test

  if (ezStringUtils::IsNullOrEmpty(szObjectSequence))
  {
    // in case we are searching for a specific component type, verify that it exists on this object
    if (pExpectedComponent != nullptr)
    {
      ezComponent* pComp = nullptr;
      if (!TryGetComponentOfBaseType(pExpectedComponent, pComp))
        return nullptr;
    }

    return this;
  }

  const char* szSep = ezStringUtils::FindSubString(szObjectSequence, "/");
  const char* szNextSequence = nullptr;
  ezUInt32 uiNameHash = 0;

  if (szSep == nullptr)
  {
    const size_t len = (size_t)ezStringUtils::GetStringElementCount(szObjectSequence);
    uiNameHash = ezHashingUtils::xxHash32(szObjectSequence, len);
    szNextSequence = szObjectSequence + len;
  }
  else
  {
    uiNameHash = ezHashingUtils::xxHash32(szObjectSequence, szSep - szObjectSequence);
    szNextSequence = szSep + 1;
  }

  const ezTempHashedString name(uiNameHash);

  // first go through all direct children an see if any of them actually matches the current name
  // if so, continue the recursion from there and give them the remaining search path to continue
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName == name)
    {
      ezGameObject* res = it->SearchForChildByNameSequence(szNextSequence, pExpectedComponent);
      if (res != nullptr)
        return res;
    }
  }

  // if no direct child fulfilled the requirements, just recurse with the full name sequence
  // however, we can skip any child that already fulfilled the next sequence name,
  // because that's definitely a lost cause
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName != name)
    {
      ezGameObject* res = it->SearchForChildByNameSequence(szObjectSequence, pExpectedComponent);
      if (res != nullptr)
        return res;
    }
  }

  return nullptr;
}


void ezGameObject::SearchForChildrenByNameSequence(
  const char* szObjectSequence, const ezRTTI* pExpectedComponent, ezHybridArray<ezGameObject*, 8>& out_Objects)
{
  /// \test Needs a unit test

  if (ezStringUtils::IsNullOrEmpty(szObjectSequence))
  {
    // in case we are searching for a specific component type, verify that it exists on this object
    if (pExpectedComponent != nullptr)
    {
      ezComponent* pComp = nullptr;
      if (!TryGetComponentOfBaseType(pExpectedComponent, pComp))
        return;
    }

    out_Objects.PushBack(this);
    return;
  }

  const char* szSep = ezStringUtils::FindSubString(szObjectSequence, "/");
  const char* szNextSequence = nullptr;
  ezUInt32 uiNameHash = 0;

  if (szSep == nullptr)
  {
    const size_t len = (size_t)ezStringUtils::GetStringElementCount(szObjectSequence);
    uiNameHash = ezHashingUtils::xxHash32(szObjectSequence, len);
    szNextSequence = szObjectSequence + len;
  }
  else
  {
    uiNameHash = ezHashingUtils::xxHash32(szObjectSequence, szSep - szObjectSequence);
    szNextSequence = szSep + 1;
  }

  const ezTempHashedString name(uiNameHash);

  // first go through all direct children an see if any of them actually matches the current name
  // if so, continue the recursion from there and give them the remaining search path to continue
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName == name)
    {
      it->SearchForChildrenByNameSequence(szNextSequence, pExpectedComponent, out_Objects);
    }
  }

  // if no direct child fulfilled the requirements, just recurse with the full name sequence
  // however, we can skip any child that already fulfilled the next sequence name,
  // because that's definitely a lost cause
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName != name) // TODO: in this function it is actually debatable whether to skip these or not
    {
      it->SearchForChildrenByNameSequence(szObjectSequence, pExpectedComponent, out_Objects);
    }
  }
}

ezWorld* ezGameObject::GetWorld()
{
  return ezWorld::GetWorld(m_InternalId.m_WorldIndex);
}

const ezWorld* ezGameObject::GetWorld() const
{
  return ezWorld::GetWorld(m_InternalId.m_WorldIndex);
}

ezVec3 ezGameObject::GetGlobalDirForwards() const
{
  ezCoordinateSystem coordinateSystem;
  GetWorld()->GetCoordinateSystem(GetGlobalPosition(), coordinateSystem);

  return GetGlobalRotation() * coordinateSystem.m_vForwardDir;
}

ezVec3 ezGameObject::GetGlobalDirRight() const
{
  ezCoordinateSystem coordinateSystem;
  GetWorld()->GetCoordinateSystem(GetGlobalPosition(), coordinateSystem);

  return GetGlobalRotation() * coordinateSystem.m_vRightDir;
}

ezVec3 ezGameObject::GetGlobalDirUp() const
{
  ezCoordinateSystem coordinateSystem;
  GetWorld()->GetCoordinateSystem(GetGlobalPosition(), coordinateSystem);

  return GetGlobalRotation() * coordinateSystem.m_vUpDir;
}

void ezGameObject::UpdateLocalBounds()
{
  ezMsgUpdateLocalBounds msg;
  msg.m_ResultingLocalBounds.SetInvalid();

  SendMessage(msg);

  if (m_pTransformationData->m_uiSpatialDataCategoryBitmask != msg.m_uiSpatialDataCategoryBitmask)
  {
    m_pTransformationData->m_globalBounds.SetInvalid(); // force spatial data update
  }

  m_pTransformationData->m_localBounds = ezSimdConversion::ToBBoxSphere(msg.m_ResultingLocalBounds);
  m_pTransformationData->m_localBounds.m_BoxHalfExtents.SetW(msg.m_bAlwaysVisible ? 1.0f : 0.0f);
  m_pTransformationData->m_uiSpatialDataCategoryBitmask = msg.m_uiSpatialDataCategoryBitmask;

  if (IsStatic())
  {
    if (ezSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
    {
      m_pTransformationData->UpdateGlobalBoundsAndSpatialData(*pSpatialSystem);
    }
    else
    {
      m_pTransformationData->UpdateGlobalBounds();
    }
  }
}

void ezGameObject::UpdateGlobalTransformAndBounds()
{
  m_pTransformationData->ConditionalUpdateGlobalBounds(GetWorld()->GetSpatialSystem());
}

bool ezGameObject::TryGetComponentOfBaseType(const ezRTTI* pType, ezComponent*& out_pComponent)
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

bool ezGameObject::TryGetComponentOfBaseType(const ezRTTI* pType, const ezComponent*& out_pComponent) const
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


void ezGameObject::TryGetComponentsOfBaseType(const ezRTTI* pType, ezHybridArray<ezComponent*, 8>& out_components)
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

void ezGameObject::TryGetComponentsOfBaseType(const ezRTTI* pType, ezHybridArray<const ezComponent*, 8>& out_components) const
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

void ezGameObject::OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg)
{
  GetWorld()->DeleteObjectNow(GetHandle());
}

void ezGameObject::AddComponent(ezComponent* pComponent)
{
  EZ_ASSERT_DEV(pComponent->m_pOwner == nullptr, "Component must not be added twice.");
  EZ_ASSERT_DEV(IsDynamic() || !pComponent->IsDynamic(), "Cannot attach a dynamic component to a static object. Call MakeDynamic() first.");

  pComponent->m_pOwner = this;
  m_Components.PushBack(pComponent, GetWorld()->GetAllocator());

  pComponent->UpdateActiveState(IsActive());

  if (m_Flags.IsSet(ezObjectFlags::ComponentChangesNotifications))
  {
    ezMsgComponentsChanged msg;
    msg.m_Type = ezMsgComponentsChanged::Type::ComponentAdded;
    msg.m_hOwner = GetHandle();
    msg.m_hComponent = pComponent->GetHandle();

    SendNotificationMessage(msg);
  }
}

void ezGameObject::RemoveComponent(ezComponent* pComponent)
{
  ezUInt32 uiIndex = m_Components.IndexOf(pComponent);
  EZ_ASSERT_DEV(uiIndex != ezInvalidIndex, "Component not found");

  pComponent->m_pOwner = nullptr;
  m_Components.RemoveAtAndSwap(uiIndex);

  if (m_Flags.IsSet(ezObjectFlags::ComponentChangesNotifications))
  {
    ezMsgComponentsChanged msg;
    msg.m_Type = ezMsgComponentsChanged::Type::ComponentRemoved;
    msg.m_hOwner = GetHandle();
    msg.m_hComponent = pComponent->GetHandle();

    SendNotificationMessage(msg);
  }
}

bool ezGameObject::SendMessageInternal(ezMessage& msg, bool bWasPostedMsg)
{
  bool bSentToAny = false;

  const ezRTTI* pRtti = ezGetStaticRTTI<ezGameObject>();
  bSentToAny |= pRtti->DispatchMessage(this, msg);

  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    ezComponent* pComponent = m_Components[i];
    bSentToAny |= pComponent->SendMessageInternal(msg, bWasPostedMsg);
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (!bSentToAny && msg.GetDebugMessageRouting())
  {
    ezLog::Warning("ezGameObject::SendMessage: None of the target object's components had a handler for messages of type {0}.", msg.GetId());
  }
#endif

  return bSentToAny;
}

bool ezGameObject::SendMessageInternal(ezMessage& msg, bool bWasPostedMsg) const
{
  bool bSentToAny = false;

  const ezRTTI* pRtti = ezGetStaticRTTI<ezGameObject>();
  bSentToAny |= pRtti->DispatchMessage(this, msg);

  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    ezComponent* pComponent = m_Components[i];
    bSentToAny |= pComponent->SendMessageInternal(msg, bWasPostedMsg);
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (!bSentToAny && msg.GetDebugMessageRouting())
  {
    ezLog::Warning("ezGameObject::SendMessage (const): None of the target object's components had a handler for messages of type {0}.", msg.GetId());
  }
#endif

  return bSentToAny;
}

bool ezGameObject::SendMessageRecursiveInternal(ezMessage& msg, bool bWasPostedMsg)
{
  bool bSentToAny = false;

  const ezRTTI* pRtti = ezGetStaticRTTI<ezGameObject>();
  bSentToAny |= pRtti->DispatchMessage(this, msg);

  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    ezComponent* pComponent = m_Components[i];
    bSentToAny |= pComponent->SendMessageInternal(msg, bWasPostedMsg);
  }

  for (auto childIt = GetChildren(); childIt.IsValid(); ++childIt)
  {
    bSentToAny |= childIt->SendMessageRecursiveInternal(msg, bWasPostedMsg);
  }

  // should only be evaluated at the top function call
  //#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  //  if (!bSentToAny && msg.GetDebugMessageRouting())
  //  {
  //    ezLog::Warning("ezGameObject::SendMessageRecursive: None of the target object's components had a handler for messages of type {0}.",
  //    msg.GetId());
  //  }
  //#endif
  //#
  return bSentToAny;
}

bool ezGameObject::SendMessageRecursiveInternal(ezMessage& msg, bool bWasPostedMsg) const
{
  bool bSentToAny = false;

  const ezRTTI* pRtti = ezGetStaticRTTI<ezGameObject>();
  bSentToAny |= pRtti->DispatchMessage(this, msg);

  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    ezComponent* pComponent = m_Components[i];
    bSentToAny |= pComponent->SendMessageInternal(msg, bWasPostedMsg);
  }

  for (auto childIt = GetChildren(); childIt.IsValid(); ++childIt)
  {
    bSentToAny |= childIt->SendMessageRecursiveInternal(msg, bWasPostedMsg);
  }

  // should only be evaluated at the top function call
  //#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  //  if (!bSentToAny && msg.GetDebugMessageRouting())
  //  {
  //    ezLog::Warning("ezGameObject::SendMessageRecursive(const): None of the target object's components had a handler for messages of type
  //    {0}.", msg.GetId());
  //  }
  //#endif
  //#
  return bSentToAny;
}

void ezGameObject::PostMessage(const ezMessage& msg, ezTime delay, ezObjectMsgQueueType::Enum queueType) const
{
  GetWorld()->PostMessage(GetHandle(), msg, delay, queueType);
}

void ezGameObject::PostMessageRecursive(const ezMessage& msg, ezTime delay, ezObjectMsgQueueType::Enum queueType) const
{
  GetWorld()->PostMessageRecursive(GetHandle(), msg, delay, queueType);
}

void ezGameObject::SendEventMessage(ezEventMessage& msg, const ezComponent* pSenderComponent)
{
  if (ezComponent* pReceiver = const_cast<ezComponent*>(GetWorld()->FindEventMsgHandler(msg, this)))
  {
    if (pSenderComponent)
    {
      msg.m_hSenderComponent = pSenderComponent->GetHandle();
      msg.m_hSenderObject = pSenderComponent->GetOwner()->GetHandle();
    }

    pReceiver->SendMessage(msg);
  }
}

void ezGameObject::SendEventMessage(ezEventMessage& msg, const ezComponent* pSenderComponent) const
{
  if (const ezComponent* pReceiver = GetWorld()->FindEventMsgHandler(msg, const_cast<ezGameObject*>(this)))
  {
    if (pSenderComponent)
    {
      msg.m_hSenderComponent = pSenderComponent->GetHandle();
      msg.m_hSenderObject = pSenderComponent->GetOwner()->GetHandle();
    }

    pReceiver->SendMessage(msg);
  }
}

void ezGameObject::PostEventMessage(
  ezEventMessage& msg, const ezComponent* pSenderComponent, ezTime delay, ezObjectMsgQueueType::Enum queueType) const
{
  if (const ezComponent* pReceiver = GetWorld()->FindEventMsgHandler(msg, const_cast<ezGameObject*>(this)))
  {
    if (pSenderComponent)
    {
      msg.m_hSenderComponent = pSenderComponent->GetHandle();
      msg.m_hSenderObject = pSenderComponent->GetOwner()->GetHandle();
    }

    pReceiver->PostMessage(msg, delay, queueType);
  }
}

void ezGameObject::FixComponentPointer(ezComponent* pOldPtr, ezComponent* pNewPtr)
{
  ezUInt32 uiIndex = m_Components.IndexOf(pOldPtr);
  EZ_ASSERT_DEV(uiIndex != ezInvalidIndex, "Memory corruption?");
  m_Components[uiIndex] = pNewPtr;
}

void ezGameObject::SendNotificationMessage(ezMessage& msg)
{
  ezGameObject* pObject = this;
  while (pObject != nullptr)
  {
    pObject->SendMessage(msg);

    pObject = pObject->GetParent();
  }
}

//////////////////////////////////////////////////////////////////////////

void ezGameObject::TransformationData::UpdateLocalTransform()
{
  ezSimdTransform tLocal;

  if (m_pParentData != nullptr)
  {
    tLocal.SetLocalTransform(m_pParentData->m_globalTransform, m_globalTransform);
  }
  else
  {
    tLocal = m_globalTransform;
  }

  m_localPosition = tLocal.m_Position;
  m_localRotation = tLocal.m_Rotation;
  m_localScaling = tLocal.m_Scale;
  m_localScaling.SetW(1.0f);
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

void ezGameObject::TransformationData::ConditionalUpdateGlobalBounds(ezSpatialSystem* pSpatialSytem)
{
  // Ensure that global transform is updated
  ConditionalUpdateGlobalTransform();

  if (pSpatialSytem == nullptr)
  {
    UpdateGlobalBounds();
  }
  else
  {
    UpdateGlobalBoundsAndSpatialData(*pSpatialSytem);
  }
}

void ezGameObject::TransformationData::UpdateSpatialData(ezSpatialSystem& spatialSystem, bool bWasAlwaysVisible, bool bIsAlwaysVisible)
{
  if (bWasAlwaysVisible != bIsAlwaysVisible)
  {
    if (!m_hSpatialData.IsInvalidated())
    {
      spatialSystem.DeleteSpatialData(m_hSpatialData);
      m_hSpatialData.Invalidate();
    }
  }

  if (bIsAlwaysVisible)
  {
    if (m_hSpatialData.IsInvalidated())
    {
      m_hSpatialData = spatialSystem.CreateSpatialDataAlwaysVisible(m_pObject, m_uiSpatialDataCategoryBitmask);
    }
  }
  else
  {
    if (m_globalBounds.IsValid())
    {
      if (m_hSpatialData.IsInvalidated())
      {
        m_hSpatialData = spatialSystem.CreateSpatialData(m_globalBounds, m_pObject, m_uiSpatialDataCategoryBitmask);
      }
      else
      {
        spatialSystem.UpdateSpatialData(m_hSpatialData, m_globalBounds, m_pObject, m_uiSpatialDataCategoryBitmask);
      }
    }
    else
    {
      if (!m_hSpatialData.IsInvalidated())
      {
        spatialSystem.DeleteSpatialData(m_hSpatialData);
        m_hSpatialData.Invalidate();
      }
    }
  }
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_GameObject);
