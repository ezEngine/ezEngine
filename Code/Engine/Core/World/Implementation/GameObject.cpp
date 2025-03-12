#include <Core/CorePCH.h>

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
    ezVariantArray value(ezStaticsAllocatorWrapper::GetAllocator());
    value.PushBack("CastShadow");
    return value;
  }
} // namespace

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTransformPreservation, 1)
  EZ_ENUM_CONSTANTS(ezTransformPreservation::PreserveLocal, ezTransformPreservation::PreserveGlobal)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezGameObject, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetNameInternal, SetNameInternal),
    EZ_ACCESSOR_PROPERTY("Active", GetActiveFlag, SetActiveFlag)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("GlobalKey", GetGlobalKeyInternal, SetGlobalKeyInternal),
    EZ_ENUM_ACCESSOR_PROPERTY("Mode", ezObjectMode, Reflection_GetMode, Reflection_SetMode),
    EZ_ACCESSOR_PROPERTY("LocalPosition", GetLocalPosition, SetLocalPosition)->AddAttributes(new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("LocalRotation", GetLocalRotation, SetLocalRotation),
    EZ_ACCESSOR_PROPERTY("LocalScaling", GetLocalScaling, SetLocalScaling)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f, 1.0f, 1.0f))),
    EZ_ACCESSOR_PROPERTY("LocalUniformScaling", GetLocalUniformScaling, SetLocalUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_SET_ACCESSOR_PROPERTY("Tags", GetTags, Reflection_SetTag, Reflection_RemoveTag)->AddAttributes(new ezTagSetWidgetAttribute("Default"), new ezDefaultValueAttribute(GetDefaultTags())),
    EZ_SET_ACCESSOR_PROPERTY("Children", Reflection_GetChildren, Reflection_AddChild, Reflection_DetachChild)->AddFlags(ezPropertyFlags::PointerOwner | ezPropertyFlags::Hidden),
    EZ_SET_ACCESSOR_PROPERTY("Components", Reflection_GetComponents, Reflection_AddComponent, Reflection_RemoveComponent)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(IsActive),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetCreatedByPrefab),
    EZ_SCRIPT_FUNCTION_PROPERTY(WasCreatedByPrefab),

    EZ_SCRIPT_FUNCTION_PROPERTY(HasName, In, "Name"),
    EZ_SCRIPT_FUNCTION_PROPERTY(HasTag, In, "TagName"),

    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_GetParent),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_FindChildByName, In, "Name", In, "Recursive")->AddFlags(ezPropertyFlags::PureFunction),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_FindChildByPath, In, "Path")->AddFlags(ezPropertyFlags::PureFunction),

    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_SetGlobalPosition, In, "Position"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetGlobalPosition),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_SetGlobalRotation, In, "Rotation"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetGlobalRotation),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_SetGlobalScaling, In, "Scaling"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetGlobalScaling),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_SetGlobalTransform, In, "Transform"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetGlobalTransform),

    EZ_SCRIPT_FUNCTION_PROPERTY(GetGlobalDirForwards),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetGlobalDirRight),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetGlobalDirUp),

#if EZ_ENABLED(EZ_GAMEOBJECT_VELOCITY)
    EZ_SCRIPT_FUNCTION_PROPERTY(GetLinearVelocity),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetAngularVelocity),
#endif

    EZ_SCRIPT_FUNCTION_PROPERTY(SetTeamID, In, "Id"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetTeamID),    
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

void ezGameObject::Reflection_SetTag(const char* szTagName)
{
  if (ezStringUtils::IsNullOrEmpty(szTagName))
    return;

  const ezTag& tag = ezTagRegistry::GetGlobalRegistry().RegisterTag(szTagName);
  SetTag(tag);
}

void ezGameObject::Reflection_RemoveTag(const char* szTagName)
{
  if (ezStringUtils::IsNullOrEmpty(szTagName))
    return;

  if (const ezTag* pTag = ezTagRegistry::GetGlobalRegistry().GetTagByName(ezTempHashedString(szTagName)))
  {
    RemoveTag(*pTag);
  }
}

