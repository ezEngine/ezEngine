#include <PCH.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>

struct ezShaderStateVersion
{
  enum Enum : ezUInt32
  {
    Version0 = 0,
    Version1,
    Version2,


    ENUM_COUNT,
    Current = ENUM_COUNT - 1
  };
};

void ezShaderStateResourceDescriptor::Save(ezStreamWriter& stream) const
{
  stream << (ezUInt32) ezShaderStateVersion::Current;

  // Blend State
  {
    stream << m_BlendDesc.m_bAlphaToCoverage;
    stream << m_BlendDesc.m_bIndependentBlend;

    const ezUInt8 iBlends = m_BlendDesc.m_bIndependentBlend ? EZ_GAL_MAX_RENDERTARGET_COUNT : 1;
    stream << iBlends; // in case EZ_GAL_MAX_RENDERTARGET_COUNT ever changes

    for (ezUInt32 b = 0; b < iBlends; ++b)
    {
      stream << m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_bBlendingEnabled;
      stream << (ezUInt8) m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOp;
      stream << (ezUInt8) m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOpAlpha;
      stream << (ezUInt8) m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlend;
      stream << (ezUInt8) m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlendAlpha;
      stream << (ezUInt8) m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlend;
      stream << (ezUInt8) m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlendAlpha;
      stream << m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_uiWriteMask;
    }
  }

  // Depth Stencil State
  {
    stream << (ezUInt8) m_DepthStencilDesc.m_DepthTestFunc;
    stream << m_DepthStencilDesc.m_bDepthTest;
    stream << m_DepthStencilDesc.m_bDepthWrite;
    stream << m_DepthStencilDesc.m_bSeparateFrontAndBack;
    stream << m_DepthStencilDesc.m_bStencilTest;
    stream << m_DepthStencilDesc.m_uiStencilReadMask;
    stream << m_DepthStencilDesc.m_uiStencilWriteMask;
    stream << (ezUInt8) m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp;
    stream << (ezUInt8) m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp;
    stream << (ezUInt8) m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp;
    stream << (ezUInt8) m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc;
    stream << (ezUInt8) m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp;
    stream << (ezUInt8) m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp;
    stream << (ezUInt8) m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp;
    stream << (ezUInt8) m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc;
  }

  // Rasterizer State
  {
    stream << m_RasterizerDesc.m_bFrontCounterClockwise;
    stream << m_RasterizerDesc.m_bScissorTest;
    stream << m_RasterizerDesc.m_bWireFrame;
    stream << (ezUInt8) m_RasterizerDesc.m_CullMode;
    stream << m_RasterizerDesc.m_fDepthBiasClamp;
    stream << m_RasterizerDesc.m_fSlopeScaledDepthBias;
    stream << m_RasterizerDesc.m_iDepthBias;
  }
}

void ezShaderStateResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt32 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion >= ezShaderStateVersion::Version1 && uiVersion <= ezShaderStateVersion::Current, "Invalid version {0}", uiVersion);

  // Blend State
  {
    stream >> m_BlendDesc.m_bAlphaToCoverage;
    stream >> m_BlendDesc.m_bIndependentBlend;

    ezUInt8 iBlends = 0;
    stream >> iBlends; // in case EZ_GAL_MAX_RENDERTARGET_COUNT ever changes

    for (ezUInt32 b = 0; b < iBlends; ++b)
    {
      ezUInt8 uiTemp;
      stream >> m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_bBlendingEnabled;
      stream >> uiTemp; m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOp = (ezGALBlendOp::Enum) uiTemp;
      stream >> uiTemp; m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOpAlpha = (ezGALBlendOp::Enum) uiTemp;
      stream >> uiTemp; m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlend = (ezGALBlend::Enum) uiTemp;
      stream >> uiTemp; m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlendAlpha = (ezGALBlend::Enum) uiTemp;
      stream >> uiTemp; m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlend = (ezGALBlend::Enum) uiTemp;
      stream >> uiTemp; m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlendAlpha = (ezGALBlend::Enum) uiTemp;
      stream >> m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_uiWriteMask;
    }
  }

  // Depth Stencil State
  {
    ezUInt8 uiTemp = 0;
    stream >> uiTemp; m_DepthStencilDesc.m_DepthTestFunc = (ezGALCompareFunc::Enum) uiTemp;
    stream >> m_DepthStencilDesc.m_bDepthTest;
    stream >> m_DepthStencilDesc.m_bDepthWrite;
    stream >> m_DepthStencilDesc.m_bSeparateFrontAndBack;
    stream >> m_DepthStencilDesc.m_bStencilTest;
    stream >> m_DepthStencilDesc.m_uiStencilReadMask;
    stream >> m_DepthStencilDesc.m_uiStencilWriteMask;
    stream >> uiTemp; m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp = (ezGALStencilOp::Enum) uiTemp;
    stream >> uiTemp; m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp = (ezGALStencilOp::Enum) uiTemp;
    stream >> uiTemp; m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp = (ezGALStencilOp::Enum) uiTemp;
    stream >> uiTemp; m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc = (ezGALCompareFunc::Enum) uiTemp;
    stream >> uiTemp; m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp = (ezGALStencilOp::Enum) uiTemp;
    stream >> uiTemp; m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp = (ezGALStencilOp::Enum) uiTemp;
    stream >> uiTemp; m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp = (ezGALStencilOp::Enum) uiTemp;
    stream >> uiTemp; m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc = (ezGALCompareFunc::Enum) uiTemp;
  }

  // Rasterizer State
  {
    ezUInt8 uiTemp = 0;

    if (uiVersion < ezShaderStateVersion::Version2)
    {
      bool dummy;
      stream >> dummy;
    }

    stream >> m_RasterizerDesc.m_bFrontCounterClockwise;

    if (uiVersion < ezShaderStateVersion::Version2)
    {
      bool dummy;
      stream >> dummy;
      stream >> dummy;
    }

    stream >> m_RasterizerDesc.m_bScissorTest;
    stream >> m_RasterizerDesc.m_bWireFrame;
    stream >> uiTemp; m_RasterizerDesc.m_CullMode = (ezGALCullMode::Enum) uiTemp;
    stream >> m_RasterizerDesc.m_fDepthBiasClamp;
    stream >> m_RasterizerDesc.m_fSlopeScaledDepthBias;
    stream >> m_RasterizerDesc.m_iDepthBias;
  }
}

ezUInt32 ezShaderStateResourceDescriptor::CalculateHash() const
{
  return m_BlendDesc.CalculateHash() + m_RasterizerDesc.CalculateHash() + m_DepthStencilDesc.CalculateHash();
}

static const char* InsertNumber(const char* szString, ezUInt32 uiNumber, ezStringBuilder& sTemp)
{
  sTemp.Format(szString, uiNumber);
  return sTemp.GetData();
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
static ezSet<ezString> s_AllAllowedVariables;
#endif

static bool GetBoolStateVariable(const ezMap<ezString, ezString>& variables, const char* szVariable, bool defValue)
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return defValue;

  if (it.Value() == "true")
    return true;
  if (it.Value() == "false")
    return false;

  ezLog::Error("Shader state variable '{0}' is set to invalid value '{1}'. Should be 'true' or 'false'", szVariable, it.Value());
  return defValue;
}

static ezInt32 GetEnumStateVariable(const ezMap<ezString, ezString>& variables, const ezMap<ezString, ezInt32>& values, const char* szVariable, ezInt32 defValue)
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return defValue;

  auto itVal = values.Find(it.Value());
  if (!itVal.IsValid())
  {
    ezStringBuilder valid;
    for (auto vv = values.GetIterator(); vv.IsValid(); ++vv)
    {
      valid.Append(" ", vv.Key());
    }

    ezLog::Error("Shader state variable '{0}' is set to invalid value '{1}'. Valid values are:{2}", szVariable, it.Value(), valid);
    return defValue;
  }

  return itVal.Value();
}

static float GetFloatStateVariable(const ezMap<ezString, ezString>& variables, const char* szVariable, float defValue)
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return defValue;

  double result = 0;
  if (ezConversionUtils::StringToFloat(it.Value(), result).Failed())
  {
    ezLog::Error("Shader state variable '{0}' is not a valid float value: '{1}'.", szVariable, it.Value());
    return defValue;
  }

  return (float)result;
}

