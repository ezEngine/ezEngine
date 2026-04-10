#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <Foundation/SimdMath/SimdNoise.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Tracks/Curve1D.h>

using namespace ezExpression;

namespace
{
  static const char* s_szRegisterTypeNames[] = {
    "Unknown",
    "Bool",
    "Int",
    "Float",
  };

  static_assert(EZ_ARRAY_SIZE(s_szRegisterTypeNames) == RegisterType::Count);

  static const char* s_szRegisterTypeNamesShort[] = {
    "U",
    "B",
    "I",
    "F",
  };

  static_assert(EZ_ARRAY_SIZE(s_szRegisterTypeNamesShort) == RegisterType::Count);

  static_assert(RegisterType::Count <= EZ_BIT(RegisterType::MaxNumBits));
} // namespace

// static
const char* RegisterType::GetName(Enum registerType)
{
  EZ_ASSERT_DEBUG(registerType >= 0 && static_cast<ezUInt32>(registerType) < EZ_ARRAY_SIZE(s_szRegisterTypeNames), "Out of bounds access");
  return s_szRegisterTypeNames[registerType];
}

//////////////////////////////////////////////////////////////////////////

ezResult StreamDesc::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_sName;
  inout_stream << static_cast<ezUInt8>(m_DataType);

  return EZ_SUCCESS;
}

ezResult StreamDesc::Deserialize(ezStreamReader& inout_stream)
{
  inout_stream >> m_sName;

  ezUInt8 dataType = 0;
  inout_stream >> dataType;
  m_DataType = static_cast<ezProcessingStream::DataType>(dataType);

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

bool FunctionDesc::operator<(const FunctionDesc& other) const
{
  if (m_sName != other.m_sName)
    return m_sName < other.m_sName;

  if (m_uiNumRequiredInputs != other.m_uiNumRequiredInputs)
    return m_uiNumRequiredInputs < other.m_uiNumRequiredInputs;

  if (m_OutputType != other.m_OutputType)
    return m_OutputType < other.m_OutputType;

  return m_InputTypes.GetArrayPtr() < other.m_InputTypes.GetArrayPtr();
}

ezResult FunctionDesc::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_sName;
  EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_InputTypes));
  inout_stream << m_uiNumRequiredInputs;
  inout_stream << m_OutputType;

  return EZ_SUCCESS;
}

ezResult FunctionDesc::Deserialize(ezStreamReader& inout_stream)
{
  inout_stream >> m_sName;
  EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_InputTypes));
  inout_stream >> m_uiNumRequiredInputs;
  inout_stream >> m_OutputType;

  return EZ_SUCCESS;
}

ezHashedString FunctionDesc::GetMangledName() const
{
  ezStringBuilder sMangledName = m_sName.GetView();
  sMangledName.Append("_");

  for (auto inputType : m_InputTypes)
  {
    sMangledName.Append(s_szRegisterTypeNamesShort[inputType]);
  }

  ezHashedString sResult;
  sResult.Assign(sMangledName);
  return sResult;
}

//////////////////////////////////////////////////////////////////////////

namespace
{
  static const ezEnum<RegisterType> s_RandomInputTypes[] = {RegisterType::Int, RegisterType::Int};