void ezGameObject::Reflection_AddChild(ezGameObject* pChild)
{
  if (IsDynamic())
  {
    pChild->MakeDynamic();
  }

  AddChild(pChild->GetHandle(), ezTransformPreservation::PreserveLocal);

  // Check whether the child object was only dynamic because of its old parent
  // If that's the case make it static now.
  pChild->ConditionalMakeStatic();
}

void ezGameObject::Reflection_DetachChild(ezGameObject* pChild)
{
  DetachChild(pChild->GetHandle(), ezTransformPreservation::PreserveLocal);

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

ezGameObject* ezGameObject::Reflection_GetParent() const
{
  return GetWorld()->GetObjectUnchecked(m_uiParentIndex);
}

void ezGameObject::Reflection_SetGlobalPosition(const ezVec3& vPosition)
{
  SetGlobalPosition(vPosition);
}

void ezGameObject::Reflection_SetGlobalRotation(const ezQuat& qRotation)
{
  SetGlobalRotation(qRotation);
}

void ezGameObject::Reflection_SetGlobalScaling(const ezVec3& vScaling)
{
  SetGlobalScaling(vScaling);
}

void ezGameObject::Reflection_SetGlobalTransform(const ezTransform& transform)
{
  SetGlobalTransform(transform);
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

void ezGameObject::UpdateGlobalTransformAndBoundsRecursive()
{
  if (IsStatic() && GetWorld()->ReportErrorWhenStaticObjectMoves())
  {
    ezLog::Error("Static object '{0}' was moved during runtime.", GetName());
  }

  ezSimdTransform oldGlobalTransform = GetGlobalTransformSimd();

  m_pTransformationData->UpdateGlobalTransformNonRecursive(GetWorld()->GetUpdateCounter());

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

void ezGameObject::UpdateLastGlobalTransform()
{
  m_pTransformationData->UpdateLastGlobalTransform(GetWorld()->GetUpdateCounter());
}

void ezGameObject::ConstChildIterator::Next()
{
  m_pObject = m_pWorld->GetObjectUnchecked(m_pObject->m_uiNextSiblingIndex);
}

ezGameObject::~ezGameObject()
{
  // Since we are using the small array base class for components we have to cleanup ourself with the correct allocator.
  m_Components.Clear();
  m_Components.Compact(GetWorld()->GetAllocator());
}

void ezGameObject::operator=(const ezGameObject& other)
{
  EZ_ASSERT_DEV(m_InternalId.m_WorldIndex == other.m_InternalId.m_WorldIndex, "Cannot copy between worlds.");

  m_InternalId = other.m_InternalId;
  m_Flags = other.m_Flags;
  m_sName = other.m_sName;

  m_uiParentIndex = other.m_uiParentIndex;
  m_uiFirstChildIndex = other.m_uiFirstChildIndex;
  m_uiLastChildIndex = other.m_uiLastChildIndex;

  m_uiNextSiblingIndex = other.m_uiNextSiblingIndex;
  m_uiPrevSiblingIndex = other.m_uiPrevSiblingIndex;
  m_uiChildCount = other.m_uiChildCount;

  m_uiTeamID = other.m_uiTeamID;

  m_uiHierarchyLevel = other.m_uiHierarchyLevel;
  m_pTransformationData = other.m_pTransformationData;
  m_pTransformationData->m_pObject = this;

  if (!m_pTransformationData->m_hSpatialData.IsInvalidated())
  {
    ezSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem();
    pSpatialSystem->UpdateSpatialDataObject(m_pTransformationData->m_hSpatialData, this);
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

ezStringView ezGameObject::GetGlobalKey() const
{
  return GetWorld()->GetObjectGlobalKey(this);
}

const char* ezGameObject::GetGlobalKeyInternal() const
{
  return GetWorld()->GetObjectGlobalKey(this).GetStartPointer(); // we know that it's zero terminated
}

void ezGameObject::SetParent(const ezGameObjectHandle& hParent, ezTransformPreservation::Enum preserve)
{
  ezWorld* pWorld = GetWorld();

  ezGameObject* pParent = nullptr;
  bool _ = pWorld->TryGetObject(hParent, pParent);
  EZ_IGNORE_UNUSED(_);
  pWorld->SetParent(this, pParent, preserve);
}

ezGameObject* ezGameObject::GetParent()
{
  return GetWorld()->GetObjectUnchecked(m_uiParentIndex);
}

const ezGameObject* ezGameObject::GetParent() const
{
  return GetWorld()->GetObjectUnchecked(m_uiParentIndex);
}

void ezGameObject::AddChild(const ezGameObjectHandle& hChild, ezTransformPreservation::Enum preserve)
{
  ezWorld* pWorld = GetWorld();

  ezGameObject* pChild = nullptr;
  if (pWorld->TryGetObject(hChild, pChild))
  {
    pWorld->SetParent(pChild, this, preserve);
  }
}

void ezGameObject::DetachChild(const ezGameObjectHandle& hChild, ezTransformPreservation::Enum preserve)
{
  ezWorld* pWorld = GetWorld();

  ezGameObject* pChild = nullptr;
  if (pWorld->TryGetObject(hChild, pChild))
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
  return ChildIterator(pWorld->GetObjectUnchecked(m_uiFirstChildIndex), pWorld);
}

ezGameObject::ConstChildIterator ezGameObject::GetChildren() const
{
  const ezWorld* pWorld = GetWorld();
  return ConstChildIterator(pWorld->GetObjectUnchecked(m_uiFirstChildIndex), pWorld);
}

ezGameObject* ezGameObject::FindChildByName(const ezTempHashedString& sName, bool bRecursive /*= true*/)
{
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName == sName)
    {
      return &(*it);
    }
  }

  if (bRecursive)
  {
    for (auto it = GetChildren(); it.IsValid(); ++it)
    {
      ezGameObject* pChild = it->FindChildByName(sName, bRecursive);

      if (pChild != nullptr)
        return pChild;
    }
  }

  return nullptr;
}

const ezGameObject* ezGameObject::FindChildByName(const ezTempHashedString& sName, bool bRecursive /*= true*/) const
{
  ezGameObject* pThis = const_cast<ezGameObject*>(this);
  return pThis->FindChildByName(sName, bRecursive);
}

ezGameObject* ezGameObject::FindChildByPath(ezStringView sPath)
{
  if (sPath.IsEmpty())
    return this;

  const char* szSep = sPath.FindSubString("/");
  ezUInt64 uiNameHash = 0;

  if (szSep == nullptr)
    uiNameHash = ezHashingUtils::StringHash(sPath);
  else
    uiNameHash = ezHashingUtils::StringHash(ezStringView(sPath.GetStartPointer(), szSep));

  ezGameObject* pNextChild = FindChildByName(ezTempHashedString(uiNameHash), false);

  if (szSep == nullptr || pNextChild == nullptr)
    return pNextChild;

  return pNextChild->FindChildByPath(ezStringView(szSep + 1, sPath.GetEndPointer()));
}

const ezGameObject* ezGameObject::FindChildByPath(ezStringView sPath) const
{
  ezGameObject* pThis = const_cast<ezGameObject*>(this);
  return pThis->FindChildByPath(sPath);
}

ezGameObject* ezGameObject::SearchForChildByNameSequence(ezStringView sObjectSequence, const ezRTTI* pExpectedComponent /*= nullptr*/)
{
  if (sObjectSequence.IsEmpty())
  {
    // in case we are searching for a specific component type, verify that it exists on this object
    if (pExpectedComponent != nullptr)
    {
      const ezComponent* pComp = nullptr;
      if (!TryGetComponentOfBaseType(pExpectedComponent, pComp))
        return nullptr;
    }

    return this;
  }

  const char* szSep = sObjectSequence.FindSubString("/");
  ezStringView sNextSequence;
  ezUInt64 uiNameHash = 0;

  if (szSep == nullptr)
  {
    uiNameHash = ezHashingUtils::StringHash(sObjectSequence);
  }
  else
  {
    uiNameHash = ezHashingUtils::StringHash(ezStringView(sObjectSequence.GetStartPointer(), szSep));
    sNextSequence = ezStringView(szSep + 1, sObjectSequence.GetEndPointer());
  }

  const ezTempHashedString name(uiNameHash);

  // first go through all direct children an see if any of them actually matches the current name
  // if so, continue the recursion from there and give them the remaining search path to continue
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName == name)
    {
      ezGameObject* res = it->SearchForChildByNameSequence(sNextSequence, pExpectedComponent);
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
      ezGameObject* res = it->SearchForChildByNameSequence(sObjectSequence, pExpectedComponent);
      if (res != nullptr)
        return res;
    }
  }

  return nullptr;
}

const ezGameObject* ezGameObject::SearchForChildByNameSequence(ezStringView sObjectSequence, const ezRTTI* pExpectedComponent /*= nullptr*/) const
{
  ezGameObject* pThis = const_cast<ezGameObject*>(this);
  return pThis->SearchForChildByNameSequence(sObjectSequence, pExpectedComponent);
}

void ezGameObject::SearchForChildrenByNameSequence(ezStringView sObjectSequence, const ezRTTI* pExpectedComponent, ezHybridArray<ezGameObject*, 8>& out_objects)
{
  if (sObjectSequence.IsEmpty())
  {
    // in case we are searching for a specific component type, verify that it exists on this object
    if (pExpectedComponent != nullptr)
    {
      ezComponent* pComp = nullptr;
      if (!TryGetComponentOfBaseType(pExpectedComponent, pComp))
        return;
    }

    out_objects.PushBack(this);
    return;
  }

  const char* szSep = sObjectSequence.FindSubString("/");
  ezStringView sNextSequence;
  ezUInt64 uiNameHash = 0;

  if (szSep == nullptr)
  {
    uiNameHash = ezHashingUtils::StringHash(sObjectSequence);
  }
  else
  {
    uiNameHash = ezHashingUtils::StringHash(ezStringView(sObjectSequence.GetStartPointer(), szSep));
    sNextSequence = ezStringView(szSep + 1, sObjectSequence.GetEndPointer());
  }

  const ezTempHashedString name(uiNameHash);

  // first go through all direct children an see if any of them actually matches the current name
  // if so, continue the recursion from there and give them the remaining search path to continue
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName == name)
    {
      it->SearchForChildrenByNameSequence(sNextSequence, pExpectedComponent, out_objects);
    }
  }

  // if no direct child fulfilled the requirements, just recurse with the full name sequence
  // however, we can skip any child that already fulfilled the next sequence name,
  // because that's definitely a lost cause
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName != name) // TODO: in this function it is actually debatable whether to skip these or not
    {
      it->SearchForChildrenByNameSequence(sObjectSequence, pExpectedComponent, out_objects);
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

#if EZ_ENABLED(EZ_GAMEOBJECT_VELOCITY)
void ezGameObject::SetLastGlobalTransform(const ezSimdTransform& transform)
{
  m_pTransformationData->m_lastGlobalTransform = transform;
  m_pTransformationData->m_uiLastGlobalTransformUpdateCounter = GetWorld()->GetUpdateCounter();
}

ezVec3 ezGameObject::GetLinearVelocity() const
{
  const ezSimdFloat invDeltaSeconds = GetWorld()->GetInvDeltaSeconds();
  const ezSimdVec4f linearVelocity = (m_pTransformationData->m_globalTransform.m_Position - m_pTransformationData->m_lastGlobalTransform.m_Position) * invDeltaSeconds;
  return ezSimdConversion::ToVec3(linearVelocity);
}

ezVec3 ezGameObject::GetAngularVelocity() const
{
  const ezSimdFloat invDeltaSeconds = GetWorld()->GetInvDeltaSeconds();
  const ezSimdQuat q = m_pTransformationData->m_globalTransform.m_Rotation * -m_pTransformationData->m_lastGlobalTransform.m_Rotation;
  ezSimdVec4f angularVelocity = ezSimdVec4f::MakeZero();

  ezSimdVec4f axis;
  ezSimdFloat angle;
  if (q.GetRotationAxisAndAngle(axis, angle).Succeeded())
  {
    angularVelocity = axis * (angle * invDeltaSeconds);
  }
  return ezSimdConversion::ToVec3(angularVelocity);
}
#endif

void ezGameObject::UpdateGlobalTransform()
{
  m_pTransformationData->UpdateGlobalTransformRecursive(GetWorld()->GetUpdateCounter());
}

void ezGameObject::UpdateLocalBounds()
{
  ezMsgUpdateLocalBounds msg;
  msg.m_ResultingLocalBounds = ezBoundingBoxSphere::MakeInvalid();

  SendMessage(msg);

  const bool bIsAlwaysVisible = m_pTransformationData->m_localBounds.m_BoxHalfExtents.w() != ezSimdFloat::MakeZero();
  bool bRecreateSpatialData = false;

  if (m_pTransformationData->m_hSpatialData.IsInvalidated() == false)
  {
    // force spatial data re-creation if categories have changed
    bRecreateSpatialData |= m_pTransformationData->m_uiSpatialDataCategoryBitmask != msg.m_uiSpatialDataCategoryBitmask;

    // force spatial data re-creation if always visible flag has changed
    bRecreateSpatialData |= bIsAlwaysVisible != msg.m_bAlwaysVisible;

    // delete old spatial data if bounds are now invalid
    bRecreateSpatialData |= msg.m_bAlwaysVisible == false && msg.m_ResultingLocalBounds.IsValid() == false;
  }

  m_pTransformationData->m_localBounds = ezSimdConversion::ToBBoxSphere(msg.m_ResultingLocalBounds);
  m_pTransformationData->m_localBounds.m_BoxHalfExtents.SetW(msg.m_bAlwaysVisible ? 1.0f : 0.0f);
  m_pTransformationData->m_uiSpatialDataCategoryBitmask = msg.m_uiSpatialDataCategoryBitmask;

  ezSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem();
  if (pSpatialSystem != nullptr && (bRecreateSpatialData || m_pTransformationData->m_hSpatialData.IsInvalidated()))
  {
    m_pTransformationData->RecreateSpatialData(*pSpatialSystem);
  }

  if (IsStatic())
  {
    m_pTransformationData->UpdateGlobalBounds(pSpatialSystem);
  }
}

void ezGameObject::UpdateGlobalTransformAndBounds()
{
  m_pTransformationData->UpdateGlobalTransformRecursive(GetWorld()->GetUpdateCounter());
  m_pTransformationData->UpdateGlobalBounds(GetWorld()->GetSpatialSystem());
}

void ezGameObject::UpdateGlobalBounds()
{
  m_pTransformationData->UpdateGlobalBounds(GetWorld()->GetSpatialSystem());
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


void ezGameObject::TryGetComponentsOfBaseType(const ezRTTI* pType, ezDynamicArray<ezComponent*>& out_components)
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

void ezGameObject::TryGetComponentsOfBaseType(const ezRTTI* pType, ezDynamicArray<const ezComponent*>& out_components) const
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

void ezGameObject::SetTeamID(ezUInt16 uiId)
{
  m_uiTeamID = uiId;

  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    it->SetTeamID(uiId);
  }
}

ezVisibilityState::Enum ezGameObject::GetVisibilityState(ezUInt32 uiNumFramesBeforeInvisible) const
{
  if (!m_pTransformationData->m_hSpatialData.IsInvalidated())
  {
    const ezSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem();
    return pSpatialSystem->GetVisibilityState(m_pTransformationData->m_hSpatialData, uiNumFramesBeforeInvisible);
  }

  return ezVisibilityState::Direct;
}

void ezGameObject::OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg)
{
  GetWorld()->DeleteObjectNow(GetHandle(), msg.m_bDeleteEmptyParents);
}

