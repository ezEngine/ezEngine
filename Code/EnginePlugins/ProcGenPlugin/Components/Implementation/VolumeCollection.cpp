#include <ProcGenPluginPCH.h>

#include <ProcGenPlugin/Components/VolumeCollection.h>

namespace
{
  EZ_FORCE_INLINE float ApplyValue(ezProcGenBlendMode::Enum blendMode, float fInitialValue, float fNewValue)
  {
    switch (blendMode)
    {
      case ezProcGenBlendMode::Add:
        return fInitialValue + fNewValue;
      case ezProcGenBlendMode::Subtract:
        return fInitialValue - fNewValue;
      case ezProcGenBlendMode::Multiply:
        return fInitialValue * fNewValue;
      case ezProcGenBlendMode::Divide:
        return fInitialValue / fNewValue;
      case ezProcGenBlendMode::Max:
        return ezMath::Max(fInitialValue, fNewValue);
      case ezProcGenBlendMode::Min:
        return ezMath::Min(fInitialValue, fNewValue);
      case ezProcGenBlendMode::Set:
        return fNewValue;
      default:
        return fInitialValue;
    }
  }
} // namespace

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVolumeCollection, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

//static
ezUInt32 ezVolumeCollection::ComputeSortingKey(float fSortOrder, float fMaxScale)
{
  ezUInt32 uiSortingKey = (ezUInt32)(ezMath::Min(fSortOrder * 512.0f, 32767.0f) + 32768.0f);
  uiSortingKey = (uiSortingKey << 16) | ((ezUInt32)(fMaxScale * 100.0f) & 0xFFFF);
  return uiSortingKey;
}

float ezVolumeCollection::EvaluateAtGlobalPosition(const ezVec3& vPosition, float fInitialValue /*= 0.0f*/) const
{
  ezSimdVec4f globalPos = ezSimdConversion::ToVec3(vPosition);
  float fValue = fInitialValue;

  for (auto& sphere : m_Spheres)
  {
    const ezSimdVec4f localPos = sphere.m_GlobalToLocalTransform.TransformPosition(globalPos);
    const float distSquared = localPos.GetLengthSquared<3>();
    if (distSquared <= 1.0f)
    {
      const float fNewValue = ApplyValue(sphere.m_BlendMode, fValue, sphere.m_fValue);
      const float fAlpha = ezMath::Saturate(ezMath::Sqrt(distSquared) * sphere.m_fFadeOutScale + sphere.m_fFadeOutBias);
      fValue = ezMath::Lerp(fValue, fNewValue, fAlpha);
    }
  }

  for (auto& box : m_Boxes)
  {
    const ezSimdVec4f absLocalPos = box.m_GlobalToLocalTransform.TransformPosition(globalPos).Abs();
    if ((absLocalPos <= ezSimdVec4f(1.0f)).AllSet<3>())
    {
      const float fNewValue = ApplyValue(box.m_BlendMode, fValue, box.m_fValue);
      ezSimdVec4f vAlpha = absLocalPos.CompMul(ezSimdConversion::ToVec3(box.m_vFadeOutScale)) + ezSimdConversion::ToVec3(box.m_vFadeOutBias);
      vAlpha = vAlpha.CompMin(ezSimdVec4f(1.0f)).CompMax(ezSimdVec4f::ZeroVector());
      const float fAlpha = vAlpha.x() * vAlpha.y() * vAlpha.z();
      fValue = ezMath::Lerp(fValue, fNewValue, fAlpha);
    }
  }

  return fValue;
}

//static
void ezVolumeCollection::ExtractVolumesInBox(const ezWorld& world, const ezBoundingBox& box, ezSpatialData::Category spatialCategory,
  const ezTagSet& includeTags, ezVolumeCollection& out_Collection, const ezRTTI* pComponentBaseType)
{
  ezMsgExtractVolumes msg;
  msg.m_pCollection = &out_Collection;

  world.GetSpatialSystem()->FindObjectsInBox(box, spatialCategory.GetBitmask(), [&](ezGameObject* pObject) {
    if (includeTags.IsEmpty() || includeTags.IsAnySet(pObject->GetTags()))
    {
      if (pComponentBaseType != nullptr)
      {
        ezHybridArray<const ezComponent*, 8> components;
        pObject->TryGetComponentsOfBaseType(pComponentBaseType, components);

        for (auto pComponent : components)
        {
          pComponent->SendMessage(msg);
        }
      }
      else
      {
        pObject->SendMessage(msg);
      }
    }
    return ezVisitorExecution::Continue;
  });

  out_Collection.m_Spheres.Sort();
  out_Collection.m_Boxes.Sort();
}

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgExtractVolumes);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgExtractVolumes, 1, ezRTTIDefaultAllocator<ezMsgExtractVolumes>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

void ezMsgExtractVolumes::AddSphere(const ezSimdTransform& transform, float fRadius, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder,
  float fValue, float fFadeOutStart)
{
  ezSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale *= fRadius;

  auto& sphere = m_pCollection->m_Spheres.ExpandAndGetRef();
  sphere.m_GlobalToLocalTransform = scaledTransform.GetAsMat4().GetInverse();
  sphere.m_BlendMode = blendMode;
  sphere.m_fValue = fValue;
  sphere.m_uiSortingKey = ezVolumeCollection::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  sphere.m_fFadeOutScale = -1.0f / ezMath::Max(1.0f - fFadeOutStart, 0.0001f);
  sphere.m_fFadeOutBias = -sphere.m_fFadeOutScale;
}

void ezMsgExtractVolumes::AddBox(const ezSimdTransform& transform, const ezVec3& vExtents, ezEnum<ezProcGenBlendMode> blendMode,
  float fSortOrder, float fValue, const ezVec3& vFadeOutStart)
{
  ezSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale = scaledTransform.m_Scale.CompMul(ezSimdConversion::ToVec3(vExtents)) * 0.5f;

  auto& box = m_pCollection->m_Boxes.ExpandAndGetRef();
  box.m_GlobalToLocalTransform = scaledTransform.GetAsMat4().GetInverse();
  box.m_BlendMode = blendMode;
  box.m_fValue = fValue;
  box.m_uiSortingKey = ezVolumeCollection::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  box.m_vFadeOutScale = ezVec3(-1.0f).CompDiv((ezVec3(1.0f) - vFadeOutStart).CompMax(ezVec3(0.0001f)));
  box.m_vFadeOutBias = -box.m_vFadeOutScale;
}