static ezInt32 GetIntStateVariable(const ezMap<ezString, ezString>& variables, const char* szVariable, ezInt32 defValue)
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return defValue;

  ezInt32 result = 0;
  if (ezConversionUtils::StringToInt(it.Value(), result).Failed())
  {
    ezLog::Error("Shader state variable '{0}' is not a valid int value: '{1}'.", szVariable, it.Value());
    return defValue;
  }

  return result;
}

// Global variables don't use memory tracking, so these won't reported as memory leaks.
static ezMap<ezString, ezInt32> StateValuesBlend;
static ezMap<ezString, ezInt32> StateValuesBlendOp;
static ezMap<ezString, ezInt32> StateValuesCullMode;
static ezMap<ezString, ezInt32> StateValuesCompareFunc;
static ezMap<ezString, ezInt32> StateValuesStencilOp;

ezResult ezShaderStateResourceDescriptor::Load(const char* szSource)
{
  ezMap<ezString, ezString> VariableValues;

  // extract all state assignments
  {
    ezStringBuilder sSource = szSource;

    ezHybridArray<ezStringView, 32> allAssignments;
    ezHybridArray<ezStringView, 4> components;
    sSource.Split(false, allAssignments, "\n", ";", "\r");

    ezStringBuilder temp1, temp2;
    for (const ezStringView& ass : allAssignments)
    {
      temp1 = ass;
      temp1.Trim(" \t\r\n;");
      if (temp1.IsEmpty())
        continue;

      temp1.Split(false, components, " ", "\t", "=", "\r");

      if (components.GetCount() != 2)
      {
        ezLog::Error("Malformed shader state assignment: '{0}'", temp1);
        continue;
      }

      temp1 = components[0];
      temp2 = components[1];

      VariableValues[temp1] = temp2;
    }
  }

  if (StateValuesBlend.IsEmpty())
  {
    // ezGALBlend
    {
      StateValuesBlend["Blend_Zero"] = ezGALBlend::Zero;
      StateValuesBlend["Blend_One"] = ezGALBlend::One;
      StateValuesBlend["Blend_SrcColor"] = ezGALBlend::SrcColor;
      StateValuesBlend["Blend_InvSrcColor"] = ezGALBlend::InvSrcColor;
      StateValuesBlend["Blend_SrcAlpha"] = ezGALBlend::SrcAlpha;
      StateValuesBlend["Blend_InvSrcAlpha"] = ezGALBlend::InvSrcAlpha;
      StateValuesBlend["Blend_DestAlpha"] = ezGALBlend::DestAlpha;
      StateValuesBlend["Blend_InvDestAlpha"] = ezGALBlend::InvDestAlpha;
      StateValuesBlend["Blend_DestColor"] = ezGALBlend::DestColor;
      StateValuesBlend["Blend_InvDestColor"] = ezGALBlend::InvDestColor;
      StateValuesBlend["Blend_SrcAlphaSaturated"] = ezGALBlend::SrcAlphaSaturated;
      StateValuesBlend["Blend_BlendFactor"] = ezGALBlend::BlendFactor;
      StateValuesBlend["Blend_InvBlendFactor"] = ezGALBlend::InvBlendFactor;
    }

    // ezGALBlendOp
    {
      StateValuesBlendOp["BlendOp_Add"] = ezGALBlendOp::Add;
      StateValuesBlendOp["BlendOp_Subtract"] = ezGALBlendOp::Subtract;
      StateValuesBlendOp["BlendOp_RevSubtract"] = ezGALBlendOp::RevSubtract;
      StateValuesBlendOp["BlendOp_Min"] = ezGALBlendOp::Min;
      StateValuesBlendOp["BlendOp_Max"] = ezGALBlendOp::Max;
    }

    // ezGALCullMode
    {
      StateValuesCullMode["CullMode_None"] = ezGALCullMode::None;
      StateValuesCullMode["CullMode_Front"] = ezGALCullMode::Front;
      StateValuesCullMode["CullMode_Back"] = ezGALCullMode::Back;
    }

    // ezGALCompareFunc
    {
      StateValuesCompareFunc["CompareFunc_Never"] = ezGALCompareFunc::Never;
      StateValuesCompareFunc["CompareFunc_Less"] = ezGALCompareFunc::Less;
      StateValuesCompareFunc["CompareFunc_Equal"] = ezGALCompareFunc::Equal;
      StateValuesCompareFunc["CompareFunc_LessEqual"] = ezGALCompareFunc::LessEqual;
      StateValuesCompareFunc["CompareFunc_Greater"] = ezGALCompareFunc::Greater;
      StateValuesCompareFunc["CompareFunc_NotEqual"] = ezGALCompareFunc::NotEqual;
      StateValuesCompareFunc["CompareFunc_GreaterEqual"] = ezGALCompareFunc::GreaterEqual;
      StateValuesCompareFunc["CompareFunc_Always"] = ezGALCompareFunc::Always;
    }

    // ezGALStencilOp
    {
      StateValuesStencilOp["StencilOp_Keep"] = ezGALStencilOp::Keep;
      StateValuesStencilOp["StencilOp_Zero"] = ezGALStencilOp::Zero;
      StateValuesStencilOp["StencilOp_Replace"] = ezGALStencilOp::Replace;
      StateValuesStencilOp["StencilOp_IncrementSaturated"] = ezGALStencilOp::IncrementSaturated;
      StateValuesStencilOp["StencilOp_DecrementSaturated"] = ezGALStencilOp::DecrementSaturated;
      StateValuesStencilOp["StencilOp_Invert"] = ezGALStencilOp::Invert;
      StateValuesStencilOp["StencilOp_Increment"] = ezGALStencilOp::Increment;
      StateValuesStencilOp["StencilOp_Decrement"] = ezGALStencilOp::Decrement;
    }
  }

  // Retrieve Blend State
  {
    m_BlendDesc.m_bAlphaToCoverage = GetBoolStateVariable(VariableValues, "AlphaToCoverage", m_BlendDesc.m_bAlphaToCoverage);
    m_BlendDesc.m_bIndependentBlend = GetBoolStateVariable(VariableValues, "IndependentBlend", m_BlendDesc.m_bIndependentBlend);

    ezStringBuilder s;

    for (ezUInt32 i = 0; i < 8; ++i)
    {
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_bBlendingEnabled = GetBoolStateVariable(VariableValues, InsertNumber("BlendingEnabled{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_bBlendingEnabled);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_BlendOp = (ezGALBlendOp::Enum) GetEnumStateVariable(VariableValues, StateValuesBlendOp, InsertNumber("BlendOp{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_BlendOp);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_BlendOpAlpha = (ezGALBlendOp::Enum) GetEnumStateVariable(VariableValues, StateValuesBlendOp, InsertNumber("BlendOpAlpha{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_BlendOpAlpha);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_DestBlend = (ezGALBlend::Enum) GetEnumStateVariable(VariableValues, StateValuesBlend, InsertNumber("DestBlend{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_DestBlend);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_DestBlendAlpha = (ezGALBlend::Enum) GetEnumStateVariable(VariableValues, StateValuesBlend, InsertNumber("DestBlendAlpha{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_DestBlendAlpha);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_SourceBlend = (ezGALBlend::Enum) GetEnumStateVariable(VariableValues, StateValuesBlend, InsertNumber("SourceBlend{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlend);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_SourceBlendAlpha = (ezGALBlend::Enum) GetEnumStateVariable(VariableValues, StateValuesBlend, InsertNumber("SourceBlendAlpha{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlendAlpha);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_uiWriteMask = GetIntStateVariable(VariableValues, InsertNumber("WriteMask{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_uiWriteMask);
    }
  }

  // Retrieve Rasterizer State
  {
    m_RasterizerDesc.m_bFrontCounterClockwise = GetBoolStateVariable(VariableValues, "FrontCounterClockwise", m_RasterizerDesc.m_bFrontCounterClockwise);
    m_RasterizerDesc.m_bScissorTest = GetBoolStateVariable(VariableValues, "ScissorTest", m_RasterizerDesc.m_bScissorTest);
    m_RasterizerDesc.m_bWireFrame = GetBoolStateVariable(VariableValues, "WireFrame", m_RasterizerDesc.m_bWireFrame);
    m_RasterizerDesc.m_CullMode = (ezGALCullMode::Enum) GetEnumStateVariable(VariableValues, StateValuesCullMode, "CullMode", m_RasterizerDesc.m_CullMode);
    m_RasterizerDesc.m_fDepthBiasClamp = GetFloatStateVariable(VariableValues, "DepthBiasClamp", m_RasterizerDesc.m_fDepthBiasClamp);
    m_RasterizerDesc.m_fSlopeScaledDepthBias = GetFloatStateVariable(VariableValues, "SlopeScaledDepthBias", m_RasterizerDesc.m_fSlopeScaledDepthBias);
    m_RasterizerDesc.m_iDepthBias = GetIntStateVariable(VariableValues, "DepthBias", m_RasterizerDesc.m_iDepthBias);
  }

  // Retrieve Depth-Stencil State
  {
    m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp = (ezGALStencilOp::Enum) GetEnumStateVariable(VariableValues, StateValuesStencilOp, "BackFaceDepthFailOp", m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp = (ezGALStencilOp::Enum) GetEnumStateVariable(VariableValues, StateValuesStencilOp, "BackFaceFailOp", m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp = (ezGALStencilOp::Enum) GetEnumStateVariable(VariableValues, StateValuesStencilOp, "BackFacePassOp", m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc = (ezGALCompareFunc::Enum) GetEnumStateVariable(VariableValues, StateValuesCompareFunc, "BackFaceStencilFunc", m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc);

    m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp = (ezGALStencilOp::Enum) GetEnumStateVariable(VariableValues, StateValuesStencilOp, "FrontFaceDepthFailOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp = (ezGALStencilOp::Enum) GetEnumStateVariable(VariableValues, StateValuesStencilOp, "FrontFaceFailOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp = (ezGALStencilOp::Enum) GetEnumStateVariable(VariableValues, StateValuesStencilOp, "FrontFacePassOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc = (ezGALCompareFunc::Enum) GetEnumStateVariable(VariableValues, StateValuesCompareFunc, "FrontFaceStencilFunc", m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc);

    m_DepthStencilDesc.m_bDepthTest = GetBoolStateVariable(VariableValues, "DepthTest", m_DepthStencilDesc.m_bDepthTest);
    m_DepthStencilDesc.m_bDepthWrite = GetBoolStateVariable(VariableValues, "DepthWrite", m_DepthStencilDesc.m_bDepthWrite);
    m_DepthStencilDesc.m_bSeparateFrontAndBack = GetBoolStateVariable(VariableValues, "SeparateFrontAndBack", m_DepthStencilDesc.m_bSeparateFrontAndBack);
    m_DepthStencilDesc.m_bStencilTest = GetBoolStateVariable(VariableValues, "StencilTest", m_DepthStencilDesc.m_bStencilTest);
    m_DepthStencilDesc.m_DepthTestFunc = (ezGALCompareFunc::Enum) GetEnumStateVariable(VariableValues, StateValuesCompareFunc, "DepthTestFunc", m_DepthStencilDesc.m_DepthTestFunc);
    m_DepthStencilDesc.m_uiStencilReadMask = GetIntStateVariable(VariableValues, "StencilReadMask", m_DepthStencilDesc.m_uiStencilReadMask);
    m_DepthStencilDesc.m_uiStencilWriteMask = GetIntStateVariable(VariableValues, "StencilWriteMask", m_DepthStencilDesc.m_uiStencilWriteMask);
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // check for invalid variable names
  {
    for (auto it = VariableValues.GetIterator(); it.IsValid(); ++it)
    {
      if (!s_AllAllowedVariables.Contains(it.Key()))
      {
        ezLog::Error("The shader state variable '{0}' does not exist.", it.Key());
      }
    }
  }
#endif


  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ShaderStateDescriptor);