void ezGameObject::AddComponent(ezComponent* pComponent)
{
  EZ_ASSERT_DEV(pComponent->m_pOwner == nullptr, "Component must not be added twice.");
  EZ_ASSERT_DEV(IsDynamic() || !pComponent->IsDynamic(), "Cannot attach a dynamic component to a static object. Call MakeDynamic() first.");

  pComponent->m_pOwner = this;
  m_Components.PushBack(pComponent, GetWorld()->GetAllocator());
  m_Components.GetUserData<ComponentUserData>().m_uiVersion++;

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
  m_Components.GetUserData<ComponentUserData>().m_uiVersion++;

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
    // forward only to 'const' message handlers
    const ezComponent* pComponent = m_Components[i];
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
  // #if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  //  if (!bSentToAny && msg.GetDebugMessageRouting())
  //  {
  //    ezLog::Warning("ezGameObject::SendMessageRecursive: None of the target object's components had a handler for messages of type {0}.",
  //    msg.GetId());
  //  }
  // #endif
  // #
  return bSentToAny;
}

bool ezGameObject::SendMessageRecursiveInternal(ezMessage& msg, bool bWasPostedMsg) const
{
  bool bSentToAny = false;

  const ezRTTI* pRtti = ezGetStaticRTTI<ezGameObject>();
  bSentToAny |= pRtti->DispatchMessage(this, msg);

  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    // forward only to 'const' message handlers
    const ezComponent* pComponent = m_Components[i];
    bSentToAny |= pComponent->SendMessageInternal(msg, bWasPostedMsg);
  }

  for (auto childIt = GetChildren(); childIt.IsValid(); ++childIt)
  {
    bSentToAny |= childIt->SendMessageRecursiveInternal(msg, bWasPostedMsg);
  }

  // should only be evaluated at the top function call
  // #if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  //  if (!bSentToAny && msg.GetDebugMessageRouting())
  //  {
  //    ezLog::Warning("ezGameObject::SendMessageRecursive(const): None of the target object's components had a handler for messages of type
  //    {0}.", msg.GetId());
  //  }
  // #endif
  // #
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

