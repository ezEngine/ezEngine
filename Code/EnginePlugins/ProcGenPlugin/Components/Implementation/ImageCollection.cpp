#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <GameEngine/Utils/ImageDataResource.h>
#include <ProcGenPlugin/Components/ImageCollection.h>
#include <Texture/Image/ImageUtils.h>

void ezImageCollection::Shape::SetGlobalToLocalTransform(const ezSimdMat4f& t)
{
  ezSimdVec4f r0, r1, r2, r3;
  t.GetRows(r0, r1, r2, r3);

  m_GlobalToLocalTransform0 = ezSimdConversion::ToVec4(r0);
  m_GlobalToLocalTransform1 = ezSimdConversion::ToVec4(r1);
  m_GlobalToLocalTransform2 = ezSimdConversion::ToVec4(r2);
}

ezSimdMat4f ezImageCollection::Shape::GetGlobalToLocalTransform() const
{
  ezSimdMat4f m;
  m.SetRows(ezSimdConversion::ToVec4(m_GlobalToLocalTransform0), ezSimdConversion::ToVec4(m_GlobalToLocalTransform1),
    ezSimdConversion::ToVec4(m_GlobalToLocalTransform2), ezSimdVec4f(0, 0, 0, 1));

  return m;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezImageCollection, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

// static
ezUInt32 ezImageCollection::ComputeSortingKey(float fSortOrder, float fMaxScale)
{
  ezUInt32 uiSortingKey = (ezUInt32)(ezMath::Min(fSortOrder * 512.0f, 32767.0f) + 32768.0f);
  uiSortingKey = (uiSortingKey << 16) | (0xFFFF - ((ezUInt32)(fMaxScale * 100.0f) & 0xFFFF));
  return uiSortingKey;
}

float ezImageCollection::EvaluateAtGlobalPosition(const ezVec3& vPosition, float fInitialValue /*= 0.0f*/) const
{
  ezSimdVec4f globalPos = ezSimdConversion::ToVec3(vPosition);
  float fValue = fInitialValue;

  for (const auto& shape : m_Shapes)
  {
    const ezSimdVec4f absLocalPos = shape.GetGlobalToLocalTransform().TransformPosition(globalPos).Abs();
    if ((absLocalPos <= ezSimdVec4f(1.0f)).AllSet<3>() && shape.m_pPixelData != nullptr)
    {
      ezVec2 uv;
      uv.x = static_cast<float>(absLocalPos.x()) * 0.5f + 0.5f;
      uv.y = static_cast<float>(absLocalPos.y()) * 0.5f + 0.5f;

      ezColor c = ezImageUtils::NearestSample(shape.m_pPixelData, shape.m_uiImageWidth, shape.m_uiImageHeight, ezImageAddressMode::Clamp, uv);

      // TODO
      fValue = c.r;
    }
  }

  return fValue;
}

// static
void ezImageCollection::ExtractImagesInBox(const ezWorld& world, const ezBoundingBox& box, ezSpatialData::Category spatialCategory,
  const ezTagSet& includeTags, ezImageCollection& out_Collection, const ezRTTI* pComponentBaseType)
{
  ezMsgExtractProcImages msg;
  msg.m_pCollection = &out_Collection;

  ezSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = spatialCategory.GetBitmask();
  queryParams.m_IncludeTags = includeTags;

  world.GetSpatialSystem()->FindObjectsInBox(box, queryParams, [&](ezGameObject* pObject)
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

  out_Collection.m_Shapes.Sort();
}

void ezImageCollection::AddShape(const ezSimdTransform& transform, const ezVec3& vExtents, float fSortOrder, const ezImageDataResourceHandle& image)
{
  ezSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale = scaledTransform.m_Scale.CompMul(ezSimdConversion::ToVec3(vExtents)) * 0.5f;

  auto& shape = m_Shapes.ExpandAndGetRef();
  shape.SetGlobalToLocalTransform(scaledTransform.GetAsMat4().GetInverse());
  shape.m_uiSortingKey = ezImageCollection::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  shape.m_Image = image;

  if (shape.m_Image.IsValid())
  {
    ezResourceLock<ezImageDataResource> pImage(shape.m_Image, ezResourceAcquireMode::BlockTillLoaded);
    shape.m_pPixelData = pImage->GetDescriptor().m_Image.GetPixelPointer<ezColor>();
    shape.m_uiImageWidth = pImage->GetDescriptor().m_Image.GetWidth();
    shape.m_uiImageHeight = pImage->GetDescriptor().m_Image.GetHeight();
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgExtractProcImages);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgExtractProcImages, 1, ezRTTIDefaultAllocator<ezMsgExtractProcImages>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
