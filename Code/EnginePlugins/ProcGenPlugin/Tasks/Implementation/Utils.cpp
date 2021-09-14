#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/SimdMath/SimdVec4i.h>
#include <ProcGenPlugin/Components/ImageCollection.h>
#include <ProcGenPlugin/Components/ProcImageComponent.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>
#include <ProcGenPlugin/Tasks/Utils.h>

namespace
{
  ezSpatialData::Category s_ProcVolumeCategory = ezSpatialData::RegisterCategory("ProcVolume", ezSpatialData::Flags::None);
  ezSpatialData::Category s_ProcImageCategory = ezSpatialData::RegisterCategory("ProcImage", ezSpatialData::Flags::None);
  static ezHashedString s_sVolumes = ezMakeHashedString("Volumes");
  static ezHashedString s_sImages = ezMakeHashedString("Images");
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

void ezProcGenExpressionFunctions::SampleImages(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData)
{
  const ezVariantArray& images = globalData.GetValue(s_sImages)->Get<ezVariantArray>();
  if (images.IsEmpty())
    return;

  ezUInt32 uiTagSetIndex = 0;
  if (inputs.GetCount() > 4)
  {
    uiTagSetIndex = ezSimdVec4i::Truncate(inputs[4][0]).x();
  }

  auto pCollection = ezDynamicCast<const ezImageCollection*>(images[uiTagSetIndex].Get<ezReflectedClass*>());
  if (pCollection == nullptr)
    return;

  const ezSimdVec4f* pPosX = inputs[0].GetPtr();
  const ezSimdVec4f* pPosY = inputs[1].GetPtr();
  const ezSimdVec4f* pPosZ = inputs[2].GetPtr();
  const ezSimdVec4f* pPosXEnd = pPosX + inputs[0].GetCount();

  const ezSimdVec4f* pInitialValues = inputs[3].GetPtr();

  const ezSimdVec4f* pRefColR = inputs[5].GetPtr();
  const ezSimdVec4f* pRefColG = inputs[6].GetPtr();
  const ezSimdVec4f* pRefColB = inputs[7].GetPtr();
  const ezSimdVec4f* pRefColA = inputs[8].GetPtr();

  ezSimdVec4f* pOutput = output.GetPtr();

  while (pPosX < pPosXEnd)
  {
    pOutput->SetX(pCollection->EvaluateAtGlobalPosition(ezVec3(pPosX->x(), pPosY->x(), pPosZ->x()), pInitialValues->x(), ezColor(pRefColR->x(), pRefColG->x(), pRefColB->x(), pRefColA->x())));
    pOutput->SetY(pCollection->EvaluateAtGlobalPosition(ezVec3(pPosX->y(), pPosY->y(), pPosZ->y()), pInitialValues->y(), ezColor(pRefColR->y(), pRefColG->y(), pRefColB->y(), pRefColA->y())));
    pOutput->SetZ(pCollection->EvaluateAtGlobalPosition(ezVec3(pPosX->z(), pPosY->z(), pPosZ->z()), pInitialValues->z(), ezColor(pRefColR->z(), pRefColG->z(), pRefColB->z(), pRefColA->z())));
    pOutput->SetW(pCollection->EvaluateAtGlobalPosition(ezVec3(pPosX->w(), pPosY->w(), pPosZ->w()), pInitialValues->w(), ezColor(pRefColR->w(), pRefColG->w(), pRefColB->w(), pRefColA->w())));

    ++pPosX;
    ++pPosY;
    ++pPosZ;
    ++pInitialValues;
    ++pOutput;
  }
}

ezResult ezProcGenExpressionFunctions::SampleImagesValidate(const ezExpression::GlobalData& globalData)
{
  if (!globalData.IsEmpty())
  {
    if (const ezVariant* pValue = globalData.GetValue("Images"))
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

void ezProcGenInternal::ExtractImageCollections(const ezWorld& world, const ezBoundingBox& box, const Output& output, ezDeque<ezImageCollection>& imageCollections, ezExpression::GlobalData& globalData)
{
  auto& tagSetIndices = output.m_ImageTagSetIndices;
  if (tagSetIndices.IsEmpty())
    return;

  ezVariantArray images;
  if (ezVariant* var = globalData.GetValue(s_sImages))
  {
    images = var->Get<ezVariantArray>();
  }

  for (ezUInt8 tagSetIndex : tagSetIndices)
  {
    if (tagSetIndex < images.GetCount() && images[tagSetIndex].IsValid())
    {
      continue;
    }

    auto pGraphSharedData = static_cast<const ezProcGenInternal::GraphSharedData*>(output.m_pGraphSharedData.Borrow());
    auto& includeTags = pGraphSharedData->GetTagSet(tagSetIndex);

    auto& collection = imageCollections.ExpandAndGetRef();
    ezImageCollection::ExtractImagesInBox(world, box, s_ProcImageCategory, includeTags, collection, ezGetStaticRTTI<ezProcImageComponent>());

    images.EnsureCount(tagSetIndex + 1);
    images[tagSetIndex] = ezVariant(&collection);
  }

  globalData.Insert(s_sImages, images);
}
