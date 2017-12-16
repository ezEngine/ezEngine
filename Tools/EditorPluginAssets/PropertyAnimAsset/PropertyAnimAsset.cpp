#include <PCH.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectAccessor.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <GameEngine/Resources/PropertyAnimResource.h>
#include <ToolsFoundation/Object/DocumentObjectVisitor.h>
#include <Core/World/GameObject.h>
#include <ToolsFoundation/Command/TreeCommands.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPropertyAnimationTrack, 1, ezRTTIDefaultAllocator<ezPropertyAnimationTrack>);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ObjectPath", m_sObjectSearchSequence),
    EZ_MEMBER_PROPERTY("ComponentType", m_sComponentType),
    EZ_MEMBER_PROPERTY("Property", m_sPropertyPath),
    EZ_ENUM_MEMBER_PROPERTY("Target", ezPropertyAnimTarget, m_Target),
    EZ_MEMBER_PROPERTY("FloatCurve", m_FloatCurve),
    EZ_MEMBER_PROPERTY("Gradient", m_ColorGradient),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPropertyAnimationTrackGroup, 1, ezRTTIDefaultAllocator<ezPropertyAnimationTrackGroup>);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("FPS", m_uiFramesPerSecond)->AddAttributes(new ezDefaultValueAttribute(60)),
    EZ_MEMBER_PROPERTY("Duration", m_uiCurveDuration)->AddAttributes(new ezDefaultValueAttribute(480)),
    EZ_ARRAY_MEMBER_PROPERTY("Tracks", m_Tracks)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPropertyAnimAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPropertyAnimationTrackGroup::~ezPropertyAnimationTrackGroup()
{
  for (ezPropertyAnimationTrack* pTrack : m_Tracks)
  {
    EZ_DEFAULT_DELETE(pTrack);
  }
}

ezPropertyAnimAssetDocument::ezPropertyAnimAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezPropertyAnimationTrackGroup, ezGameObjectContextDocument>(EZ_DEFAULT_NEW(ezPropertyAnimObjectManager), szDocumentPath, true, true)
{
  m_GameObjectContextEvents.AddEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::GameObjectContextEventHandler, this));
  m_pAccessor = EZ_DEFAULT_NEW(ezPropertyAnimObjectAccessor, this, GetCommandHistory());
}

ezPropertyAnimAssetDocument::~ezPropertyAnimAssetDocument()
{
  m_GameObjectContextEvents.RemoveEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::GameObjectContextEventHandler, this));

  GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::TreeStructureEventHandler, this));
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::TreePropertyEventHandler, this));
}

ezObjectAccessorBase* ezPropertyAnimAssetDocument::GetObjectAccessor() const
{
  return m_pAccessor.Borrow();
}

void ezPropertyAnimAssetDocument::SetAnimationDurationTicks(ezUInt64 uiNumTicks)
{
  const ezPropertyAnimationTrackGroup* pProp = GetProperties();

  if (pProp->m_uiCurveDuration == uiNumTicks)
    return;

  {
    ezCommandHistory* history = GetCommandHistory();
    history->StartTransaction("Set Animation Duration");

    ezSetObjectPropertyCommand cmdSet;
    cmdSet.m_Object = GetPropertyObject()->GetGuid();
    cmdSet.m_sProperty = "Duration";
    cmdSet.m_NewValue = uiNumTicks;
    history->AddCommand(cmdSet);

    history->FinishTransaction();
  }

  {
    ezPropertyAnimAssetDocumentEvent e;
    e.m_pDocument = this;
    e.m_Type = ezPropertyAnimAssetDocumentEvent::Type::AnimationLengthChanged;
    m_PropertyAnimEvents.Broadcast(e);
  }
}

ezUInt64 ezPropertyAnimAssetDocument::GetAnimationDurationTicks() const
{
  const ezPropertyAnimationTrackGroup* pProp = GetProperties();

  return pProp->m_uiCurveDuration;
}


ezTime ezPropertyAnimAssetDocument::GetAnimationDurationTime() const
{
  const ezInt64 ticks = GetAnimationDurationTicks();

  return ezTime::Seconds(ticks / 4800.0);
}

void ezPropertyAnimAssetDocument::AdjustDuration()
{
  ezUInt64 uiDuration = 480;

  const ezPropertyAnimationTrackGroup* pProp = GetProperties();

  for (ezUInt32 i = 0; i < pProp->m_Tracks.GetCount(); ++i)
  {
    const ezPropertyAnimationTrack* pTrack = pProp->m_Tracks[i];

    for (const auto& cp : pTrack->m_FloatCurve.m_ControlPoints)
    {
      uiDuration = ezMath::Max(uiDuration, (ezUInt64)cp.m_iTick);
    }

    for (const auto& cp : pTrack->m_ColorGradient.m_ColorCPs)
    {
      uiDuration = ezMath::Max<ezInt64>(uiDuration, cp.m_iTick);
    }

    for (const auto& cp : pTrack->m_ColorGradient.m_AlphaCPs)
    {
      uiDuration = ezMath::Max<ezInt64>(uiDuration, cp.m_iTick);
    }

    for (const auto& cp : pTrack->m_ColorGradient.m_IntensityCPs)
    {
      uiDuration = ezMath::Max<ezInt64>(uiDuration, cp.m_iTick);
    }
  }

  SetAnimationDurationTicks(uiDuration);
}

