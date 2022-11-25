#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <Foundation/SimdMath/SimdNoise.h>
#include <Foundation/SimdMath/SimdRandom.h>

using namespace ezExpression;

namespace
{
  static const char* s_szRegisterTypeNames[] = {
    "Float",
    "Int",
    "Bool",
    "Unknown",
  };

  static_assert(EZ_ARRAY_SIZE(s_szRegisterTypeNames) == RegisterType::Count);
  static_assert(RegisterType::Count <= EZ_BIT(RegisterType::MaxNumBits));
} // namespace

// static
const char* RegisterType::GetName(Enum registerType)
{
  EZ_ASSERT_DEBUG(registerType >= 0 && registerType < EZ_ARRAY_SIZE(s_szRegisterTypeNames), "Out of bounds access");
  return s_szRegisterTypeNames[registerType];
}

//////////////////////////////////////////////////////////////////////////

ezResult StreamDesc::Serialize(ezStreamWriter& stream) const
{
  stream << m_sName;
  stream << static_cast<ezUInt8>(m_DataType);

  return EZ_SUCCESS;
}

ezResult StreamDesc::Deserialize(ezStreamReader& stream)
{
  stream >> m_sName;

  ezUInt8 dataType = 0;
  stream >> dataType;
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

ezResult FunctionDesc::Serialize(ezStreamWriter& stream) const
{
  stream << m_sName;
  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_InputTypes));
  stream << m_uiNumRequiredInputs;
  stream << m_OutputType;

  return EZ_SUCCESS;
}

ezResult FunctionDesc::Deserialize(ezStreamReader& stream)
{
  stream >> m_sName;
  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_InputTypes));
  stream >> m_uiNumRequiredInputs;
  stream >> m_OutputType;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

namespace
{
  static const ezEnum<RegisterType> s_RandomInputTypes[] = {RegisterType::Int, RegisterType::Int};

  static void Random(Inputs inputs, Output output, const GlobalData& globalData)
  {
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

  static ezSimdPerlinNoise s_PerlinNoise(12345);
  static const ezEnum<RegisterType> s_PerlinNoiseInputTypes[] = {
    RegisterType::Float,
    RegisterType::Float,
    RegisterType::Float,
    RegisterType::Int,
  };

  static void PerlinNoise(Inputs inputs, Output output, const GlobalData& globalData)
  {
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
} // namespace

ezExpressionFunction ezDefaultExpressionFunctions::s_RandomFunc = {
  {ezMakeHashedString("Random"), ezMakeArrayPtr(s_RandomInputTypes), 1, RegisterType::Float},
  &Random,
};

ezExpressionFunction ezDefaultExpressionFunctions::s_PerlinNoiseFunc = {
  {ezMakeHashedString("PerlinNoise"), ezMakeArrayPtr(s_PerlinNoiseInputTypes), 3, RegisterType::Float},
  &PerlinNoise,
};
