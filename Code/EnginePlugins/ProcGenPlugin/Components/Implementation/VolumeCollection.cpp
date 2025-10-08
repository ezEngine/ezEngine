#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <GameEngine/Utils/ImageDataResource.h>
#include <GameEngine/Volumes/VolumeSampler.h>
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

static_assert(sizeof(ezVolumeCollection::Sphere) == 60);
static_assert(sizeof(ezVolumeCollection::Box) == 80);

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

ezVolumeCollection::ezVolumeCollection()
  : m_Allocator("VolumeCollection", ezFoundation::GetAlignedAllocator())
{
}

ezVolumeCollection::~ezVolumeCollection() = default;

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
        const float fAlpha = ezMath::Saturate(ezMath::Sqrt(distSquared) * sphere.m_fFadeOut - sphere.m_fFadeOut);
        fValue = ezMath::Lerp(fValue, fNewValue, fAlpha);
      }
    }
    else if (pShape->m_Type == ShapeType::Box)
    {
      auto& box = *static_cast<const Box*>(pShape);
      const ezSimdVec4f localPos = box.GetGlobalToLocalTransform().TransformPosition(vPosition);
      const ezSimdVec4f absLocalPos = localPos.Abs();
      if ((absLocalPos <= ezSimdVec4f(1.0f)).AllSet<3>())
      {
        const ezSimdVec4f fadeOut = ezSimdVec4f::Select(localPos > ezSimdVec4f::MakeZero(), ezSimdConversion::ToVec3(box.m_vPositiveFadeOut), ezSimdConversion::ToVec3(box.m_vNegativeFadeOut));
        ezSimdVec4f vAlpha = absLocalPos.CompMul(fadeOut) - fadeOut;
        vAlpha = vAlpha.CompMin(ezSimdVec4f(1.0f)).CompMax(ezSimdVec4f::MakeZero());
        const float fAlpha = vAlpha.x() * vAlpha.y() * vAlpha.z();

        const float fNewValue = ApplyValue(box.m_BlendMode, fValue, box.m_fValue);
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
          const ezSimdVec4f fadeOut = ezSimdVec4f::Select(localPos > ezSimdVec4f::MakeZero(), ezSimdConversion::ToVec3(image.m_vPositiveFadeOut), ezSimdConversion::ToVec3(image.m_vNegativeFadeOut));
          ezSimdVec4f vAlpha = absLocalPos.CompMul(fadeOut) - fadeOut;
          vAlpha = vAlpha.CompMin(ezSimdVec4f(1.0f)).CompMax(ezSimdVec4f::MakeZero());
          const float fAlpha = vAlpha.x() * vAlpha.y() * vAlpha.z();

          const float fNewValue = ApplyValue(image.m_BlendMode, fValue, image.m_fValue);
          fValue = ezMath::Lerp(fValue, fNewValue, fAlpha);
        }
      }
    }
    else if (pShape->m_Type == ShapeType::Spline)
    {
      auto& spline = *static_cast<const Spline*>(pShape);
      const ezSimdVec4f localPos = spline.GetGlobalToLocalTransform().TransformPosition(vPosition);
      const ezSimdBBox localBox = ezSimdConversion::ToBBox(spline.m_BoundingBox);
      if (!localBox.Contains(localPos))
        continue;

      float fT = 0.0f;
      float fDistanceSquared = 0.0f;
      const ezSimdVec4f pointOnSpline = spline.m_Spline.FindClosestPoint(localPos, fT, fDistanceSquared, spline.m_fMaxError);
      const ezSimdMat4f t = spline.m_Spline.EvaluateTransform(fT).GetAsMat4().GetInverse();
      const float fNormalizedDistance = float(t.TransformPosition(localPos).GetLength<3>()) * spline.m_fInvRadius;
      if (fNormalizedDistance <= 1.0f)
      {
        const float fNewValue = ApplyValue(spline.m_BlendMode, fValue, spline.m_fValue);
        const float fAlpha = ezMath::Saturate(fNormalizedDistance * spline.m_fFadeOut - spline.m_fFadeOut);
        fValue = ezMath::Lerp(fValue, fNewValue, fAlpha);
      }
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
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
  queryParams.m_pIncludeTags = &includeTags;

  world.GetSpatialSystem()->FindObjectsInBox(box, queryParams,
    [&](ezGameObject* pObject)
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

      return ezVisitorExecution::Continue;
    });

  struct Sorter
  {
    bool Less(const Shape* lhs, const Shape* rhs) const
    {
      return *lhs < *rhs;
    }
  };

  out_collection.m_SortedShapes.Sort(Sorter());
}