bool ezGameObject::SendEventMessage(ezMessage& ref_msg, const ezComponent* pSenderComponent)
{
  if (auto pEventMsg = ezDynamicCast<ezEventMessage*>(&ref_msg))
  {
    pEventMsg->FillFromSenderComponent(pSenderComponent);
  }

  ezHybridArray<ezComponent*, 4> eventMsgHandlers;
  GetWorld()->FindEventMsgHandlers(ref_msg, this, eventMsgHandlers);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (ref_msg.GetDebugMessageRouting())
  {
    if (eventMsgHandlers.IsEmpty())
    {
      ezLog::Warning("ezGameObject::SendEventMessage: None of the target object's components had a handler for messages of type {0}.", ref_msg.GetId());
    }
  }
#endif

  bool bResult = false;
  for (auto pEventMsgHandler : eventMsgHandlers)
  {
    bResult |= pEventMsgHandler->SendMessage(ref_msg);
  }
  return bResult;
}

bool ezGameObject::SendEventMessage(ezMessage& ref_msg, const ezComponent* pSenderComponent) const
{
  if (auto pEventMsg = ezDynamicCast<ezEventMessage*>(&ref_msg))
  {
    pEventMsg->FillFromSenderComponent(pSenderComponent);
  }

  ezHybridArray<const ezComponent*, 4> eventMsgHandlers;
  GetWorld()->FindEventMsgHandlers(ref_msg, this, eventMsgHandlers);

  bool bResult = false;
  for (auto pEventMsgHandler : eventMsgHandlers)
  {
    bResult |= pEventMsgHandler->SendMessage(ref_msg);
  }
  return bResult;
}