bool ezPropertyAnimAssetDocument::SetScrubberPosition(ezUInt64 uiTick)
{
  if (!m_bPlayAnimation)
  {
    const ezUInt32 uiTicksPerFrame = 4800 / GetProperties()->m_uiFramesPerSecond;
    uiTick = (ezUInt64)ezMath::Round((double)uiTick, (double)uiTicksPerFrame);
  }
  uiTick = ezMath::Clamp<ezUInt64>(uiTick, 0, GetAnimationDurationTicks());

  if (m_uiScrubberTickPos == uiTick)
    return false;

  m_uiScrubberTickPos = uiTick;
  ApplyAnimation();

  ezPropertyAnimAssetDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = ezPropertyAnimAssetDocumentEvent::Type::ScrubberPositionChanged;
  m_PropertyAnimEvents.Broadcast(e);

  return true;
}

ezStatus ezPropertyAnimAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  const ezPropertyAnimationTrackGroup* pProp = GetProperties();

  ezResourceHandleWriteContext HandleContext;
  HandleContext.BeginWritingToStream(&stream);

  ezPropertyAnimResourceDescriptor desc;
  desc.m_AnimationDuration = GetAnimationDurationTime();

  for (ezUInt32 i = 0; i < pProp->m_Tracks.GetCount(); ++i)
  {
    const ezPropertyAnimationTrack* pTrack = pProp->m_Tracks[i];

    if (pTrack->m_Target == ezPropertyAnimTarget::Color)
    {
      auto& anim = desc.m_ColorAnimations.ExpandAndGetRef();
      anim.m_sObjectSearchSequence = pTrack->m_sObjectSearchSequence;
      anim.m_sComponentType = pTrack->m_sComponentType;
      anim.m_sPropertyPath = pTrack->m_sPropertyPath;
      anim.m_Target = pTrack->m_Target;
      pTrack->m_ColorGradient.FillGradientData(anim.m_Gradient);
      anim.m_Gradient.SortControlPoints();
    }
    else
    {
      auto& anim = desc.m_FloatAnimations.ExpandAndGetRef();
      anim.m_sObjectSearchSequence = pTrack->m_sObjectSearchSequence;
      anim.m_sComponentType = pTrack->m_sComponentType;
      anim.m_sPropertyPath = pTrack->m_sPropertyPath;
      anim.m_Target = pTrack->m_Target;
      pTrack->m_FloatCurve.ConvertToRuntimeData(anim.m_Curve);
      anim.m_Curve.SortControlPoints();
      anim.m_Curve.ApplyTangentModes();
      anim.m_Curve.ClampTangents();
    }
  }

  // sort animation tracks by object path for better cache reuse at runtime
  {
    desc.m_FloatAnimations.Sort([](const ezFloatPropertyAnimEntry& lhs, const ezFloatPropertyAnimEntry& rhs) -> bool
    {
      const ezInt32 res = lhs.m_sObjectSearchSequence.Compare(rhs.m_sObjectSearchSequence);
      if (res < 0)
        return true;
      if (res > 0)
        return false;

      return lhs.m_sComponentType < rhs.m_sComponentType;
    });

    desc.m_ColorAnimations.Sort([](const ezColorPropertyAnimEntry& lhs, const ezColorPropertyAnimEntry& rhs) -> bool
    {
      const ezInt32 res = lhs.m_sObjectSearchSequence.Compare(rhs.m_sObjectSearchSequence);
      if (res < 0)
        return true;
      if (res > 0)
        return false;

      return lhs.m_sComponentType < rhs.m_sComponentType;
    });
  }

  desc.Save(stream);

  HandleContext.EndWritingToStream(&stream);

  return ezStatus(EZ_SUCCESS);
}


void ezPropertyAnimAssetDocument::InitializeAfterLoading()
{
  // Filter needs to be set before base class init as that one sends the doc.
  // (Local mirror ignores temporaries, i.e. only mirrors the asset itself)
  m_ObjectMirror.SetFilterFunction([this](const ezDocumentObject* pObject, const char* szProperty) -> bool
  {
    return !static_cast<ezPropertyAnimObjectManager*>(GetObjectManager())->IsTemporary(pObject, szProperty);
  });
  // (Remote IPC mirror only sends temporaries, i.e. the context)
  m_Mirror.SetFilterFunction([this](const ezDocumentObject* pObject, const char* szProperty) -> bool
  {
    return static_cast<ezPropertyAnimObjectManager*>(GetObjectManager())->IsTemporary(pObject, szProperty);
  });
  SUPER::InitializeAfterLoading();
  // Important to do these after base class init as we want our subscriptions to happen after the mirror of the base class.
  GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::TreeStructureEventHandler, this));
  GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::TreePropertyEventHandler, this));
  // Subscribe here as otherwise base init will fire a context changed event when we are not set up yet.
  //RebuildMapping();
}

void ezPropertyAnimAssetDocument::GameObjectContextEventHandler(const ezGameObjectContextEvent& e)
{
  switch (e.m_Type)
  {
  case ezGameObjectContextEvent::Type::ContextAboutToBeChanged:
    static_cast<ezPropertyAnimObjectManager*>(GetObjectManager())->SetAllowStructureChangeOnTemporaries(true);
    break;
  case ezGameObjectContextEvent::Type::ContextChanged:
    static_cast<ezPropertyAnimObjectManager*>(GetObjectManager())->SetAllowStructureChangeOnTemporaries(false);
    RebuildMapping();
    break;
  }
}

