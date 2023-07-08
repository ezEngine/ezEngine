#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <GameEngine/Utils/ImageDataResource.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <Texture/Image/ImageUtils.h>

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

float ezVolumeCollection::EvaluateAtGlobalPosition(const ezSimdVec4f& vPosition, float fInitialValue, ezProcVolumeImageMode::Enum imgMode, const ezColor& refColor) const
{
  float fValue = fInitialValue;

  for (auto pShape : m_SortedShapes)
  {
    if (pShape->m_Type == ShapeType::Sphere)
    {
      auto& sphere = *static_cast<const Sphere*>(pShape);
      const ezSimdVec4f localPos = sphere.GetGlobalToLocalTransform().TransformPosition(vPosition);
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
      const ezSimdVec4f absLocalPos = box.GetGlobalToLocalTransform().TransformPosition(vPosition).Abs();
      if ((absLocalPos <= ezSimdVec4f(1.0f)).AllSet<3>())
      {
        const float fNewValue = ApplyValue(box.m_BlendMode, fValue, box.m_fValue);
        ezSimdVec4f vAlpha = absLocalPos.CompMul(ezSimdConversion::ToVec3(box.m_vFadeOutScale)) + ezSimdConversion::ToVec3(box.m_vFadeOutBias);
        vAlpha = vAlpha.CompMin(ezSimdVec4f(1.0f)).CompMax(ezSimdVec4f::ZeroVector());
        const float fAlpha = vAlpha.x() * vAlpha.y() * vAlpha.z();
        fValue = ezMath::Lerp(fValue, fNewValue, fAlpha);
      }
    }
    else if (pShape->m_Type == ShapeType::Image)
    {
      auto& image = *static_cast<const Image*>(pShape);

      const ezSimdVec4f localPos = image.GetGlobalToLocalTransform().TransformPosition(vPosition);
      const ezSimdVec4f absLocalPos = localPos.Abs();

      if ((absLocalPos <= ezSimdVec4f(1.0f)).AllSet<3>() && image.m_pPixelData != nullptr)
      {
        ezVec2 uv;
        uv.x = static_cast<float>(localPos.x()) * 0.5f + 0.5f;
        uv.y = static_cast<float>(localPos.y()) * 0.5f + 0.5f;

        const ezColor col = ezImageUtils::NearestSample(image.m_pPixelData, image.m_uiImageWidth, image.m_uiImageHeight, ezImageAddressMode::Clamp, uv);

        float fValueToUse = image.m_fValue;
        EZ_IGNORE_UNUSED(fValueToUse);

        switch (imgMode)
        {
          case ezProcVolumeImageMode::ReferenceColor:
            fValueToUse = image.m_fValue;
            break;
          case ezProcVolumeImageMode::ChannelR:
            fValueToUse = image.m_fValue * col.r;
            break;
          case ezProcVolumeImageMode::ChannelG:
            fValueToUse = image.m_fValue * col.g;
            break;
          case ezProcVolumeImageMode::ChannelB:
            fValueToUse = image.m_fValue * col.b;
            break;
          case ezProcVolumeImageMode::ChannelA:
            fValueToUse = image.m_fValue * col.a;
            break;
        }

        if (imgMode != ezProcVolumeImageMode::ReferenceColor || col.IsEqualRGBA(refColor, 0.1f))
        {
          const float fNewValue = ApplyValue(image.m_BlendMode, fValue, fValueToUse);
          ezSimdVec4f vAlpha = absLocalPos.CompMul(ezSimdConversion::ToVec3(image.m_vFadeOutScale)) + ezSimdConversion::ToVec3(image.m_vFadeOutBias);
          vAlpha = vAlpha.CompMin(ezSimdVec4f(1.0f)).CompMax(ezSimdVec4f::ZeroVector());
          const float fAlpha = vAlpha.x() * vAlpha.y() * vAlpha.z();
          fValue = ezMath::Lerp(fValue, fNewValue, fAlpha);
        }
      }
    }
  }

  return fValue;
}

