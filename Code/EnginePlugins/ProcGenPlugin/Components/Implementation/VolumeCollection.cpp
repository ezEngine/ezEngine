#include <ProcGenPluginPCH.h>

#include <ProcGenPlugin/Components/VolumeCollection.h>

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgExtractVolumes);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgExtractVolumes, 1, ezRTTIDefaultAllocator<ezMsgExtractVolumes>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

float ezVolumeCollection::EvaluateAtGlobalPosition(const ezVec3& vPosition, float fInitialValue /*= 0.0f*/) const
{
  return fInitialValue;
}

//static
void ezVolumeCollection::FindObjectsInBox(const ezWorld& world, const ezBoundingBox& box, ezSpatialData::Category spatialCategory,
  const ezTagSet& includeTags, ezVolumeCollection& out_Collection)
{
  ezMsgExtractVolumes msg;
  msg.m_pCollection = &out_Collection;

  world.GetSpatialSystem().FindObjectsInBox(box, spatialCategory.GetBitmask(), [&](ezGameObject* pObject) {
    if (includeTags.IsEmpty() || includeTags.IsAnySet(pObject->GetTags()))
    {
      pObject->SendMessage(msg);
    }
    return ezVisitorExecution::Continue;
  });
}