void ezPropertyAnimAssetDocument::TreeStructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  auto pManager = static_cast<ezPropertyAnimObjectManager*>(GetObjectManager());
  if (e.m_pPreviousParent && pManager->IsTemporary(e.m_pPreviousParent, e.m_sParentProperty))
    return;
  if (e.m_pNewParent && pManager->IsTemporary(e.m_pNewParent, e.m_sParentProperty))
    return;

  if (e.m_pObject->GetType() == ezGetStaticRTTI<ezPropertyAnimationTrack>())
  {
    switch (e.m_EventType)
    {
    case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case ezDocumentObjectStructureEvent::Type::AfterObjectMoved:
      AddTrack(e.m_pObject->GetGuid());
      return;
    case ezDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    case ezDocumentObjectStructureEvent::Type::BeforeObjectMoved:
      RemoveTrack(e.m_pObject->GetGuid());
      return;
    }
  }
  else
  {
    ApplyAnimation();
  }
}

void ezPropertyAnimAssetDocument::TreePropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  auto pManager = static_cast<ezPropertyAnimObjectManager*>(GetObjectManager());
  if (pManager->IsTemporary(e.m_pObject))
    return;

  if (e.m_pObject->GetType() == ezGetStaticRTTI<ezPropertyAnimationTrack>())
  {
    if (e.m_EventType == ezDocumentObjectPropertyEvent::Type::PropertySet)
    {
      RemoveTrack(e.m_pObject->GetGuid());
      AddTrack(e.m_pObject->GetGuid());
      return;
    }
  }
  else
  {
    ApplyAnimation();
  }
}

void ezPropertyAnimAssetDocument::RebuildMapping()
{
  while (!m_TrackTable.IsEmpty())
  {
    RemoveTrack(m_TrackTable.GetIterator().Key());
  }
  EZ_ASSERT_DEBUG(m_PropertyTable.IsEmpty() && m_TrackTable.IsEmpty(), "All tracks should be removed.");

  const ezAbstractProperty* pTracksProp = ezGetStaticRTTI<ezPropertyAnimationTrackGroup>()->FindPropertyByName("Tracks");
  EZ_ASSERT_DEBUG(pTracksProp, "Name of property ezPropertyAnimationTrackGroup::m_Tracks has changed.");
  ezHybridArray<ezVariant, 16> values;
  m_pAccessor->GetValues(GetPropertyObject(), pTracksProp, values);
  for (const ezVariant& value : values)
  {
    AddTrack(value.Get<ezUuid>());
  }
}

void ezPropertyAnimAssetDocument::RemoveTrack(const ezUuid& track)
{
  auto& keys = *m_TrackTable.GetValue(track);
  for (const PropertyKey& key : keys)
  {
    PropertyValue& value = *m_PropertyTable.GetValue(key);
    value.m_Tracks.RemoveSwap(track);
    ApplyAnimation(key, value);
    if (value.m_Tracks.IsEmpty())
      m_PropertyTable.Remove(key);
  }
  m_TrackTable.Remove(track);
}

void ezPropertyAnimAssetDocument::AddTrack(const ezUuid& track)
{
  EZ_ASSERT_DEV(!m_TrackTable.Contains(track), "Track already exists.");
  auto& keys = m_TrackTable[track];
  const ezDocumentObject* pContext = GetContextObject();
  if (!pContext)
    return;

  auto pTrack = GetTrack(track);
  FindTrackKeys(pTrack->m_sObjectSearchSequence.GetData(), pTrack->m_sComponentType.GetData(), pTrack->m_sPropertyPath.GetData(), keys);

  for (const PropertyKey& key : keys)
  {
    if (!m_PropertyTable.Contains(key))
    {
      PropertyValue value;
      EZ_VERIFY(m_pAccessor->GetValue(GetObjectManager()->GetObject(key.m_Object), key.m_pProperty,
        value.m_InitialValue, key.m_Index).Succeeded(), "Computed key invalid, does not resolve to a value.");
      m_PropertyTable.Insert(key, value);
    }

    PropertyValue& value = *m_PropertyTable.GetValue(key);
    value.m_Tracks.PushBack(track);
    ApplyAnimation(key, value);
  }
}

