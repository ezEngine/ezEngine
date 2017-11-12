#include <PCH.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectAccessor.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <GameEngine/Resources/PropertyAnimResource.h>
#include <ToolsFoundation/Object/DocumentObjectVisitor.h>
#include <Core/World/GameObject.h>

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
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezPropertyAnimMode, m_Mode),
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

  GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::CommandHistoryEventHandler, this));
}

ezPropertyAnimAssetDocument::~ezPropertyAnimAssetDocument()
{
  m_GameObjectContextEvents.RemoveEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::GameObjectContextEventHandler, this));
  GetCommandHistory()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::CommandHistoryEventHandler, this));

  GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::TreeStructureEventHandler, this));
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::TreePropertyEventHandler, this));
}

ezObjectAccessorBase* ezPropertyAnimAssetDocument::GetObjectAccessor() const
{
  return m_pAccessor.Borrow();
}

ezInt64 ezPropertyAnimAssetDocument::GetAnimationDurationTicks() const
{
  if (m_iCachedAnimationDuration != 0)
    return m_iCachedAnimationDuration;

  const ezPropertyAnimationTrackGroup* pProp = GetProperties();

  // minimum duration
  m_iCachedAnimationDuration = 480; // 1/10th second

  for (ezUInt32 i = 0; i < pProp->m_Tracks.GetCount(); ++i)
  {
    const ezPropertyAnimationTrack* pTrack = pProp->m_Tracks[i];

    for (const auto& cp : pTrack->m_FloatCurve.m_ControlPoints)
    {
      m_iCachedAnimationDuration = ezMath::Max(m_iCachedAnimationDuration, cp.m_iTick);
    }

    for (const auto& cp : pTrack->m_ColorGradient.m_ColorCPs)
    {
      m_iCachedAnimationDuration = ezMath::Max<ezInt64>(m_iCachedAnimationDuration, cp.m_iTick);
    }

    for (const auto& cp : pTrack->m_ColorGradient.m_AlphaCPs)
    {
      m_iCachedAnimationDuration = ezMath::Max<ezInt64>(m_iCachedAnimationDuration, cp.m_iTick);
    }

    for (const auto& cp : pTrack->m_ColorGradient.m_IntensityCPs)
    {
      m_iCachedAnimationDuration = ezMath::Max<ezInt64>(m_iCachedAnimationDuration, cp.m_iTick);
    }
  }

  if (m_iLastAnimationDuration != m_iCachedAnimationDuration)
  {
    m_iLastAnimationDuration = m_iCachedAnimationDuration;

    ezPropertyAnimAssetDocumentEvent e;
    e.m_pDocument = this;
    e.m_Type = ezPropertyAnimAssetDocumentEvent::Type::AnimationLengthChanged;
    m_PropertyAnimEvents.Broadcast(e);
  }

  return m_iCachedAnimationDuration;
}


ezTime ezPropertyAnimAssetDocument::GetAnimationDurationTime() const
{
  const ezInt64 ticks = GetAnimationDurationTicks();

  return ezTime::Seconds(ticks / 4800.0);
}

void ezPropertyAnimAssetDocument::SetScrubberPosition(ezUInt64 uiTick)
{
  const ezUInt32 uiTicksPerFrame = 4800 / GetProperties()->m_uiFramesPerSecond;
  uiTick = (ezUInt64)ezMath::Round((double)uiTick, (double)uiTicksPerFrame);

  if (m_uiScrubberTickPos == uiTick)
    return;

  m_uiScrubberTickPos = uiTick;
  ApplyAnimation();

  ezPropertyAnimAssetDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = ezPropertyAnimAssetDocumentEvent::Type::ScrubberPositionChanged;
  m_PropertyAnimEvents.Broadcast(e);
}

ezStatus ezPropertyAnimAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  const ezPropertyAnimationTrackGroup* pProp = GetProperties();

  ezResourceHandleWriteContext HandleContext;
  HandleContext.BeginWritingToStream(&stream);

  ezPropertyAnimResourceDescriptor desc;
  desc.m_Mode = pProp->m_Mode;
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
  SUPER::InitializeAfterLoading();
  GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::TreeStructureEventHandler, this));
  GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocument::TreePropertyEventHandler, this));
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

void ezPropertyAnimAssetDocument::CommandHistoryEventHandler(const ezCommandHistoryEvent& e)
{
  switch (e.m_Type)
  {
  case ezCommandHistoryEvent::Type::UndoEnded:
  case ezCommandHistoryEvent::Type::RedoEnded:
    ClearCachedAnimationDuration();
    GetAnimationDurationTicks();
    break;
  }
}

void ezPropertyAnimAssetDocument::TreeStructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  auto pManager = static_cast<ezPropertyAnimObjectManager*>(GetObjectManager());
  if (e.m_pPreviousParent && pManager->IsTemporary(e.m_pPreviousParent, e.m_sParentProperty ))
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

  {
    ezDocumentObjectVisitor visitor(GetObjectManager(), "Children", "TempObjects");
    auto obj = m_Context.GetObjectByGUID(track);
    EZ_ASSERT_DEBUG(obj.m_pType == ezGetStaticRTTI<ezPropertyAnimationTrack>(), "Track guid does not resolve to a track, "
      "either the track is not yet created in the mirror or already destroyed. Make sure callbacks are executed in the right order.");
    auto pTrack = static_cast<const ezPropertyAnimationTrack*>(obj.m_pObject);
    ezHybridArray<const ezDocumentObject*, 8> input;
    input.PushBack(pContext);
    ezHybridArray<const ezDocumentObject*, 8> output;

    // Find objects that match the search path
    ezStringBuilder sObjectSearchSequence = pTrack->m_sObjectSearchSequence;
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
        if (pTrack->m_sComponentType.IsEmpty())
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
            if (pTrack->m_sComponentType == pChild->GetType()->GetTypeName())
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
      if (ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(pTrack->m_sPropertyPath))
      {
        //#TODO: Support property path (sub-objects and indices into arrays / maps)
        PropertyKey key;
        key.m_Object = pObject->GetGuid();
        key.m_pProperty = pProp;
        keys.PushBack(key);
      }
    }
  }

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
  for (const ezUuid& track : value.m_Tracks)
  {
    auto obj = m_Context.GetObjectByGUID(track);
    EZ_ASSERT_DEBUG(obj.m_pType == ezGetStaticRTTI<ezPropertyAnimationTrack>(), "Track guid does not resolve to a track, "
      "either the track is not yet created in the mirror or already destroyed. Make sure callbacks are executed in the right order.");
    auto pTrack = static_cast<const ezPropertyAnimationTrack*>(obj.m_pObject);
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
          double fValue = pTrack->m_FloatCurve.Evaluate(m_uiScrubberTickPos);
          EZ_CHECK_AT_COMPILETIME_MSG(ezPropertyAnimTarget::VectorX == 1, "Need to fix enum index code below");
          ezReflectionUtils::SetComponent(animValue, (ezUInt32)pTrack->m_Target - 1, fValue);
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
  ezDocumentObject* pObj = GetObjectManager()->GetObject(key.m_Object);
  GetObjectManager()->SetValue(pObj, key.m_pProperty->GetPropertyName(), animValue, key.m_Index);
}
