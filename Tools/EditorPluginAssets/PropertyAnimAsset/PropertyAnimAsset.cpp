#include <PCH.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
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
  : ezSimpleAssetDocument<ezPropertyAnimationTrackGroup>(szDocumentPath)
{
}


ezTime ezPropertyAnimAssetDocument::GetAnimationDuration() const
{
  const ezPropertyAnimationTrackGroup* pProp = GetProperties();

  // minimum duration
  double length = 0.1;

  for (ezUInt32 i = 0; i < pProp->m_Tracks.GetCount(); ++i)
  {
    const ezPropertyAnimationTrack* pTrack = pProp->m_Tracks[i];

    for (const auto& cp : pTrack->m_FloatCurve.m_ControlPoints)
    {
      length = ezMath::Max(length, cp.GetTickAsTime());
    }
  }

  return ezTime::Seconds(length);
}

ezStatus ezPropertyAnimAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  const ezPropertyAnimationTrackGroup* pProp = GetProperties();

  ezResourceHandleWriteContext HandleContext;
  HandleContext.BeginWritingToStream(&stream);

  ezPropertyAnimResourceDescriptor desc;
  desc.m_Mode = pProp->m_Mode;
  desc.m_AnimationDuration = GetAnimationDuration();

  for (ezUInt32 i = 0; i < pProp->m_Tracks.GetCount(); ++i)
  {
    const ezPropertyAnimationTrack* pTrack = pProp->m_Tracks[i];

    if (pTrack->m_Target == ezPropertyAnimTarget::Color)
    {
      // TODO

      //auto& anim = desc.m_ColorAnimations.ExpandAndGetRef();
      //anim.m_sObjectSearchSequence = pTrack->m_sObjectSearchSequence;
      //anim.m_sComponentType = pTrack->m_sComponentType;
      //anim.m_sPropertyPath = pTrack->m_sPropertyPath;
      //anim.m_Target = pTrack->m_Target;
      //pTrack->m_ColorGradient.ConvertToRuntimeData(anim.m_Curve);
      //anim.m_Gradient.SortControlPoints();
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