void ezPropertyAnimAssetDocument::FindTrackKeys(const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath, ezHybridArray<PropertyKey, 1>& keys) const
{
  keys.Clear();
  const ezDocumentObject* pContext = GetContextObject();
  ezDocumentObjectVisitor visitor(GetObjectManager(), "Children", "TempObjects");
  ezHybridArray<const ezDocumentObject*, 8> input;
  input.PushBack(pContext);
  ezHybridArray<const ezDocumentObject*, 8> output;

  // Find objects that match the search path
  ezStringBuilder sObjectSearchSequence = szObjectSearchSequence;
  ezHybridArray<ezStringView, 4> names;
  sObjectSearchSequence.Split(false, names, "/");
  for (const ezStringView& sName : names)
  {
    for (const ezDocumentObject* pObj : input)
    {
      visitor.Visit(pContext, false, [&output, &sName](const ezDocumentObject* pObject) -> bool
      {
        const auto& sObjectName = pObject->GetTypeAccessor().GetValue("Name").Get<ezString>();
        if (sObjectName == sName)
        {
          output.PushBack(pObject);
          return false;
        }
        return true;
      });
    }
    input.Clear();
    input.Swap(output);
  }

  // Test found objects for component
  for (const ezDocumentObject* pObject : input)
  {
    // Could also be the root object in which case we found nothing.
    if (pObject->GetType() == ezGetStaticRTTI<ezGameObject>())
    {
      if (ezStringUtils::IsNullOrEmpty(szComponentType))
      {
        // We are animating the game object directly
        output.PushBack(pObject);
      }
      else
      {
        const ezInt32 iComponents = pObject->GetTypeAccessor().GetCount("Components");
        for (ezInt32 i = 0; i < iComponents; i++)
        {
          ezVariant value = pObject->GetTypeAccessor().GetValue("Components", i);
          auto pChild = GetObjectManager()->GetObject(value.Get<ezUuid>());
          if (ezStringUtils::IsEqual(szComponentType, pChild->GetType()->GetTypeName()))
          {
            output.PushBack(pChild);
            continue; //#TODO: break on found component?
          }
        }
      }
    }
  }
  input.Clear();
  input.Swap(output);

  // Test found objects / components for property
  for (const ezDocumentObject* pObject : input)
  {
    if (ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szPropertyPath))
    {
      //#TODO: Support property path (sub-objects and indices into arrays / maps)
      PropertyKey key;
      key.m_Object = pObject->GetGuid();
      key.m_pProperty = pProp;
      keys.PushBack(key);
    }
  }
}


void ezPropertyAnimAssetDocument::GenerateTrackInfo(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezStringBuilder& sObjectSearchSequence, ezStringBuilder& sComponentType, ezStringBuilder& sPropertyPath) const
{
  const ezRTTI* pObjType = ezGetStaticRTTI<ezGameObject>();
  const ezAbstractProperty* pName = pObjType->FindPropertyByName("Name");

  sPropertyPath = pProp->GetPropertyName();
  const ezDocumentObject* pObj = pObject;
  while (pObj != GetContextObject())
  {
    if (pObj->GetType() == ezGetStaticRTTI<ezGameObject>())
    {
      ezString sName = m_pAccessor->Get<ezString>(pObj, pName);
      if (!sName.IsEmpty())
      {
        if (!sObjectSearchSequence.IsEmpty())
          sObjectSearchSequence.Prepend("/");
        sObjectSearchSequence.Prepend(sName);
      }
    }
    else if (pObj->GetType()->IsDerivedFrom(ezGetStaticRTTI<ezComponent>()))
    {
      sComponentType = pObj->GetType()->GetTypeName();
    }
    else
    {
      if (!sPropertyPath.IsEmpty())
        sPropertyPath.Prepend("/");
      sPropertyPath.Prepend(pObj->GetParentPropertyType()->GetPropertyName());
    }
    pObj = pObj->GetParent();
  }
}

void ezPropertyAnimAssetDocument::ApplyAnimation()
{
  for (auto it = m_PropertyTable.GetIterator(); it.IsValid(); ++it)
  {
    ApplyAnimation(it.Key(), it.Value());
  }
}

void ezPropertyAnimAssetDocument::ApplyAnimation(const PropertyKey& key, const PropertyValue& value)
{
  ezVariant animValue = value.m_InitialValue;
  ezAngle euler[3];
  bool bIsRotation = false;

  for (const ezUuid& track : value.m_Tracks)
  {
    auto pTrack = GetTrack(track);
    const ezRTTI* pPropRtti = key.m_pProperty->GetSpecificType();

    //#TODO apply pTrack to animValue
    switch (pTrack->m_Target)
    {
    case ezPropertyAnimTarget::Number:
      {
        if (pPropRtti->GetVariantType() >= ezVariantType::Bool && pPropRtti->GetVariantType() <= ezVariantType::Double)
        {
          ezVariant value = pTrack->m_FloatCurve.Evaluate(m_uiScrubberTickPos);
          animValue = value.ConvertTo(animValue.GetType());
        }
      }
      break;

    case ezPropertyAnimTarget::VectorX:
    case ezPropertyAnimTarget::VectorY:
    case ezPropertyAnimTarget::VectorZ:
    case ezPropertyAnimTarget::VectorW:
      {
        if (pPropRtti->GetVariantType() >= ezVariantType::Vector2 && pPropRtti->GetVariantType() <= ezVariantType::Vector4U)
        {
          const double fValue = pTrack->m_FloatCurve.Evaluate(m_uiScrubberTickPos);

          ezReflectionUtils::SetComponent(animValue, (ezUInt32)pTrack->m_Target - ezPropertyAnimTarget::VectorX, fValue);
        }
      }
      break;

    case ezPropertyAnimTarget::RotationX:
    case ezPropertyAnimTarget::RotationY:
    case ezPropertyAnimTarget::RotationZ:
    {
      if (pPropRtti->GetVariantType() == ezVariantType::Quaternion)
      {
        bIsRotation = true;
        const double fValue = pTrack->m_FloatCurve.Evaluate(m_uiScrubberTickPos);

        euler[(ezUInt32)pTrack->m_Target - ezPropertyAnimTarget::RotationX] = ezAngle::Degree(fValue);
      }
    }
      break;

    case ezPropertyAnimTarget::Color:
      {
        if (pPropRtti->GetVariantType() == ezVariantType::Color || pPropRtti->GetVariantType() == ezVariantType::ColorGamma)
        {
          ezVariant value = pTrack->m_ColorGradient.Evaluate(m_uiScrubberTickPos);
          animValue = value.ConvertTo(animValue.GetType());
        }
      }
      break;
    }
  }

  if (bIsRotation)
  {
    ezQuat qRotation;
    qRotation.SetFromEulerAngles(euler[0], euler[1], euler[2]);
    animValue = qRotation;
  }

  ezDocumentObject* pObj = GetObjectManager()->GetObject(key.m_Object);
  ezVariant oldValue;
  EZ_VERIFY(m_pAccessor->GetValue(pObj, key.m_pProperty, oldValue, key.m_Index).Succeeded(), "Retrieving old value failed.");
  if (oldValue != animValue)
    GetObjectManager()->SetValue(pObj, key.m_pProperty->GetPropertyName(), animValue, key.m_Index);
}