  static void Random(Inputs inputs, Output output, const GlobalData& globalData)
  {
    EZ_IGNORE_UNUSED(globalData);

    const Register* pPositions = inputs[0].GetPtr();
    const Register* pPositionsEnd = inputs[0].GetEndPtr();
    Register* pOutput = output.GetPtr();

    if (inputs.GetCount() >= 2)
    {
      const Register* pSeeds = inputs[1].GetPtr();

      while (pPositions < pPositionsEnd)
      {
        pOutput->f = ezSimdRandom::FloatZeroToOne(pPositions->i, ezSimdVec4u(pSeeds->i));

        ++pPositions;
        ++pSeeds;
        ++pOutput;
      }
    }
    else
    {
      while (pPositions < pPositionsEnd)
      {
        pOutput->f = ezSimdRandom::FloatZeroToOne(pPositions->i);

        ++pPositions;
        ++pOutput;
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////

  static ezSimdPerlinNoise s_PerlinNoise(12345);
  static const ezEnum<RegisterType> s_PerlinNoiseInputTypes[] = {
    RegisterType::Float,
    RegisterType::Float,
    RegisterType::Float,
    RegisterType::Int,
  };

  static void PerlinNoise(Inputs inputs, Output output, const GlobalData& globalData)
  {
    EZ_IGNORE_UNUSED(globalData);

    const Register* pPosX = inputs[0].GetPtr();
    const Register* pPosY = inputs[1].GetPtr();
    const Register* pPosZ = inputs[2].GetPtr();
    const Register* pPosXEnd = inputs[0].GetEndPtr();

    const ezUInt32 uiNumOctaves = (inputs.GetCount() >= 4) ? inputs[3][0].i.x() : 1;

    Register* pOutput = output.GetPtr();

    while (pPosX < pPosXEnd)
    {
      pOutput->f = s_PerlinNoise.NoiseZeroToOne(pPosX->f, pPosY->f, pPosZ->f, uiNumOctaves);

      ++pPosX;
      ++pPosY;
      ++pPosZ;
      ++pOutput;
    }
  }

  //////////////////////////////////////////////////////////////////////////

  static ezHashedString s_sCurves = ezMakeHashedString("Curves");

  static const ezEnum<ezExpression::RegisterType> s_SampleCurvesInputTypes[] = {
    ezExpression::RegisterType::Int,   // CurveIndex
    ezExpression::RegisterType::Float, // X
  };

  static void SampleCurve(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData)
  {
    const ezVariantArray& curves = globalData.GetValue(s_sCurves)->Get<ezVariantArray>();
    if (curves.IsEmpty())
      return;

    ezUInt32 uiCurveIndex = inputs[0].GetPtr()->i.x();
    if (uiCurveIndex >= curves.GetCount())
      return;

    auto pSampledCurve = ezDynamicCast<const ezSampledCurve1D*>(curves[uiCurveIndex].Get<ezReflectedClass*>());
    if (pSampledCurve == nullptr || pSampledCurve->m_Samples.IsEmpty())
      return;

    const ezUInt32 uiMaxIdx = pSampledCurve->m_Samples.GetCount() - 1;
    const ezSimdVec4f vOffsetX = ezSimdVec4f(-pSampledCurve->m_fMinX);
    const float fRange = pSampledCurve->m_fMaxX - pSampledCurve->m_fMinX;
    const ezSimdVec4f vScale = ezSimdVec4f(fRange > 0.0f ? static_cast<float>(uiMaxIdx) / fRange : 0.0f);
    const ezSimdVec4i vMaxIdx = ezSimdVec4i(uiMaxIdx);
    const float* samples = pSampledCurve->m_Samples.GetData();

    const ezExpression::Register* pX = inputs[1].GetPtr();
    const ezExpression::Register* pXEnd = inputs[1].GetEndPtr();
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

      pOutput->f = ezSimdVec4f::Lerp(vSample0, vSample1, vFrac);

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

ezExpressionFunction ezDefaultExpressionFunctions::s_RandomFunc = {
  {ezMakeHashedString("random"), ezExpression::FunctionDesc::TypeList(s_RandomInputTypes), 1, RegisterType::Float},
  &Random,
};

ezExpressionFunction ezDefaultExpressionFunctions::s_PerlinNoiseFunc = {
  {ezMakeHashedString("perlinNoise"), ezExpression::FunctionDesc::TypeList(s_PerlinNoiseInputTypes), 3, RegisterType::Float},
  &PerlinNoise,
};

ezExpressionFunction ezExtendedExpressionFunctions::s_SampleCurveFunc = {
  {ezMakeHashedString("sampleCurve"), ezExpression::FunctionDesc::TypeList(s_SampleCurvesInputTypes), 2, ezExpression::RegisterType::Float},
  &SampleCurve,
  &SampleCurveValidate,
};

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezExpressionWidgetAttribute, 1, ezRTTIDefaultAllocator<ezExpressionWidgetAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("InputsProperty", m_sInputsProperty),
    EZ_MEMBER_PROPERTY("OutputsProperty", m_sOutputsProperty),
    EZ_MEMBER_PROPERTY("CustomKeywords", m_sCustomKeywords),
    EZ_MEMBER_PROPERTY("CustomKeywordColor", m_CustomKeywordColor),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, ezColorGammaUB),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


EZ_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Expression_Implementation_ExpressionDeclarations);
