#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/SimdMath/SimdVec4i.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>
#include <ProcGenPlugin/Tasks/Utils.h>

namespace
{
  ezSpatialData::Category s_ProcVolumeCategory = ezSpatialData::RegisterCategory("ProcVolume", ezSpatialData::Flags::None);
  static ezHashedString s_sVolumes = ezMakeHashedString("Volumes");
} // namespace

void ezProcGenExpressionFunctions::ApplyVolumes(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData)
{
  const ezVariantArray& volumes = globalData.GetValue(s_sVolumes)->Get<ezVariantArray>();
  if (volumes.IsEmpty())
    return;

  ezUInt32 uiTagSetIndex = 0;
  if (inputs.GetCount() > 4)
  {
    uiTagSetIndex = ezSimdVec4i::Truncate(inputs[4][0]).x();
  }

  auto pVolumeCollection = ezDynamicCast<const ezVolumeCollection*>(volumes[uiTagSetIndex].Get<ezReflectedClass*>());
  if (pVolumeCollection == nullptr)
    return;

  const ezSimdVec4f* pPosX = inputs[0].GetPtr();
  const ezSimdVec4f* pPosY = inputs[1].GetPtr();
  const ezSimdVec4f* pPosZ = inputs[2].GetPtr();
  const ezSimdVec4f* pPosXEnd = pPosX + inputs[0].GetCount();

  const ezSimdVec4f* pInitialValues = inputs[3].GetPtr();

  const ezSimdVec4f* pImgMode = inputs[5].GetPtr();
  const ezSimdVec4f* pRefColR = inputs[6].GetPtr();
  const ezSimdVec4f* pRefColG = inputs[7].GetPtr();
  const ezSimdVec4f* pRefColB = inputs[8].GetPtr();
  const ezSimdVec4f* pRefColA = inputs[9].GetPtr();

  const ezProcVolumeImageMode::Enum imgMode = static_cast<ezProcVolumeImageMode::Enum>((float)pImgMode->x());
  const ezColor refCol = ezColor(pRefColR->x(), pRefColG->x(), pRefColB->x(), pRefColA->x());

  ezSimdVec4f* pOutput = output.GetPtr();

  while (pPosX < pPosXEnd)
  {
    pOutput->SetX(pVolumeCollection->EvaluateAtGlobalPosition(ezVec3(pPosX->x(), pPosY->x(), pPosZ->x()), pInitialValues->x(), imgMode, refCol));
    pOutput->SetY(pVolumeCollection->EvaluateAtGlobalPosition(ezVec3(pPosX->y(), pPosY->y(), pPosZ->y()), pInitialValues->y(), imgMode, refCol));
    pOutput->SetZ(pVolumeCollection->EvaluateAtGlobalPosition(ezVec3(pPosX->z(), pPosY->z(), pPosZ->z()), pInitialValues->z(), imgMode, refCol));
    pOutput->SetW(pVolumeCollection->EvaluateAtGlobalPosition(ezVec3(pPosX->w(), pPosY->w(), pPosZ->w()), pInitialValues->w(), imgMode, refCol));

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

void ezProcGenInternal::ExtractVolumeCollections(const ezWorld& world, const ezBoundingBox& box, const Output& output, ezDeque<ezVolumeCollection>& volumeCollections, ezExpression::GlobalData& globalData)
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
