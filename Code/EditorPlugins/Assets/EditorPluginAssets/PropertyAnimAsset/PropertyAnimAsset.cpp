#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectAccessor.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPropertyAnimationTrack, 1, ezRTTIDefaultAllocator<ezPropertyAnimationTrack>)
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
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPropertyAnimationTrackGroup, 1, ezRTTIDefaultAllocator<ezPropertyAnimationTrackGroup>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("FPS", m_uiFramesPerSecond)->AddAttributes(new ezDefaultValueAttribute(60)),
    EZ_MEMBER_PROPERTY("Duration", m_uiCurveDuration)->AddAttributes(new ezDefaultValueAttribute(480)),
    EZ_ARRAY_MEMBER_PROPERTY("Tracks", m_Tracks)->AddFlags(ezPropertyFlags::PointerOwner),
    EZ_MEMBER_PROPERTY("EventTrack", m_EventTrack),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPropertyAnimAssetDocument, 2, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPropertyAnimationTrackGroup::~ezPropertyAnimationTrackGroup()
{
  for (ezPropertyAnimationTrack* pTrack : m_Tracks)
  {
    EZ_DEFAULT_DELETE(pTrack);
  }
}

ezPropertyAnimAssetDocument::ezPropertyAnimAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezPropertyAnimationTrackGroup, ezGameObjectContextDocument>(
      EZ_DEFAULT_NEW(ezPropertyAnimObjectManager), sDocumentPath, ezAssetDocEngineConnection::FullObjectMirroring)
{
  m_GameObjectContextEvents.AddEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::GameObjectContextEventHandler, this));
  m_pObjectAccessor = EZ_DEFAULT_NEW(ezPropertyAnimObjectAccessor, this, GetCommandHistory());
}

ezPropertyAnimAssetDocument::~ezPropertyAnimAssetDocument()
{
  m_GameObjectContextEvents.RemoveEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::GameObjectContextEventHandler, this));

  GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::TreeStructureEventHandler, this));
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::TreePropertyEventHandler, this));
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
    history->AddCommand(cmdSet).AssertSuccess();

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

  return ezTime::MakeFromSeconds(ticks / 4800.0);
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
    uiTick = (ezUInt64)ezMath::RoundToMultiple((double)uiTick, (double)uiTicksPerFrame);
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

ezTransformStatus ezPropertyAnimAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
  const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const ezPropertyAnimationTrackGroup* pProp = GetProperties();

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

      return lhs.m_sComponentType < rhs.m_sComponentType; });

    desc.m_ColorAnimations.Sort([](const ezColorPropertyAnimEntry& lhs, const ezColorPropertyAnimEntry& rhs) -> bool
      {
      const ezInt32 res = lhs.m_sObjectSearchSequence.Compare(rhs.m_sObjectSearchSequence);
      if (res < 0)
        return true;
      if (res > 0)
        return false;

      return lhs.m_sComponentType < rhs.m_sComponentType; });
  }

  pProp->m_EventTrack.ConvertToRuntimeData(desc.m_EventTrack);

  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}


void ezPropertyAnimAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  m_pMirror = EZ_DEFAULT_NEW(ezIPCObjectMirrorEditor);
  // Filter needs to be set before base class init as that one sends the doc.
  // (Local mirror ignores temporaries, i.e. only mirrors the asset itself)
  m_ObjectMirror.SetFilterFunction([this](const ezDocumentObject* pObject, ezStringView sProperty) -> bool
    { return !static_cast<ezPropertyAnimObjectManager*>(GetObjectManager())->IsTemporary(pObject, sProperty); });
  // (Remote IPC mirror only sends temporaries, i.e. the context)
  m_pMirror->SetFilterFunction([this](const ezDocumentObject* pObject, ezStringView sProperty) -> bool
    { return static_cast<ezPropertyAnimObjectManager*>(GetObjectManager())->IsTemporary(pObject, sProperty); });
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
  // Important to do these after base class init as we want our subscriptions to happen after the mirror of the base class.
  GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::TreeStructureEventHandler, this));
  GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::TreePropertyEventHandler, this));
  // Subscribe here as otherwise base init will fire a context changed event when we are not set up yet.
  // RebuildMapping();
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

      default:
        break;
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
  m_pObjectAccessor->GetValues(GetPropertyObject(), pTracksProp, values).AssertSuccess();
  for (const ezVariant& value : values)
  {
    AddTrack(value.Get<ezUuid>());
  }
}