void ezPropertyAnimAssetDocument::SetPlayAnimation(bool play)
{
  if (m_bPlayAnimation == play)
    return;

  if (m_uiScrubberTickPos >= GetAnimationDurationTicks())
    m_uiScrubberTickPos = 0;

  m_bPlayAnimation = play;
  if (!m_bPlayAnimation)
  {
    // During playback we do not round to frames, so we need to round it again on stop.
    SetScrubberPosition(GetScrubberPosition());
  }
  m_LastFrameTime = ezTime::Now();

  ezPropertyAnimAssetDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = ezPropertyAnimAssetDocumentEvent::Type::PlaybackChanged;
  m_PropertyAnimEvents.Broadcast(e);
}

void ezPropertyAnimAssetDocument::SetRepeatAnimation(bool repeat)
{
  if (m_bRepeatAnimation == repeat)
    return;

  m_bRepeatAnimation = repeat;

  ezPropertyAnimAssetDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = ezPropertyAnimAssetDocumentEvent::Type::PlaybackChanged;
  m_PropertyAnimEvents.Broadcast(e);
}

void ezPropertyAnimAssetDocument::ExecuteAnimationPlaybackStep()
{
  const ezTime currentTime = ezTime::Now();
  const ezTime tDiff = (currentTime - m_LastFrameTime) * GetSimulationSpeed();
  const ezUInt64 uiTicks = (ezUInt64)(tDiff.GetSeconds() * 4800.0);
  // Accumulate further if we render too fast and round ticks to zero.
  if (uiTicks == 0)
    return;

  m_LastFrameTime = currentTime;
  const ezUInt64 uiNewPos = GetScrubberPosition() + uiTicks;
  SetScrubberPosition(uiNewPos);

  if (uiNewPos > GetAnimationDurationTicks())
  {
    SetPlayAnimation(false);

    if (m_bRepeatAnimation)
      SetPlayAnimation(true);
  }
}

const ezPropertyAnimationTrack* ezPropertyAnimAssetDocument::GetTrack(const ezUuid& track) const
{
  return const_cast<ezPropertyAnimAssetDocument*>(this)->GetTrack(track);
}

ezPropertyAnimationTrack* ezPropertyAnimAssetDocument::GetTrack(const ezUuid& track)
{
  auto obj = m_Context.GetObjectByGUID(track);
  EZ_ASSERT_DEBUG(obj.m_pType == ezGetStaticRTTI<ezPropertyAnimationTrack>(), "Track guid does not resolve to a track, "
    "either the track is not yet created in the mirror or already destroyed. Make sure callbacks are executed in the right order.");
  auto pTrack = static_cast<ezPropertyAnimationTrack*>(obj.m_pObject);
  return pTrack;
}


ezStatus ezPropertyAnimAssetDocument::CanAnimate(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezPropertyAnimTarget::Enum target) const
{
  if (!pObject)
    return ezStatus("Object is null.");
  if (!pProp)
    return ezStatus("Property is null.");
  if (index.IsValid())
    return ezStatus("Property indices not supported.");

  if (!GetContextObject())
    return ezStatus("No context set.");

  {
    const ezDocumentObject* pNode = pObject;
    while (pNode && pNode != GetContextObject())
    {
      pNode = pNode->GetParent();
    }
    if (!pNode)
    {
      return ezStatus("Object not below context sub-tree.");
    }
  }
  PropertyKey key;
  key.m_Object = pObject->GetGuid();
  key.m_pProperty = pProp;
  key.m_Index = index;

  ezStringBuilder sObjectSearchSequence;
  ezStringBuilder sComponentType;
  ezStringBuilder sPropertyPath;
  GenerateTrackInfo(pObject, pProp, index, sObjectSearchSequence, sComponentType, sPropertyPath);

  const ezAbstractProperty* pName = ezGetStaticRTTI<ezGameObject>()->FindPropertyByName("Name");
  const ezDocumentObject* pNode = pObject;
  while (pNode != GetContextObject() && pNode->GetType() != ezGetStaticRTTI<ezGameObject>())
  {
    pNode = pNode->GetParent();
  }
  ezString sName = m_pAccessor->Get<ezString>(pNode, pName);

  if (sName.IsEmpty() && pNode != GetContextObject())
  {
    return ezStatus("Empty node name only allowed on context root object animations.");
  }

  ezHybridArray<PropertyKey, 1> keys;
  FindTrackKeys(sObjectSearchSequence.GetData(), sComponentType.GetData(), sPropertyPath.GetData(), keys);
  if (!keys.Contains(key))
    return ezStatus("No node name set or property is not reachable.");

  return ezStatus(EZ_SUCCESS);
}