// static
void ezVolumeCollection::ExtractVolumesInBox(const ezWorld& world, const ezBoundingBox& box, ezSpatialData::Category spatialCategory,
  const ezTagSet& includeTags, ezVolumeCollection& out_collection, const ezRTTI* pComponentBaseType)
{
  ezMsgExtractVolumes msg;
  msg.m_pCollection = &out_collection;

  ezSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = spatialCategory.GetBitmask();
  queryParams.m_IncludeTags = includeTags;

  world.GetSpatialSystem()->FindObjectsInBox(box, queryParams, [&](ezGameObject* pObject) {
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

    return ezVisitorExecution::Continue;
  });

  out_collection.m_Spheres.Sort();
  out_collection.m_Boxes.Sort();
  out_collection.m_Images.Sort();

  const ezUInt32 uiNumSpheres = out_collection.m_Spheres.GetCount();
  const ezUInt32 uiNumBoxes = out_collection.m_Boxes.GetCount();
  const ezUInt32 uiNumImages = out_collection.m_Images.GetCount();

  out_collection.m_SortedShapes.Reserve(uiNumSpheres + uiNumBoxes + uiNumImages);

  ezUInt32 uiCurrentSphere = 0;
  ezUInt32 uiCurrentBox = 0;
  ezUInt32 uiCurrentImage = 0;

  while (uiCurrentSphere < uiNumSpheres || uiCurrentBox < uiNumBoxes || uiCurrentImage < uiNumImages)
  {
    Sphere* pSphere = uiCurrentSphere < uiNumSpheres ? &out_collection.m_Spheres[uiCurrentSphere] : nullptr;
    Box* pBox = uiCurrentBox < uiNumBoxes ? &out_collection.m_Boxes[uiCurrentBox] : nullptr;
    Image* pImage = uiCurrentImage < uiNumImages ? &out_collection.m_Images[uiCurrentImage] : nullptr;

    Shape* pSmallestShape = nullptr;
    ezUInt32 uiSmallestKey = 0xFFFFFFFF;

    if (pSphere && pSphere->m_uiSortingKey < uiSmallestKey)
    {
      pSmallestShape = pSphere;
      uiSmallestKey = pSmallestShape->m_uiSortingKey;
    }

    if (pBox && pBox->m_uiSortingKey < uiSmallestKey)
    {
      pSmallestShape = pBox;
      uiSmallestKey = pSmallestShape->m_uiSortingKey;
    }

    if (pImage && pImage->m_uiSortingKey < uiSmallestKey)
    {
      pSmallestShape = pImage;
      uiSmallestKey = pSmallestShape->m_uiSortingKey;
    }

    EZ_IGNORE_UNUSED(uiSmallestKey);
    EZ_ASSERT_DEBUG(pSmallestShape != nullptr, "Error sorting proc-gen volumes.");

    out_collection.m_SortedShapes.PushBack(pSmallestShape);

    if (pSmallestShape == pSphere)
    {
      ++uiCurrentSphere;
    }
    else if (pSmallestShape == pBox)
    {
      ++uiCurrentBox;
    }
    else if (pSmallestShape == pImage)
    {
      ++uiCurrentImage;
    }
  }
}

void ezVolumeCollection::AddSphere(const ezSimdTransform& transform, float fRadius, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder, float fValue, float fFalloff)
{
  ezSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale *= fRadius;

  auto& sphere = m_Spheres.ExpandAndGetRef();
  sphere.SetGlobalToLocalTransform(scaledTransform.GetAsMat4().GetInverse());
  sphere.m_Type = ShapeType::Sphere;
  sphere.m_BlendMode = blendMode;
  sphere.m_fValue = fValue;
  sphere.m_uiSortingKey = ezVolumeCollection::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  sphere.m_fFadeOutScale = -1.0f / ezMath::Max(fFalloff, 0.0001f);
  sphere.m_fFadeOutBias = -sphere.m_fFadeOutScale;
}

void ezVolumeCollection::AddBox(const ezSimdTransform& transform, const ezVec3& vExtents, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder,
  float fValue, const ezVec3& vFalloff)
{
  ezSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale = scaledTransform.m_Scale.CompMul(ezSimdConversion::ToVec3(vExtents)) * 0.5f;

  auto& box = m_Boxes.ExpandAndGetRef();
  box.SetGlobalToLocalTransform(scaledTransform.GetAsMat4().GetInverse());
  box.m_Type = ShapeType::Box;
  box.m_BlendMode = blendMode;
  box.m_fValue = fValue;
  box.m_uiSortingKey = ezVolumeCollection::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  box.m_vFadeOutScale = ezVec3(-1.0f).CompDiv(vFalloff.CompMax(ezVec3(0.0001f)));
  box.m_vFadeOutBias = -box.m_vFadeOutScale;
}

void ezVolumeCollection::AddImage(const ezSimdTransform& transform, const ezVec3& vExtents, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder, float fValue, const ezVec3& vFadeOutStart, const ezImageDataResourceHandle& hImage)
{
  ezSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale = scaledTransform.m_Scale.CompMul(ezSimdConversion::ToVec3(vExtents)) * 0.5f;

  auto& shape = m_Images.ExpandAndGetRef();
  shape.SetGlobalToLocalTransform(scaledTransform.GetAsMat4().GetInverse());
  shape.m_Type = ShapeType::Image;
  shape.m_BlendMode = blendMode;
  shape.m_fValue = fValue;
  shape.m_uiSortingKey = ezVolumeCollection::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  shape.m_vFadeOutScale = ezVec3(-1.0f).CompDiv((ezVec3(1.0f) - vFadeOutStart).CompMax(ezVec3(0.0001f)));
  shape.m_vFadeOutBias = -shape.m_vFadeOutScale;

  shape.m_Image = hImage;

  if (shape.m_Image.IsValid())
  {
    ezResourceLock<ezImageDataResource> pImage(shape.m_Image, ezResourceAcquireMode::BlockTillLoaded);
    shape.m_pPixelData = pImage->GetDescriptor().m_Image.GetPixelPointer<ezColor>();
    shape.m_uiImageWidth = pImage->GetDescriptor().m_Image.GetWidth();
    shape.m_uiImageHeight = pImage->GetDescriptor().m_Image.GetHeight();
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgExtractVolumes);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgExtractVolumes, 1, ezRTTIDefaultAllocator<ezMsgExtractVolumes>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
