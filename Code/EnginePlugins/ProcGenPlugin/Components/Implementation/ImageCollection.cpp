#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <ProcGenPlugin/Components/ImageCollection.h>

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
    if ((absLocalPos <= ezSimdVec4f(1.0f)).AllSet<3>())
    {
      // TODO
      fValue = shape.m_fValue;
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

void ezImageCollection::AddShape(const ezSimdTransform& transform, const ezVec3& vExtents, float fSortOrder, float fValue)
{
  ezSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale = scaledTransform.m_Scale.CompMul(ezSimdConversion::ToVec3(vExtents)) * 0.5f;

  auto& box = m_Shapes.ExpandAndGetRef();
  box.SetGlobalToLocalTransform(scaledTransform.GetAsMat4().GetInverse());
  box.m_fValue = fValue;
  box.m_uiSortingKey = ezImageCollection::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
}

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgExtractProcImages);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgExtractProcImages, 1, ezRTTIDefaultAllocator<ezMsgExtractProcImages>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