void ezVolumeCollection::AddSphere(const ezSimdTransform& transform, float fRadius, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder, float fValue, float fFalloff)
{
  ezSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale *= fRadius;

  Sphere* pSphere = EZ_NEW(&m_Allocator, Sphere);
  pSphere->SetGlobalToLocalTransform(scaledTransform.GetAsMat4().GetInverse());
  pSphere->m_Type = ShapeType::Sphere;
  pSphere->m_BlendMode = blendMode;
  pSphere->m_fValue = fValue;
  pSphere->m_uiSortingKey = ezVolumeSampler::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  pSphere->m_fFadeOut = -1.0f / ezMath::Max(fFalloff, 0.0001f);

  m_SortedShapes.PushBack(pSphere);
}

void ezVolumeCollection::AddBox(const ezSimdTransform& transform, const ezVec3& vExtents, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder, float fValue, const ezVec3& vPositiveFalloff, const ezVec3& vNegativeFalloff, const ezImageDataResourceHandle& hImage)
{
  const bool bHasImage = hImage.IsValid();

  ezSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale = scaledTransform.m_Scale.CompMul(ezSimdConversion::ToVec3(vExtents)) * 0.5f;

  Box* pBox = nullptr;
  if (bHasImage)
  {
    Image* pImage = EZ_NEW(&m_Allocator, Image);
    pImage->m_Type = ShapeType::Image;
    pImage->m_hImage = hImage;

    ezResourceLock<ezImageDataResource> pImageData(hImage, ezResourceAcquireMode::BlockTillLoaded);
    auto& image = pImageData->GetDescriptor().m_Image;
    pImage->m_pPixelData = image.GetPixelPointer<ezColor>();
    pImage->m_uiImageWidth = image.GetWidth();
    pImage->m_uiImageHeight = image.GetHeight();

    pBox = pImage;
  }
  else
  {
    pBox = EZ_NEW(&m_Allocator, Box);
    pBox->m_Type = ShapeType::Box;
  }

  pBox->SetGlobalToLocalTransform(scaledTransform.GetAsMat4().GetInverse());
  pBox->m_BlendMode = blendMode;
  pBox->m_fValue = fValue;
  pBox->m_uiSortingKey = ezVolumeSampler::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  pBox->m_vPositiveFadeOut = ezVec3(-1.0f).CompDiv(vPositiveFalloff.CompMax(ezVec3(0.0001f)));
  pBox->m_vNegativeFadeOut = ezVec3(-1.0f).CompDiv(vNegativeFalloff.CompMax(ezVec3(0.0001f)));

  m_SortedShapes.PushBack(pBox);
}

void ezVolumeCollection::AddSpline(const ezSimdTransform& transform, const ezSpline& spline, float fRadius, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder, float fValue, float fFalloff)
{
  Spline* pSpline = EZ_NEW(&m_Allocator, Spline);
  pSpline->SetGlobalToLocalTransform(transform.GetAsMat4().GetInverse());
  pSpline->m_Type = ShapeType::Spline;
  pSpline->m_BlendMode = blendMode;
  pSpline->m_fValue = fValue;
  pSpline->m_uiSortingKey = ezVolumeSampler::ComputeSortingKey(fSortOrder, transform.GetMaxScale());

  ezSimdBBoxSphere bounds;
  spline.CalculateBounds(bounds).IgnoreResult();
  ezSimdBBox box = bounds.GetBox();
  box.m_Min -= ezSimdVec4f(fRadius);
  box.m_Max += ezSimdVec4f(fRadius);

  pSpline->m_Spline = spline;
  pSpline->m_BoundingBox = ezSimdConversion::ToBBox(box);
  pSpline->m_fInvRadius = 1.0f / ezMath::Max(fRadius, 0.0001f);
  pSpline->m_fFadeOut = -1.0f / ezMath::Max(fFalloff, 0.0001f);
  pSpline->m_fMaxError = ezMath::Max(fRadius / 5.0f, 0.1f);

  m_SortedShapes.PushBack(pSpline);
}

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgExtractVolumes);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgExtractVolumes, 1, ezRTTIDefaultAllocator<ezMsgExtractVolumes>)
EZ_END_DYNAMIC_REFLECTED_TYPE;


EZ_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Components_Implementation_VolumeCollection);