ezUuid ezPropertyAnimAssetDocument::FindTrack(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezPropertyAnimTarget::Enum target) const
{
  PropertyKey key;
  key.m_Object = pObject->GetGuid();
  key.m_pProperty = pProp;
  key.m_Index = index;
  if (const PropertyValue* value = m_PropertyTable.GetValue(key))
  {
    for (const ezUuid& track : value->m_Tracks)
    {
      auto pTrack = GetTrack(track);
      if (pTrack->m_Target == target)
        return track;
    }
  }
  return ezUuid();
}

static ezColorGammaUB g_CurveColors[10][3] =
{
  { ezColorGammaUB(255, 102, 0), ezColorGammaUB(76, 255, 0), ezColorGammaUB(0, 255, 255) },
  { ezColorGammaUB(239, 35, 0), ezColorGammaUB(127, 255, 0), ezColorGammaUB(0, 0 ,255) },
  { ezColorGammaUB(205, 92, 92), ezColorGammaUB(120, 158, 39), ezColorGammaUB(81, 120, 188) },
  { ezColorGammaUB(255, 105, 180), ezColorGammaUB(0, 250, 154), ezColorGammaUB(0, 191, 255) },
  { ezColorGammaUB(220, 20, 60), ezColorGammaUB(0, 255, 127), ezColorGammaUB(30, 144, 255) },
  { ezColorGammaUB(240, 128, 128), ezColorGammaUB(60, 179, 113), ezColorGammaUB(135, 206, 250) },
  { ezColorGammaUB(178, 34, 34), ezColorGammaUB(46, 139, 87), ezColorGammaUB(65, 105, 225) },
  { ezColorGammaUB(211, 122, 122), ezColorGammaUB(144, 238, 144), ezColorGammaUB(135, 206, 235) },
  { ezColorGammaUB(219, 112, 147), ezColorGammaUB(0, 128, 0), ezColorGammaUB(70, 130, 180) },
  { ezColorGammaUB(255, 182, 193), ezColorGammaUB(102, 205, 170), ezColorGammaUB(100, 149, 237) },
};

static ezColorGammaUB g_FloatColors[10] =
{
  ezColorGammaUB(138, 43, 226),
  ezColorGammaUB(139, 0, 139),
  ezColorGammaUB(153, 50, 204),
  ezColorGammaUB(148, 0, 211),
  ezColorGammaUB(218, 112, 214),
  ezColorGammaUB(221, 160, 221),
  ezColorGammaUB(128, 0, 128),
  ezColorGammaUB(102, 51, 153),
  ezColorGammaUB(106, 90, 205),
  ezColorGammaUB(238, 130, 238),
};

ezUuid ezPropertyAnimAssetDocument::CreateTrack(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezPropertyAnimTarget::Enum target)
{

  ezStringBuilder sObjectSearchSequence;
  ezStringBuilder sComponentType;
  ezStringBuilder sPropertyPath;
  GenerateTrackInfo(pObject, pProp, index, sObjectSearchSequence, sComponentType, sPropertyPath);


  ezObjectCommandAccessor accessor(GetCommandHistory());
  const ezRTTI* pTrackType = ezGetStaticRTTI<ezPropertyAnimationTrack>();
  ezUuid newTrack;
  EZ_VERIFY(accessor.AddObject(GetPropertyObject(), ezGetStaticRTTI<ezPropertyAnimationTrackGroup>()->FindPropertyByName("Tracks"),
    -1, pTrackType, newTrack).Succeeded(), "Adding track failed.");
  const ezDocumentObject* pTrackObj = accessor.GetObject(newTrack);
  ezVariant value = sObjectSearchSequence.GetData();
  EZ_VERIFY(accessor.SetValue(pTrackObj, pTrackType->FindPropertyByName("ObjectPath"), value).Succeeded(), "Adding track failed.");
  value = sComponentType.GetData();
  EZ_VERIFY(accessor.SetValue(pTrackObj, pTrackType->FindPropertyByName("ComponentType"), value).Succeeded(), "Adding track failed.");
  value = sPropertyPath.GetData();
  EZ_VERIFY(accessor.SetValue(pTrackObj, pTrackType->FindPropertyByName("Property"), value).Succeeded(), "Adding track failed.");
  value = (int)target;
  EZ_VERIFY(accessor.SetValue(pTrackObj, pTrackType->FindPropertyByName("Target"), value).Succeeded(), "Adding track failed.");

  {
    const ezAbstractProperty* pFloatCurveProp = pTrackType->FindPropertyByName("FloatCurve");
    ezUuid floatCurveGuid = accessor.Get<ezUuid>(pTrackObj, pFloatCurveProp);
    const ezDocumentObject* pFloatCurveObject = GetObjectManager()->GetObject(floatCurveGuid);

    const ezAbstractProperty* pColorProp = ezGetStaticRTTI<ezSingleCurveData>()->FindPropertyByName("Color");

    ezColorGammaUB color = ezColor::White;

    const ezUInt32 uiNameHash = ezHashing::CRC32Hash(sObjectSearchSequence.GetData(), sObjectSearchSequence.GetElementCount());
    const ezUInt32 uiColorIdx = uiNameHash % EZ_ARRAY_SIZE(g_CurveColors);

    switch (target)
    {
    case ezPropertyAnimTarget::Number:
      color = g_FloatColors[uiColorIdx];
      break;
    case ezPropertyAnimTarget::VectorX:
    case ezPropertyAnimTarget::RotationX:
      color = g_CurveColors[uiColorIdx][0];
      break;
    case ezPropertyAnimTarget::VectorY:
    case ezPropertyAnimTarget::RotationY:
      color = g_CurveColors[uiColorIdx][1];
      break;
    case ezPropertyAnimTarget::VectorZ:
    case ezPropertyAnimTarget::RotationZ:
      color = g_CurveColors[uiColorIdx][2];
      break;
    case ezPropertyAnimTarget::VectorW:
      color = ezColor::Beige;
      break;
    }

    accessor.SetValue(pFloatCurveObject, pColorProp, color);
  }

  return newTrack;
}

