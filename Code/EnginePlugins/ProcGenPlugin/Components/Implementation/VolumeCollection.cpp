#include <ProcGenPluginPCH.h>

#include <ProcGenPlugin/Components/VolumeCollection.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVolumeCollection, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

float ezVolumeCollection::EvaluateAtGlobalPosition(const ezVec3& vPosition, float fInitialValue /*= 0.0f*/) const
{
  ezSimdVec4f globalPos = ezSimdConversion::ToVec3(vPosition);
  float fValue = fInitialValue;

  for (auto& sphere : m_Spheres)
  {
    ezSimdVec4f localPos = sphere.m_GlobalToLocalTransform.TransformPosition(globalPos);
    if (localPos.GetLengthSquared<3>() <= 1.0f)
    {
      fValue = 0.0f;
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

  world.GetSpatialSystem().FindObjectsInBox(box, spatialCategory.GetBitmask(), [&](ezGameObject* pObject) {
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
}

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgExtractVolumes);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgExtractVolumes, 1, ezRTTIDefaultAllocator<ezMsgExtractVolumes>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
