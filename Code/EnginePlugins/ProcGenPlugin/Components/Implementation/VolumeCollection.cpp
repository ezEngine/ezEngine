#include <ProcGenPlugin/ProcGenPluginPCH.h>

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

EZ_CHECK_AT_COMPILETIME(sizeof(ezVolumeCollection::Sphere) == 64);
EZ_CHECK_AT_COMPILETIME(sizeof(ezVolumeCollection::Box) == 80);

void ezVolumeCollection::Shape::SetGlobalToLocalTransform(const ezSimdMat4f& t)
{
  ezSimdVec4f r0, r1, r2, r3;
  t.GetRows(r0, r1, r2, r3);

  m_GlobalToLocalTransform0 = ezSimdConversion::ToVec4(r0);
  m_GlobalToLocalTransform1 = ezSimdConversion::ToVec4(r1);
  m_GlobalToLocalTransform2 = ezSimdConversion::ToVec4(r2);
}

ezSimdMat4f ezVolumeCollection::Shape::GetGlobalToLocalTransform() const
{
  ezSimdMat4f m;
  m.SetRows(ezSimdConversion::ToVec4(m_GlobalToLocalTransform0), ezSimdConversion::ToVec4(m_GlobalToLocalTransform1),
    ezSimdConversion::ToVec4(m_GlobalToLocalTransform2), ezSimdVec4f(0, 0, 0, 1));

  return m;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVolumeCollection, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

// static
ezUInt32 ezVolumeCollection::ComputeSortingKey(float fSortOrder, float fMaxScale)
{
  ezUInt32 uiSortingKey = (ezUInt32)(ezMath::Min(fSortOrder * 512.0f, 32767.0f) + 32768.0f);
  uiSortingKey = (uiSortingKey << 16) | (0xFFFF - ((ezUInt32)(fMaxScale * 100.0f) & 0xFFFF));
  return uiSortingKey;
}

float ezVolumeCollection::EvaluateAtGlobalPosition(const ezVec3& vPosition, float fInitialValue /*= 0.0f*/) const
{
  ezSimdVec4f globalPos = ezSimdConversion::ToVec3(vPosition);
  float fValue = fInitialValue;

  for (auto pShape : m_SortedShapes)
  {
    if (pShape->m_Type == ShapeType::Sphere)
    {
      auto& sphere = *static_cast<const Sphere*>(pShape);
      const ezSimdVec4f localPos = sphere.GetGlobalToLocalTransform().TransformPosition(globalPos);
      const float distSquared = localPos.GetLengthSquared<3>();
      if (distSquared <= 1.0f)
      {
        const float fNewValue = ApplyValue(sphere.m_BlendMode, fValue, sphere.m_fValue);
        const float fAlpha = ezMath::Saturate(ezMath::Sqrt(distSquared) * sphere.m_fFadeOutScale + sphere.m_fFadeOutBias);
        fValue = ezMath::Lerp(fValue, fNewValue, fAlpha);
      }
    }
    else if (pShape->m_Type == ShapeType::Box)
    {
      auto& box = *static_cast<const Box*>(pShape);
      const ezSimdVec4f absLocalPos = box.GetGlobalToLocalTransform().TransformPosition(globalPos).Abs();
      if ((absLocalPos <= ezSimdVec4f(1.0f)).AllSet<3>())
      {
        const float fNewValue = ApplyValue(box.m_BlendMode, fValue, box.m_fValue);
        ezSimdVec4f vAlpha = absLocalPos.CompMul(ezSimdConversion::ToVec3(box.m_vFadeOutScale)) + ezSimdConversion::ToVec3(box.m_vFadeOutBias);
        vAlpha = vAlpha.CompMin(ezSimdVec4f(1.0f)).CompMax(ezSimdVec4f::ZeroVector());
        const float fAlpha = vAlpha.x() * vAlpha.y() * vAlpha.z();
        fValue = ezMath::Lerp(fValue, fNewValue, fAlpha);
      }
    }
  }

  return fValue;
}

// static
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

  const ezUInt32 uiNumSpheres = out_Collection.m_Spheres.GetCount();
  const ezUInt32 uiNumBoxes = out_Collection.m_Boxes.GetCount();

  out_Collection.m_SortedShapes.Reserve(uiNumSpheres + uiNumBoxes);

  ezUInt32 uiCurrentSphere = 0;
  ezUInt32 uiCurrentBox = 0;

  while (uiCurrentSphere < uiNumSpheres || uiCurrentBox < uiNumBoxes)
  {
    Sphere* pSphere = uiCurrentSphere < uiNumSpheres ? &out_Collection.m_Spheres[uiCurrentSphere] : nullptr;
    Box* pBox = uiCurrentBox < uiNumBoxes ? &out_Collection.m_Boxes[uiCurrentBox] : nullptr;

    if (pSphere != nullptr && pBox != nullptr)
    {
      if (pSphere->m_uiSortingKey < pBox->m_uiSortingKey)
        pBox = nullptr;
      else
        pSphere = nullptr;
    }

    if (pSphere == nullptr)
    {
      out_Collection.m_SortedShapes.PushBack(pBox);
      ++uiCurrentBox;
    }
    else
    {
      out_Collection.m_SortedShapes.PushBack(pSphere);
      ++uiCurrentSphere;
    }
  }
}

void ezVolumeCollection::AddSphere(
  const ezSimdTransform& transform, float fRadius, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder, float fValue, float fFadeOutStart)
{
  ezSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale *= fRadius;

  auto& sphere = m_Spheres.ExpandAndGetRef();
  sphere.SetGlobalToLocalTransform(scaledTransform.GetAsMat4().GetInverse());
  sphere.m_Type = ShapeType::Sphere;
  sphere.m_BlendMode = blendMode;
  sphere.m_fValue = fValue;
  sphere.m_uiSortingKey = ezVolumeCollection::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  sphere.m_fFadeOutScale = -1.0f / ezMath::Max(1.0f - fFadeOutStart, 0.0001f);
  sphere.m_fFadeOutBias = -sphere.m_fFadeOutScale;
}

void ezVolumeCollection::AddBox(const ezSimdTransform& transform, const ezVec3& vExtents, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder,
  float fValue, const ezVec3& vFadeOutStart)
{
  ezSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale = scaledTransform.m_Scale.CompMul(ezSimdConversion::ToVec3(vExtents)) * 0.5f;

  auto& box = m_Boxes.ExpandAndGetRef();
  box.SetGlobalToLocalTransform(scaledTransform.GetAsMat4().GetInverse());
  box.m_Type = ShapeType::Box;
  box.m_BlendMode = blendMode;
  box.m_fValue = fValue;
  box.m_uiSortingKey = ezVolumeCollection::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  box.m_vFadeOutScale = ezVec3(-1.0f).CompDiv((ezVec3(1.0f) - vFadeOutStart).CompMax(ezVec3(0.0001f)));
  box.m_vFadeOutBias = -box.m_vFadeOutScale;
}

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgExtractVolumes);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgExtractVolumes, 1, ezRTTIDefaultAllocator<ezMsgExtractVolumes>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