ezUuid ezPropertyAnimAssetDocument::FindCurveCp(const ezUuid& trackGuid, ezInt64 tickX)
{
  auto pTrack = GetTrack(trackGuid);
  ezInt32 iIndex = -1;
  for (ezUInt32 i = 0; i < pTrack->m_FloatCurve.m_ControlPoints.GetCount(); i++)
  {
    if (pTrack->m_FloatCurve.m_ControlPoints[i].m_iTick == tickX)
    {
      iIndex = (ezInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return ezUuid();

  const ezAbstractProperty* pCurveProp = ezGetStaticRTTI<ezPropertyAnimationTrack>()->FindPropertyByName("FloatCurve");
  const ezDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  ezUuid curveGuid = m_pAccessor->Get<ezUuid>(trackObject, pCurveProp);
  const ezAbstractProperty* pControlPointsProp = ezGetStaticRTTI<ezSingleCurveData>()->FindPropertyByName("ControlPoints");
  const ezDocumentObject* curveObject = GetObjectManager()->GetObject(curveGuid);
  ezUuid cpGuid = m_pAccessor->Get<ezUuid>(curveObject, pControlPointsProp, iIndex);
  return cpGuid;
}

ezUuid ezPropertyAnimAssetDocument::InsertCurveCpAt(const ezUuid& track, ezInt64 tickX, double newPosY)
{
  ezObjectCommandAccessor accessor(GetCommandHistory());
  ezObjectAccessorBase& acc = accessor;
  acc.StartTransaction("Insert Control Point");

  const ezDocumentObject* trackObject = GetObjectManager()->GetObject(track);
  const ezVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  ezUuid newObjectGuid;
  EZ_VERIFY(acc.AddObject(accessor.GetObject(curveGuid.Get<ezUuid>()), "ControlPoints", -1,
    ezGetStaticRTTI<ezCurveControlPointData>(), newObjectGuid).Succeeded(), "");
  auto curveCPObj = accessor.GetObject(newObjectGuid);
  EZ_VERIFY(acc.SetValue(curveCPObj, "Tick", tickX).Succeeded(), "");
  EZ_VERIFY(acc.SetValue(curveCPObj, "Value", newPosY).Succeeded(), "");
  EZ_VERIFY(acc.SetValue(curveCPObj, "LeftTangent", ezVec2(-0.1f, 0.0f)).Succeeded(), "");
  EZ_VERIFY(acc.SetValue(curveCPObj, "RightTangent", ezVec2(+0.1f, 0.0f)).Succeeded(), "");

  acc.FinishTransaction();

  return newObjectGuid;
}

ezUuid ezPropertyAnimAssetDocument::FindGradientColorCp(const ezUuid& trackGuid, ezInt64 tickX)
{
  auto pTrack = GetTrack(trackGuid);
  ezInt32 iIndex = -1;
  for (ezUInt32 i = 0; i < pTrack->m_ColorGradient.m_ColorCPs.GetCount(); i++)
  {
    if (pTrack->m_ColorGradient.m_ColorCPs[i].m_iTick == tickX)
    {
      iIndex = (ezInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return ezUuid();

  const ezAbstractProperty* pCurveProp = ezGetStaticRTTI<ezPropertyAnimationTrack>()->FindPropertyByName("Gradient");
  const ezDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  ezUuid curveGuid = m_pAccessor->Get<ezUuid>(trackObject, pCurveProp);
  const ezAbstractProperty* pControlPointsProp = ezGetStaticRTTI<ezColorGradientAssetData>()->FindPropertyByName("ColorCPs");
  const ezDocumentObject* curveObject = GetObjectManager()->GetObject(curveGuid);
  ezUuid cpGuid = m_pAccessor->Get<ezUuid>(curveObject, pControlPointsProp, iIndex);
  return cpGuid;
}

ezUuid ezPropertyAnimAssetDocument::InsertGradientColorCpAt(const ezUuid& trackGuid, ezInt64 tickX, const ezColorGammaUB& color)
{
  ezObjectCommandAccessor accessor(GetCommandHistory());
  ezObjectAccessorBase& acc = accessor;

  const ezDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  const ezUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<ezUuid>();
  const ezDocumentObject* gradientObject = GetObjectManager()->GetObject(gradientGuid);

  acc.StartTransaction("Add Color Control Point");
  ezUuid newObjectGuid;
  EZ_VERIFY(acc.AddObject(gradientObject, "ColorCPs", -1, ezGetStaticRTTI<ezColorControlPoint>(), newObjectGuid).Succeeded(), "");
  const ezDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  EZ_VERIFY(acc.SetValue(cpObject, "Tick", tickX).Succeeded(), "");
  EZ_VERIFY(acc.SetValue(cpObject, "Red", color.r).Succeeded(), "");
  EZ_VERIFY(acc.SetValue(cpObject, "Green", color.g).Succeeded(), "");
  EZ_VERIFY(acc.SetValue(cpObject, "Blue", color.b).Succeeded(), "");
  acc.FinishTransaction();
  return newObjectGuid;
}

ezUuid ezPropertyAnimAssetDocument::FindGradientAlphaCp(const ezUuid& trackGuid, ezInt64 tickX)
{
  auto pTrack = GetTrack(trackGuid);
  ezInt32 iIndex = -1;
  for (ezUInt32 i = 0; i < pTrack->m_ColorGradient.m_AlphaCPs.GetCount(); i++)
  {
    if (pTrack->m_ColorGradient.m_AlphaCPs[i].m_iTick == tickX)
    {
      iIndex = (ezInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return ezUuid();

  const ezAbstractProperty* pCurveProp = ezGetStaticRTTI<ezPropertyAnimationTrack>()->FindPropertyByName("Gradient");
  const ezDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  ezUuid curveGuid = m_pAccessor->Get<ezUuid>(trackObject, pCurveProp);
  const ezAbstractProperty* pControlPointsProp = ezGetStaticRTTI<ezColorGradientAssetData>()->FindPropertyByName("AlphaCPs");
  const ezDocumentObject* curveObject = GetObjectManager()->GetObject(curveGuid);
  ezUuid cpGuid = m_pAccessor->Get<ezUuid>(curveObject, pControlPointsProp, iIndex);
  return cpGuid;
}

ezUuid ezPropertyAnimAssetDocument::InsertGradientAlphaCpAt(const ezUuid& trackGuid, ezInt64 tickX, ezUInt8 alpha)
{
  ezObjectCommandAccessor accessor(GetCommandHistory());
  ezObjectAccessorBase& acc = accessor;

  const ezDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  const ezUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<ezUuid>();
  const ezDocumentObject* gradientObject = GetObjectManager()->GetObject(gradientGuid);

  acc.StartTransaction("Add Alpha Control Point");
  ezUuid newObjectGuid;
  EZ_VERIFY(acc.AddObject(gradientObject, "AlphaCPs", -1, ezGetStaticRTTI<ezAlphaControlPoint>(), newObjectGuid).Succeeded(), "");
  const ezDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  EZ_VERIFY(acc.SetValue(cpObject, "Tick", tickX).Succeeded(), "");
  EZ_VERIFY(acc.SetValue(cpObject, "Alpha", alpha).Succeeded(), "");
  acc.FinishTransaction();
  return newObjectGuid;
}

ezUuid ezPropertyAnimAssetDocument::FindGradientIntensityCp(const ezUuid& trackGuid, ezInt64 tickX)
{
  auto pTrack = GetTrack(trackGuid);
  ezInt32 iIndex = -1;
  for (ezUInt32 i = 0; i < pTrack->m_ColorGradient.m_IntensityCPs.GetCount(); i++)
  {
    if (pTrack->m_ColorGradient.m_IntensityCPs[i].m_iTick == tickX)
    {
      iIndex = (ezInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return ezUuid();

  const ezAbstractProperty* pCurveProp = ezGetStaticRTTI<ezPropertyAnimationTrack>()->FindPropertyByName("Gradient");
  const ezDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  ezUuid curveGuid = m_pAccessor->Get<ezUuid>(trackObject, pCurveProp);
  const ezAbstractProperty* pControlPointsProp = ezGetStaticRTTI<ezColorGradientAssetData>()->FindPropertyByName("IntensityCPs");
  const ezDocumentObject* curveObject = GetObjectManager()->GetObject(curveGuid);
  ezUuid cpGuid = m_pAccessor->Get<ezUuid>(curveObject, pControlPointsProp, iIndex);
  return cpGuid;
}

ezUuid ezPropertyAnimAssetDocument::InsertGradientIntensityCpAt(const ezUuid& trackGuid, ezInt64 tickX, float intensity)
{
  ezObjectCommandAccessor accessor(GetCommandHistory());
  ezObjectAccessorBase& acc = accessor;

  const ezDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  const ezUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<ezUuid>();
  const ezDocumentObject* gradientObject = GetObjectManager()->GetObject(gradientGuid);

  acc.StartTransaction("Add Intensity Control Point");
  ezUuid newObjectGuid;
  EZ_VERIFY(acc.AddObject(gradientObject, "IntensityCPs", -1, ezGetStaticRTTI<ezIntensityControlPoint>(), newObjectGuid).Succeeded(), "");
  const ezDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  EZ_VERIFY(acc.SetValue(cpObject, "Tick", tickX).Succeeded(), "");
  EZ_VERIFY(acc.SetValue(cpObject, "Intensity", intensity).Succeeded(), "");
  acc.FinishTransaction();
  return newObjectGuid;
}