void ezGameObject::PostEventMessage(ezMessage& ref_msg, const ezComponent* pSenderComponent, ezTime delay, ezObjectMsgQueueType::Enum queueType) const
{
  if (auto pEventMsg = ezDynamicCast<ezEventMessage*>(&ref_msg))
  {
    pEventMsg->FillFromSenderComponent(pSenderComponent);
  }

  ezHybridArray<const ezComponent*, 4> eventMsgHandlers;
  GetWorld()->FindEventMsgHandlers(ref_msg, this, eventMsgHandlers);

  for (auto pEventMsgHandler : eventMsgHandlers)
  {
    pEventMsgHandler->PostMessage(ref_msg, delay, queueType);
  }
}

void ezGameObject::SetTags(const ezTagSet& tags)
{
  if (ezSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
  {
    if (m_Tags != tags)
    {
      m_Tags = tags;
      m_pTransformationData->RecreateSpatialData(*pSpatialSystem);
    }
  }
  else
  {
    m_Tags = tags;
  }
}

void ezGameObject::SetTag(const ezTag& tag)
{
  if (ezSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
  {
    if (m_Tags.IsSet(tag) == false)
    {
      m_Tags.Set(tag);
      m_pTransformationData->RecreateSpatialData(*pSpatialSystem);
    }
  }
  else
  {
    m_Tags.Set(tag);
  }
}

void ezGameObject::RemoveTag(const ezTag& tag)
{
  if (ezSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
  {
    if (m_Tags.IsSet(tag))
    {
      m_Tags.Remove(tag);
      m_pTransformationData->RecreateSpatialData(*pSpatialSystem);
    }
  }
  else
  {
    m_Tags.Remove(tag);
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
    tLocal = ezSimdTransform::MakeLocalTransform(m_pParentData->m_globalTransform, m_globalTransform);
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

void ezGameObject::TransformationData::UpdateGlobalTransformNonRecursive(ezUInt32 uiUpdateCounter)
{
  if (m_pParentData != nullptr)
  {
    UpdateGlobalTransformWithParent(uiUpdateCounter);
  }
  else
  {
    UpdateGlobalTransformWithoutParent(uiUpdateCounter);
  }
}

void ezGameObject::TransformationData::UpdateGlobalTransformRecursive(ezUInt32 uiUpdateCounter)
{
  if (m_pParentData != nullptr)
  {
    m_pParentData->UpdateGlobalTransformRecursive(uiUpdateCounter);
    UpdateGlobalTransformWithParent(uiUpdateCounter);
  }
  else
  {
    UpdateGlobalTransformWithoutParent(uiUpdateCounter);
  }
}

void ezGameObject::TransformationData::UpdateGlobalBounds(ezSpatialSystem* pSpatialSystem)
{
  if (pSpatialSystem == nullptr)
  {
    UpdateGlobalBounds();
  }
  else
  {
    UpdateGlobalBoundsAndSpatialData(*pSpatialSystem);
  }
}

void ezGameObject::TransformationData::UpdateGlobalBoundsAndSpatialData(ezSpatialSystem& ref_spatialSystem)
{
  ezSimdBBoxSphere oldGlobalBounds = m_globalBounds;

  UpdateGlobalBounds();

  const bool bIsAlwaysVisible = m_localBounds.m_BoxHalfExtents.w() != ezSimdFloat::MakeZero();
  if (m_hSpatialData.IsInvalidated() == false && bIsAlwaysVisible == false && m_globalBounds != oldGlobalBounds)
  {
    ref_spatialSystem.UpdateSpatialDataBounds(m_hSpatialData, m_globalBounds);
  }
}

void ezGameObject::TransformationData::RecreateSpatialData(ezSpatialSystem& ref_spatialSystem)
{
  if (m_hSpatialData.IsInvalidated() == false)
  {
    ref_spatialSystem.DeleteSpatialData(m_hSpatialData);
    m_hSpatialData.Invalidate();
  }

  const bool bIsAlwaysVisible = m_localBounds.m_BoxHalfExtents.w() != ezSimdFloat::MakeZero();
  if (bIsAlwaysVisible)
  {
    m_hSpatialData = ref_spatialSystem.CreateSpatialDataAlwaysVisible(m_pObject, m_uiSpatialDataCategoryBitmask, m_pObject->m_Tags);
  }
  else if (m_localBounds.IsValid())
  {
    UpdateGlobalBounds();
    m_hSpatialData = ref_spatialSystem.CreateSpatialData(m_globalBounds, m_pObject, m_uiSpatialDataCategoryBitmask, m_pObject->m_Tags);
  }
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_GameObject);
