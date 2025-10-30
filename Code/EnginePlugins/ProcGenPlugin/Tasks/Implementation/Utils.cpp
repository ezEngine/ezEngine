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
    const ezExpression::Register* pPosXEnd = inputs[0].GetEndPtr();

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
      helperMat.SetRows(pPosX->f, pPosY->f, pPosZ->f, ezSimdVec4f::MakeZero());

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

  //////////////////////////////////////////////////////////////////////////

  static ezHashedString s_sInstanceSeed = ezMakeHashedString("InstanceSeed");

  static void GetInstanceSeed(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData)
  {
    int instanceSeed = globalData.GetValue(s_sInstanceSeed)->Get<int>();

    ezExpression::Register* pOutput = output.GetPtr();
    ezExpression::Register* pOutputEnd = output.GetEndPtr();

    while (pOutput < pOutputEnd)
    {
      pOutput->i.Set(instanceSeed);

      ++pOutput;
    }
  }

  static ezResult GetInstanceSeedValidate(const ezExpression::GlobalData& globalData)
  {
    if (!globalData.IsEmpty())
    {
      if (const ezVariant* pValue = globalData.GetValue(s_sInstanceSeed))
      {
        if (pValue->GetType() == ezVariantType::Int32)
        {
          return EZ_SUCCESS;
        }
      }
    }

    return EZ_FAILURE;
  }

  //////////////////////////////////////////////////////////////////////////

  static ezHashedString s_sCurves = ezMakeHashedString("Curves");

  static const ezEnum<ezExpression::RegisterType> s_SampleCurvesTypes[] = {
    ezExpression::RegisterType::Float, // X
    ezExpression::RegisterType::Int,   // CurveIndex
  };

  static void SampleCurve(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData)
  {
    const ezVariantArray& curves = globalData.GetValue(s_sCurves)->Get<ezVariantArray>();
    if (curves.IsEmpty())
      return;

    ezUInt32 uiCurveIndex = inputs[1].GetPtr()->i.x();
    auto pCurveData = ezDynamicCast<const ezProcGenInternal::CurveData*>(curves[uiCurveIndex].Get<ezReflectedClass*>());
    if (pCurveData == nullptr)
      return;

    const ezUInt32 uiMaxIdx = pCurveData->m_Samples.GetCount() - 1;
    const ezSimdVec4f vOffsetX = ezSimdVec4f(-pCurveData->m_fMinX);
    const ezSimdVec4f vScale = ezSimdVec4f(static_cast<float>(uiMaxIdx) / (pCurveData->m_fMaxX - pCurveData->m_fMinX));
    const ezSimdVec4i vMaxIdx = ezSimdVec4i(uiMaxIdx);
    const float* samples = pCurveData->m_Samples.GetData();

    const ezExpression::Register* pX = inputs[0].GetPtr();
    const ezExpression::Register* pXEnd = inputs[0].GetEndPtr();
    ezExpression::Register* pOutput = output.GetPtr();

    while (pX < pXEnd)
    {
      const ezSimdVec4f vT = (pX->f + vOffsetX).CompMul(vScale);
      const ezSimdVec4i vIdx0 = ezSimdVec4i::Truncate(vT).CompMax(ezSimdVec4i::MakeZero()).CompMin(vMaxIdx);
      const ezSimdVec4i vIdx1 = (vIdx0 + ezSimdVec4i(1)).CompMin(vMaxIdx);
      const ezSimdVec4f vFrac = vT - vIdx0.ToFloat();

      const float sample0[] = {samples[vIdx0.x()], samples[vIdx0.y()], samples[vIdx0.z()], samples[vIdx0.w()]};
      const float sample1[] = {samples[vIdx1.x()], samples[vIdx1.y()], samples[vIdx1.z()], samples[vIdx1.w()]};
      ezSimdVec4f vSample0, vSample1;
      vSample0.Load<4>(sample0);
      vSample1.Load<4>(sample1);

      const ezSimdVec4f vResult = ezSimdVec4f::Lerp(vSample0, vSample1, vFrac);
      pOutput->f = vResult;

      ++pX;
      ++pOutput;
    }
  }

  static ezResult SampleCurveValidate(const ezExpression::GlobalData& globalData)
  {
    if (!globalData.IsEmpty())
    {
      if (const ezVariant* pValue = globalData.GetValue(s_sCurves))
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
  {ezMakeHashedString("ApplyVolumes"), ezExpression::FunctionDesc::TypeList(s_ApplyVolumesTypes), 5, ezExpression::RegisterType::Float},
  &ApplyVolumes,
  &ApplyVolumesValidate,
};

ezExpressionFunction ezProcGenExpressionFunctions::s_GetInstanceSeedFunc = {
  {ezMakeHashedString("GetInstanceSeed"), ezExpression::FunctionDesc::TypeList(), 0, ezExpression::RegisterType::Int},
  &GetInstanceSeed,
  &GetInstanceSeedValidate,
};

ezExpressionFunction ezProcGenExpressionFunctions::s_SampleCurveFunc = {
  {ezMakeHashedString("SampleCurve"), ezExpression::FunctionDesc::TypeList(s_SampleCurvesTypes), 2, ezExpression::RegisterType::Float},
  &SampleCurve,
  &SampleCurveValidate,
};

//////////////////////////////////////////////////////////////////////////

// static
void ezProcGenGlobalData::ExtractVolumeCollections(const ezWorld& world, const ezBoundingBox& box, const ezProcGenInternal::Output& output, ezDeque<ezVolumeCollection>& ref_volumeCollections, ezExpression::GlobalData& ref_globalData)
{
  auto& volumeTagSetIndices = output.m_VolumeTagSetIndices;
  if (volumeTagSetIndices.IsEmpty())
    return;

  ezVariantArray volumes;
  if (ezVariant* volumesVar = ref_globalData.GetValue(s_sVolumes))
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

    auto& volumeCollection = ref_volumeCollections.ExpandAndGetRef();
    ezVolumeCollection::ExtractVolumesInBox(world, box, s_ProcVolumeCategory, includeTags, volumeCollection, ezGetStaticRTTI<ezProcVolumeComponent>());

    volumes.EnsureCount(tagSetIndex + 1);
    volumes[tagSetIndex] = ezVariant(&volumeCollection);
  }

  ref_globalData.Insert(s_sVolumes, volumes);
}

// static
void ezProcGenGlobalData::SetInstanceSeed(ezUInt32 uiSeed, ezExpression::GlobalData& ref_globalData)
{
  ref_globalData.Insert(s_sInstanceSeed, (int)uiSeed);
}

// static
void ezProcGenGlobalData::SetCurves(const ezProcGenInternal::Output& output, ezExpression::GlobalData& ref_globalData)
{
  auto& curveIndices = output.m_CurveIndices;
  if (curveIndices.IsEmpty())
    return;

  ezVariantArray curves;
  if (ezVariant* curvesVar = ref_globalData.GetValue(s_sCurves))
  {
    curves = curvesVar->Get<ezVariantArray>();
  }

  for (ezUInt8 curveIndex : curveIndices)
  {
    if (curveIndex < curves.GetCount() && curves[curveIndex].IsValid())
    {
      continue;
    }

    auto pGraphSharedData = static_cast<const ezProcGenInternal::GraphSharedData*>(output.m_pGraphSharedData.Borrow());
    auto& curveData = pGraphSharedData->GetCurve(curveIndex);

    curves.EnsureCount(curveIndex + 1);
    curves[curveIndex] = ezVariant(&curveData);
  }

  ref_globalData.Insert(s_sCurves, curves);
}
