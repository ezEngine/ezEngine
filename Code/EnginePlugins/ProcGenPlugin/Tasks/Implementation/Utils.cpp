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

  static const ezEnum<ezExpression::RegisterType> s_ApplyVolumesTypes[] = {
    ezExpression::RegisterType::Float, // PosX
    ezExpression::RegisterType::Float, // PosY
    ezExpression::RegisterType::Float, // PosZ
    ezExpression::RegisterType::Float, // InitialValue
    ezExpression::RegisterType::Int,   // TagSetIndex
    ezExpression::RegisterType::Int,   // ImageMode
    ezExpression::RegisterType::Float, // RefColorR
    ezExpression::RegisterType::Float, // RefColorG
    ezExpression::RegisterType::Float, // RefColorB
    ezExpression::RegisterType::Float, // RefColorA
  };

  static void ApplyVolumes(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData)
  {
    const ezVariantArray& volumes = globalData.GetValue(s_sVolumes)->Get<ezVariantArray>();
    if (volumes.IsEmpty())
      return;

    ezUInt32 uiTagSetIndex = inputs[4].GetPtr()->i.x();
    auto pVolumeCollection = ezDynamicCast<const ezVolumeCollection*>(volumes[uiTagSetIndex].Get<ezReflectedClass*>());
    if (pVolumeCollection == nullptr)
      return;

    const ezExpression::Register* pPosX = inputs[0].GetPtr();
    const ezExpression::Register* pPosY = inputs[1].GetPtr();
    const ezExpression::Register* pPosZ = inputs[2].GetPtr();
    const ezExpression::Register* pPosXEnd = pPosX + inputs[0].GetCount();

    const ezExpression::Register* pInitialValues = inputs[3].GetPtr();

    ezProcVolumeImageMode::Enum imgMode = ezProcVolumeImageMode::Default;
    ezColor refColor = ezColor::White;
    if (inputs.GetCount() >= 10)
    {
      imgMode = static_cast<ezProcVolumeImageMode::Enum>(inputs[5].GetPtr()->i.x());

      const float refColR = inputs[6].GetPtr()->f.x();
      const float refColG = inputs[7].GetPtr()->f.x();
      const float refColB = inputs[8].GetPtr()->f.x();
      const float refColA = inputs[9].GetPtr()->f.x();      
      refColor = ezColor(refColR, refColG, refColB, refColA);
    }

    ezExpression::Register* pOutput = output.GetPtr();

    ezSimdMat4f helperMat;
    while (pPosX < pPosXEnd)
    {
      helperMat.SetRows(pPosX->f, pPosY->f, pPosZ->f, ezSimdVec4f::ZeroVector());

      const float x = pVolumeCollection->EvaluateAtGlobalPosition(helperMat.m_col0, pInitialValues->f.x(), imgMode, refColor);
      const float y = pVolumeCollection->EvaluateAtGlobalPosition(helperMat.m_col1, pInitialValues->f.y(), imgMode, refColor);
      const float z = pVolumeCollection->EvaluateAtGlobalPosition(helperMat.m_col2, pInitialValues->f.z(), imgMode, refColor);
      const float w = pVolumeCollection->EvaluateAtGlobalPosition(helperMat.m_col3, pInitialValues->f.w(), imgMode, refColor);
      pOutput->f.Set(x, y, z, w);

      ++pPosX;
      ++pPosY;
      ++pPosZ;
      ++pInitialValues;
      ++pOutput;
    }
  }

  static ezResult ApplyVolumesValidate(const ezExpression::GlobalData& globalData)
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
} // namespace

ezExpressionFunction ezProcGenExpressionFunctions::s_ApplyVolumesFunc = {
  {ezMakeHashedString("ApplyVolumes"), ezMakeArrayPtr(s_ApplyVolumesTypes), 5, ezExpression::RegisterType::Float},
  &ApplyVolumes,
  &ApplyVolumesValidate,
};

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