void ezPropertyAnimAssetDocument::RemoveTrack(const ezUuid& track)
{
  auto& keys = *m_TrackTable.GetValue(track);
  for (const ezPropertyReference& key : keys)
  {
    PropertyValue& value = *m_PropertyTable.GetValue(key);
    value.m_Tracks.RemoveAndSwap(track);
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
  FindTrackKeys(pTrack->m_sObjectSearchSequence.GetData(), pTrack->m_sComponentType.GetData(), pTrack->m_sPropertyPath.GetData(), keys).IgnoreResult();

  for (const ezPropertyReference& key : keys)
  {
    if (!m_PropertyTable.Contains(key))
    {
      PropertyValue value;
      EZ_VERIFY(m_pObjectAccessor->GetValue(GetObjectManager()->GetObject(key.m_Object), key.m_pProperty, value.m_InitialValue, key.m_Index).Succeeded(),
        "Computed key invalid, does not resolve to a value.");
      m_PropertyTable.Insert(key, value);
    }

    PropertyValue& value = *m_PropertyTable.GetValue(key);
    value.m_Tracks.PushBack(track);
    ApplyAnimation(key, value);
  }
}


ezStatus ezPropertyAnimAssetDocument::FindTrackKeys(const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath, ezHybridArray<ezPropertyReference, 1>& keys) const
{
  ezObjectPropertyPathContext context = {GetContextObject(), m_pObjectAccessor.Borrow(), "TempObjects"};

  keys.Clear();
  return ezObjectPropertyPath::ResolvePath(context, keys, szObjectSearchSequence, szComponentType, szPropertyPath);
}


void ezPropertyAnimAssetDocument::GenerateTrackInfo(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index,
  ezStringBuilder& sObjectSearchSequence, ezStringBuilder& sComponentType, ezStringBuilder& sPropertyPath) const
{
  ezObjectPropertyPathContext context = {GetContextObject(), m_pObjectAccessor.Borrow(), "TempObjects"};
  ezPropertyReference propertyRef = {pObject->GetGuid(), pProp, index};
  ezObjectPropertyPath::CreatePath(context, propertyRef, sObjectSearchSequence, sComponentType, sPropertyPath).AssertSuccess();
}

void ezPropertyAnimAssetDocument::ApplyAnimation()
{
  for (auto it = m_PropertyTable.GetIterator(); it.IsValid(); ++it)
  {
    ApplyAnimation(it.Key(), it.Value());
  }
}

void ezPropertyAnimAssetDocument::ApplyAnimation(const ezPropertyReference& key, const PropertyValue& value)
{
  ezVariant animValue = value.m_InitialValue;
  ezAngle euler[3];
  bool bIsRotation = false;

  for (const ezUuid& track : value.m_Tracks)
  {
    auto pTrack = GetTrack(track);
    const ezRTTI* pPropRtti = key.m_pProperty->GetSpecificType();

    // #TODO apply pTrack to animValue
    switch (pTrack->m_Target)
    {
      case ezPropertyAnimTarget::Number:
      {
        if (pPropRtti->GetVariantType() >= ezVariantType::Bool && pPropRtti->GetVariantType() <= ezVariantType::Double)
        {
          ezVariant value2 = pTrack->m_FloatCurve.Evaluate(m_uiScrubberTickPos);
          animValue = value2.ConvertTo(animValue.GetType());
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

          euler[(ezUInt32)pTrack->m_Target - ezPropertyAnimTarget::RotationX] = ezAngle::MakeFromDegree(fValue);
        }
      }
      break;

      case ezPropertyAnimTarget::Color:
      {
        if (pPropRtti->GetVariantType() == ezVariantType::Color || pPropRtti->GetVariantType() == ezVariantType::ColorGamma)
        {
          ezVariant value2 = pTrack->m_ColorGradient.Evaluate(m_uiScrubberTickPos);
          animValue = value2.ConvertTo(animValue.GetType());
        }
      }
      break;
    }
  }

  if (bIsRotation)
  {
    ezQuat qRotation;
    qRotation = ezQuat::MakeFromEulerAngles(euler[0], euler[1], euler[2]);
    animValue = qRotation;
  }

  ezDocumentObject* pObj = GetObjectManager()->GetObject(key.m_Object);
  ezVariant oldValue;
  EZ_VERIFY(m_pObjectAccessor->GetValue(pObj, key.m_pProperty, oldValue, key.m_Index).Succeeded(), "Retrieving old value failed.");

  if (oldValue != animValue)
    GetObjectManager()->SetValue(pObj, key.m_pProperty->GetPropertyName(), animValue, key.m_Index).AssertSuccess();

  // tell the gizmos and manipulators that they should update their transform
  // usually they listen to the command history and selection events, but in this case no commands are executed
  {
    ezGameObjectEvent e;
    e.m_Type = ezGameObjectEvent::Type::GizmoTransformMayBeInvalid;
    m_GameObjectEvents.Broadcast(e);
  }
}

void ezPropertyAnimAssetDocument::SetPlayAnimation(bool bPlay)
{
  if (m_bPlayAnimation == bPlay)
    return;

  if (m_uiScrubberTickPos >= GetAnimationDurationTicks())
    m_uiScrubberTickPos = 0;

  m_bPlayAnimation = bPlay;
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

void ezPropertyAnimAssetDocument::SetRepeatAnimation(bool bRepeat)
{
  if (m_bRepeatAnimation == bRepeat)
    return;

  m_bRepeatAnimation = bRepeat;

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
  EZ_ASSERT_DEBUG(obj.m_pType == ezGetStaticRTTI<ezPropertyAnimationTrack>(),
    "Track guid does not resolve to a track, "
    "either the track is not yet created in the mirror or already destroyed. Make sure callbacks are executed in the right order.");
  auto pTrack = static_cast<ezPropertyAnimationTrack*>(obj.m_pObject);
  return pTrack;
}


ezStatus ezPropertyAnimAssetDocument::CanAnimate(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezPropertyAnimTarget::Enum target) const
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
  ezPropertyReference key;
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
  ezString sName = m_pObjectAccessor->Get<ezString>(pNode, pName);

  if (sName.IsEmpty() && pNode != GetContextObject())
  {
    return ezStatus("Empty node name only allowed on context root object animations.");
  }

  ezHybridArray<ezPropertyReference, 1> keys;
  return FindTrackKeys(sObjectSearchSequence.GetData(), sComponentType.GetData(), sPropertyPath.GetData(), keys);
}

ezUuid ezPropertyAnimAssetDocument::FindTrack(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezPropertyAnimTarget::Enum target) const
{
  ezPropertyReference key;
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

static ezColorGammaUB g_CurveColors[10][3] = {
  {ezColorGammaUB(255, 102, 0), ezColorGammaUB(76, 255, 0), ezColorGammaUB(0, 255, 255)},
  {ezColorGammaUB(239, 35, 0), ezColorGammaUB(127, 255, 0), ezColorGammaUB(0, 0, 255)},
  {ezColorGammaUB(205, 92, 92), ezColorGammaUB(120, 158, 39), ezColorGammaUB(81, 120, 188)},
  {ezColorGammaUB(255, 105, 180), ezColorGammaUB(0, 250, 154), ezColorGammaUB(0, 191, 255)},
  {ezColorGammaUB(220, 20, 60), ezColorGammaUB(0, 255, 127), ezColorGammaUB(30, 144, 255)},
  {ezColorGammaUB(240, 128, 128), ezColorGammaUB(60, 179, 113), ezColorGammaUB(135, 206, 250)},
  {ezColorGammaUB(178, 34, 34), ezColorGammaUB(46, 139, 87), ezColorGammaUB(65, 105, 225)},
  {ezColorGammaUB(211, 122, 122), ezColorGammaUB(144, 238, 144), ezColorGammaUB(135, 206, 235)},
  {ezColorGammaUB(219, 112, 147), ezColorGammaUB(0, 128, 0), ezColorGammaUB(70, 130, 180)},
  {ezColorGammaUB(255, 182, 193), ezColorGammaUB(102, 205, 170), ezColorGammaUB(100, 149, 237)},
};

static ezColorGammaUB g_FloatColors[10] = {
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

ezUuid ezPropertyAnimAssetDocument::CreateTrack(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezPropertyAnimTarget::Enum target)
{
  ezStringBuilder sObjectSearchSequence;
  ezStringBuilder sComponentType;
  ezStringBuilder sPropertyPath;
  GenerateTrackInfo(pObject, pProp, index, sObjectSearchSequence, sComponentType, sPropertyPath);

  ezObjectCommandAccessor accessor(GetCommandHistory());
  const ezRTTI* pTrackType = ezGetStaticRTTI<ezPropertyAnimationTrack>();
  ezUuid newTrack;
  EZ_VERIFY(
    accessor.AddObject(GetPropertyObject(), ezGetStaticRTTI<ezPropertyAnimationTrackGroup>()->FindPropertyByName("Tracks"), -1, pTrackType, newTrack)
      .Succeeded(),
    "Adding track failed.");
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

    const ezUInt32 uiNameHash = ezHashingUtils::xxHash32(sObjectSearchSequence.GetData(), sObjectSearchSequence.GetElementCount());
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
      default:
        break;
    }

    accessor.SetValue(pFloatCurveObject, pColorProp, color).AssertSuccess();
  }

  return newTrack;
}

ezUuid ezPropertyAnimAssetDocument::FindCurveCp(const ezUuid& trackGuid, ezInt64 iTickX)
{
  auto pTrack = GetTrack(trackGuid);
  ezInt32 iIndex = -1;
  for (ezUInt32 i = 0; i < pTrack->m_FloatCurve.m_ControlPoints.GetCount(); i++)
  {
    if (pTrack->m_FloatCurve.m_ControlPoints[i].m_iTick == iTickX)
    {
      iIndex = (ezInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return ezUuid();

  const ezAbstractProperty* pCurveProp = ezGetStaticRTTI<ezPropertyAnimationTrack>()->FindPropertyByName("FloatCurve");
  const ezDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  ezUuid curveGuid = m_pObjectAccessor->Get<ezUuid>(trackObject, pCurveProp);
  const ezAbstractProperty* pControlPointsProp = ezGetStaticRTTI<ezSingleCurveData>()->FindPropertyByName("ControlPoints");
  const ezDocumentObject* curveObject = GetObjectManager()->GetObject(curveGuid);
  ezUuid cpGuid = m_pObjectAccessor->Get<ezUuid>(curveObject, pControlPointsProp, iIndex);
  return cpGuid;
}

ezUuid ezPropertyAnimAssetDocument::InsertCurveCpAt(const ezUuid& track, ezInt64 iTickX, double fNewPosY)
{
  ezObjectCommandAccessor accessor(GetCommandHistory());
  ezObjectAccessorBase& acc = accessor;
  acc.StartTransaction("Insert Control Point");

  const ezDocumentObject* trackObject = GetObjectManager()->GetObject(track);
  const ezVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  ezUuid newObjectGuid;
  EZ_VERIFY(acc.AddObjectByName(accessor.GetObject(curveGuid.Get<ezUuid>()), "ControlPoints", -1, ezGetStaticRTTI<ezCurveControlPointData>(), newObjectGuid).Succeeded(),
    "");
  auto curveCPObj = accessor.GetObject(newObjectGuid);
  EZ_VERIFY(acc.SetValueByName(curveCPObj, "Tick", iTickX).Succeeded(), "");
  EZ_VERIFY(acc.SetValueByName(curveCPObj, "Value", fNewPosY).Succeeded(), "");
  EZ_VERIFY(acc.SetValueByName(curveCPObj, "LeftTangent", ezVec2(-0.1f, 0.0f)).Succeeded(), "");
  EZ_VERIFY(acc.SetValueByName(curveCPObj, "RightTangent", ezVec2(+0.1f, 0.0f)).Succeeded(), "");

  acc.FinishTransaction();

  return newObjectGuid;
}

ezUuid ezPropertyAnimAssetDocument::FindGradientColorCp(const ezUuid& trackGuid, ezInt64 iTickX)
{
  auto pTrack = GetTrack(trackGuid);
  ezInt32 iIndex = -1;
  for (ezUInt32 i = 0; i < pTrack->m_ColorGradient.m_ColorCPs.GetCount(); i++)
  {
    if (pTrack->m_ColorGradient.m_ColorCPs[i].m_iTick == iTickX)
    {
      iIndex = (ezInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return ezUuid();

  const ezAbstractProperty* pCurveProp = ezGetStaticRTTI<ezPropertyAnimationTrack>()->FindPropertyByName("Gradient");
  const ezDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  ezUuid curveGuid = m_pObjectAccessor->Get<ezUuid>(trackObject, pCurveProp);
  const ezAbstractProperty* pControlPointsProp = ezGetStaticRTTI<ezColorGradientAssetData>()->FindPropertyByName("ColorCPs");
  const ezDocumentObject* curveObject = GetObjectManager()->GetObject(curveGuid);
  ezUuid cpGuid = m_pObjectAccessor->Get<ezUuid>(curveObject, pControlPointsProp, iIndex);
  return cpGuid;
}

ezUuid ezPropertyAnimAssetDocument::InsertGradientColorCpAt(const ezUuid& trackGuid, ezInt64 iTickX, const ezColorGammaUB& color)
{
  ezObjectCommandAccessor accessor(GetCommandHistory());
  ezObjectAccessorBase& acc = accessor;

  const ezDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  const ezUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<ezUuid>();
  const ezDocumentObject* gradientObject = GetObjectManager()->GetObject(gradientGuid);

  acc.StartTransaction("Add Color Control Point");
  ezUuid newObjectGuid;
  EZ_VERIFY(acc.AddObjectByName(gradientObject, "ColorCPs", -1, ezGetStaticRTTI<ezColorControlPoint>(), newObjectGuid).Succeeded(), "");
  const ezDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  EZ_VERIFY(acc.SetValueByName(cpObject, "Tick", iTickX).Succeeded(), "");
  EZ_VERIFY(acc.SetValueByName(cpObject, "Red", color.r).Succeeded(), "");
  EZ_VERIFY(acc.SetValueByName(cpObject, "Green", color.g).Succeeded(), "");
  EZ_VERIFY(acc.SetValueByName(cpObject, "Blue", color.b).Succeeded(), "");
  acc.FinishTransaction();
  return newObjectGuid;
}

ezUuid ezPropertyAnimAssetDocument::FindGradientAlphaCp(const ezUuid& trackGuid, ezInt64 iTickX)
{
  auto pTrack = GetTrack(trackGuid);
  ezInt32 iIndex = -1;
  for (ezUInt32 i = 0; i < pTrack->m_ColorGradient.m_AlphaCPs.GetCount(); i++)
  {
    if (pTrack->m_ColorGradient.m_AlphaCPs[i].m_iTick == iTickX)
    {
      iIndex = (ezInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return ezUuid();

  const ezAbstractProperty* pCurveProp = ezGetStaticRTTI<ezPropertyAnimationTrack>()->FindPropertyByName("Gradient");
  const ezDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  ezUuid curveGuid = m_pObjectAccessor->Get<ezUuid>(trackObject, pCurveProp);
  const ezAbstractProperty* pControlPointsProp = ezGetStaticRTTI<ezColorGradientAssetData>()->FindPropertyByName("AlphaCPs");
  const ezDocumentObject* curveObject = GetObjectManager()->GetObject(curveGuid);
  ezUuid cpGuid = m_pObjectAccessor->Get<ezUuid>(curveObject, pControlPointsProp, iIndex);
  return cpGuid;
}

ezUuid ezPropertyAnimAssetDocument::InsertGradientAlphaCpAt(const ezUuid& trackGuid, ezInt64 iTickX, ezUInt8 uiAlpha)
{
  ezObjectCommandAccessor accessor(GetCommandHistory());
  ezObjectAccessorBase& acc = accessor;

  const ezDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  const ezUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<ezUuid>();
  const ezDocumentObject* gradientObject = GetObjectManager()->GetObject(gradientGuid);

  acc.StartTransaction("Add Alpha Control Point");
  ezUuid newObjectGuid;
  EZ_VERIFY(acc.AddObjectByName(gradientObject, "AlphaCPs", -1, ezGetStaticRTTI<ezAlphaControlPoint>(), newObjectGuid).Succeeded(), "");
  const ezDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  EZ_VERIFY(acc.SetValueByName(cpObject, "Tick", iTickX).Succeeded(), "");
  EZ_VERIFY(acc.SetValueByName(cpObject, "Alpha", uiAlpha).Succeeded(), "");
  acc.FinishTransaction();
  return newObjectGuid;
}

ezUuid ezPropertyAnimAssetDocument::FindGradientIntensityCp(const ezUuid& trackGuid, ezInt64 iTickX)
{
  auto pTrack = GetTrack(trackGuid);
  ezInt32 iIndex = -1;
  for (ezUInt32 i = 0; i < pTrack->m_ColorGradient.m_IntensityCPs.GetCount(); i++)
  {
    if (pTrack->m_ColorGradient.m_IntensityCPs[i].m_iTick == iTickX)
    {
      iIndex = (ezInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return ezUuid();

  const ezAbstractProperty* pCurveProp = ezGetStaticRTTI<ezPropertyAnimationTrack>()->FindPropertyByName("Gradient");
  const ezDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  ezUuid curveGuid = m_pObjectAccessor->Get<ezUuid>(trackObject, pCurveProp);
  const ezAbstractProperty* pControlPointsProp = ezGetStaticRTTI<ezColorGradientAssetData>()->FindPropertyByName("IntensityCPs");
  const ezDocumentObject* curveObject = GetObjectManager()->GetObject(curveGuid);
  ezUuid cpGuid = m_pObjectAccessor->Get<ezUuid>(curveObject, pControlPointsProp, iIndex);
  return cpGuid;
}

ezUuid ezPropertyAnimAssetDocument::InsertGradientIntensityCpAt(const ezUuid& trackGuid, ezInt64 iTickX, float fIntensity)
{
  ezObjectCommandAccessor accessor(GetCommandHistory());
  ezObjectAccessorBase& acc = accessor;

  const ezDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  const ezUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<ezUuid>();
  const ezDocumentObject* gradientObject = GetObjectManager()->GetObject(gradientGuid);

  acc.StartTransaction("Add Intensity Control Point");
  ezUuid newObjectGuid;
  EZ_VERIFY(acc.AddObjectByName(gradientObject, "IntensityCPs", -1, ezGetStaticRTTI<ezIntensityControlPoint>(), newObjectGuid).Succeeded(), "");
  const ezDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  EZ_VERIFY(acc.SetValueByName(cpObject, "Tick", iTickX).Succeeded(), "");
  EZ_VERIFY(acc.SetValueByName(cpObject, "Intensity", fIntensity).Succeeded(), "");
  acc.FinishTransaction();
  return newObjectGuid;
}

ezUuid ezPropertyAnimAssetDocument::InsertEventTrackCpAt(ezInt64 iTickX, const char* szValue)
{
  ezObjectCommandAccessor accessor(GetCommandHistory());
  ezObjectAccessorBase& acc = accessor;
  acc.StartTransaction("Insert Event");

  const ezAbstractProperty* pTrackProp = ezGetStaticRTTI<ezPropertyAnimationTrackGroup>()->FindPropertyByName("EventTrack");
  ezUuid trackGuid = accessor.Get<ezUuid>(GetPropertyObject(), pTrackProp);

  ezUuid newObjectGuid;
  EZ_VERIFY(acc.AddObjectByName(accessor.GetObject(trackGuid), "ControlPoints", -1, ezGetStaticRTTI<ezEventTrackControlPointData>(), newObjectGuid).Succeeded(),
    "");
  const ezDocumentObject* pCPObj = accessor.GetObject(newObjectGuid);
  EZ_VERIFY(acc.SetValueByName(pCPObj, "Tick", iTickX).Succeeded(), "");
  EZ_VERIFY(acc.SetValueByName(pCPObj, "Event", szValue).Succeeded(), "");

  acc.FinishTransaction();

  return newObjectGuid;
}
