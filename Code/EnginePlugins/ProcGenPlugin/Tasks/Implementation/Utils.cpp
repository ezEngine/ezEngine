#include <ProcGenPluginPCH.h>

#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>
#include <ProcGenPlugin/Tasks/Utils.h>

namespace
{
  ezSpatialData::Category s_ProcVolumeCategory = ezSpatialData::RegisterCategory("ProcVolume");
  static ezHashedString s_sVolumes = ezMakeHashedString("Volumes");
} // namespace

void ezProcGenExpressionFunctions::ApplyVolumes(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData)
{
  const ezVariantArray& volumes = globalData.GetValue(s_sVolumes)->Get<ezVariantArray>();
  if (volumes.IsEmpty())
    return;

  ezUInt32 uiIndex = 0;
  if (inputs.GetCount() > 4)
  {
    uiIndex = ezSimdVec4i::Truncate(inputs[4][0]).x();
  }

  auto pVolumeCollection = ezDynamicCast<const ezVolumeCollection*>(volumes[uiIndex].Get<ezReflectedClass*>());
  if (pVolumeCollection == nullptr)
    return;

  const ezSimdVec4f* pPosX = inputs[0].GetPtr();
  const ezSimdVec4f* pPosY = inputs[1].GetPtr();
  const ezSimdVec4f* pPosZ = inputs[2].GetPtr();
  const ezSimdVec4f* pPosXEnd = pPosX + inputs[0].GetCount();

  const ezSimdVec4f* pInitialValues = inputs[3].GetPtr();

  ezSimdVec4f* pOutput = output.GetPtr();

  while (pPosX < pPosXEnd)
  {
    pOutput->SetX(pVolumeCollection->EvaluateAtGlobalPosition(ezVec3(pPosX->x(), pPosY->x(), pPosZ->x()), pInitialValues->x()));
    pOutput->SetY(pVolumeCollection->EvaluateAtGlobalPosition(ezVec3(pPosX->y(), pPosY->y(), pPosZ->y()), pInitialValues->y()));
    pOutput->SetZ(pVolumeCollection->EvaluateAtGlobalPosition(ezVec3(pPosX->z(), pPosY->z(), pPosZ->z()), pInitialValues->z()));
    pOutput->SetW(pVolumeCollection->EvaluateAtGlobalPosition(ezVec3(pPosX->w(), pPosY->w(), pPosZ->w()), pInitialValues->w()));

    ++pPosX;
    ++pPosY;
    ++pPosZ;
    ++pInitialValues;
    ++pOutput;
  }
}

ezResult ezProcGenExpressionFunctions::ApplyVolumesValidate(const ezExpression::GlobalData& globalData)
{
  if (!globalData.IsEmpty())
  {
    if (const ezVariant* pValue = globalData.GetValue("Volumes"))
    {
      if (pValue->GetType() == ezVariantType::VariantArray)
      {
        return EZ_SUCCESS;
      }
    }
  }

  return EZ_FAILURE;
}

//////////////////////////////////////////////////////////////////////////

void ezProcGenInternal::ExtractVolumeCollections(const ezWorld& world, const ezBoundingBox& box, const Output& output,
  ezDynamicArray<ezVolumeCollection>& volumeCollections, ezExpression::GlobalData& globalData)
{
  auto& volumeTagSetIndices = output.m_VolumeTagSetIndices;
  if (volumeTagSetIndices.IsEmpty())
    return;

  ezVariantArray volumes;
  if (ezVariant* volumesVar = globalData.GetValue(s_sVolumes))
  {
    volumes = volumesVar->Get<ezVariantArray>();
  }

  for (ezUInt8 tagSetIndex : volumeTagSetIndices)
  {
    if (tagSetIndex < volumes.GetCount() && volumes[tagSetIndex].IsValid())
    {
      continue;
    }

    auto pGraphSharedData = static_cast<const ezProcGenInternal::GraphSharedData*>(output.m_pGraphSharedData.Borrow());
    auto& includeTags = pGraphSharedData->GetTagSet(tagSetIndex);

    auto& volumeCollection = volumeCollections.ExpandAndGetRef();
    ezVolumeCollection::ExtractVolumesInBox(world, box, s_ProcVolumeCategory, includeTags, volumeCollection, ezGetStaticRTTI<ezProcVolumeComponent>());

    volumes.EnsureCount(tagSetIndex + 1);
    volumes[tagSetIndex] = ezVariant(&volumeCollection);
  }

  globalData.Insert(s_sVolumes, volumes);
}
