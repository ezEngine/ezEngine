#include <PCH.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectAccessor.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <GameEngine/Resources/PropertyAnimResource.h>

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

void ezPropertyAnimAssetDocument::GameObjectContextEventHandler(const ezGameObjectContextEvent& e)
{
  switch (e.m_Type)
  {
  case ezGameObjectContextEvent::Type::ContextAboutToBeChanged:
    static_cast<ezPropertyAnimObjectManager*>(GetObjectManager())->SetAllowStructureChangeOnTemporaries(true);
    break;
  case ezGameObjectContextEvent::Type::ContextChanged:
    static_cast<ezPropertyAnimObjectManager*>(GetObjectManager())->SetAllowStructureChangeOnTemporaries(false);
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
