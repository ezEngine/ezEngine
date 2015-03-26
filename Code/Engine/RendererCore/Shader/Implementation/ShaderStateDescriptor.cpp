#include <RendererCore/PCH.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>

enum ezShaderStateVersion : ezUInt32
{
  Version0 = 0,
  Version1,


  ENUM_COUNT,
  Current = ENUM_COUNT - 1
};

void ezShaderStateResourceDescriptor::Save(ezStreamWriterBase& stream) const
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
    stream << m_RasterizerDesc.m_bDepthClip;
    stream << m_RasterizerDesc.m_bFrontCounterClockwise;
    stream << m_RasterizerDesc.m_bLineAA;
    stream << m_RasterizerDesc.m_bMSAA;
    stream << m_RasterizerDesc.m_bScissorTest;
    stream << m_RasterizerDesc.m_bWireFrame;
    stream << (ezUInt8) m_RasterizerDesc.m_CullMode;
    stream << m_RasterizerDesc.m_fDepthBiasClamp;
    stream << m_RasterizerDesc.m_fSlopeScaledDepthBias;
    stream << m_RasterizerDesc.m_iDepthBias;
  }
}

void ezShaderStateResourceDescriptor::Load(ezStreamReaderBase& stream)
{
  ezUInt32 uiVersion = 0;
  stream >> uiVersion;
  
  EZ_ASSERT_DEV(uiVersion >= ezShaderStateVersion::Version1 && uiVersion <= ezShaderStateVersion::Current, "Invalid version %u", uiVersion);

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
    stream >> m_RasterizerDesc.m_bDepthClip;
    stream >> m_RasterizerDesc.m_bFrontCounterClockwise;
    stream >> m_RasterizerDesc.m_bLineAA;
    stream >> m_RasterizerDesc.m_bMSAA;
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

ezResult ezShaderStateResourceDescriptor::Load(const char* szSource)
{
  ezStringBuilder sSource = szSource;

  ezLuaWrapper lua;

  // ezGALBlend
  {
    lua.SetVariable("Blend_Zero", ezGALBlend::Zero);
    lua.SetVariable("Blend_One", ezGALBlend::One);
    lua.SetVariable("Blend_SrcColor", ezGALBlend::SrcColor);
    lua.SetVariable("Blend_InvSrcColor", ezGALBlend::InvSrcColor);
    lua.SetVariable("Blend_SrcAlpha", ezGALBlend::SrcAlpha);
    lua.SetVariable("Blend_InvSrcAlpha", ezGALBlend::InvSrcAlpha);
    lua.SetVariable("Blend_DestAlpha", ezGALBlend::DestAlpha);
    lua.SetVariable("Blend_InvDestAlpha", ezGALBlend::InvDestAlpha);
    lua.SetVariable("Blend_DestColor", ezGALBlend::DestColor);
    lua.SetVariable("Blend_InvDestColor", ezGALBlend::InvDestColor);
    lua.SetVariable("Blend_SrcAlphaSaturated", ezGALBlend::SrcAlphaSaturated);
    lua.SetVariable("Blend_BlendFactor", ezGALBlend::BlendFactor);
    lua.SetVariable("Blend_InvBlendFactor", ezGALBlend::InvBlendFactor);
  }

  // ezGALBlendOp
  {
    lua.SetVariable("BlendOp_Add", ezGALBlendOp::Add);
    lua.SetVariable("BlendOp_Subtract", ezGALBlendOp::Subtract);
    lua.SetVariable("BlendOp_RevSubtract", ezGALBlendOp::RevSubtract);
    lua.SetVariable("BlendOp_Min", ezGALBlendOp::Min);
    lua.SetVariable("BlendOp_Max", ezGALBlendOp::Max);
  }

  // ezGALCullMode
  {
    lua.SetVariable("CullMode_None", ezGALCullMode::None);
    lua.SetVariable("CullMode_Front", ezGALCullMode::Front);
    lua.SetVariable("CullMode_Back", ezGALCullMode::Back);
  }

  // ezGALCompareFunc
  {
    lua.SetVariable("CompareFunc_Never", ezGALCompareFunc::Never);
    lua.SetVariable("CompareFunc_Less", ezGALCompareFunc::Less);
    lua.SetVariable("CompareFunc_Equal", ezGALCompareFunc::Equal);
    lua.SetVariable("CompareFunc_LessEqual", ezGALCompareFunc::LessEqual);
    lua.SetVariable("CompareFunc_Greater", ezGALCompareFunc::Greater);
    lua.SetVariable("CompareFunc_NotEqual", ezGALCompareFunc::NotEqual);
    lua.SetVariable("CompareFunc_GreaterEqual", ezGALCompareFunc::GreaterEqual);
    lua.SetVariable("CompareFunc_Always", ezGALCompareFunc::Always);
  }

  // ezGALStencilOp
  {
    lua.SetVariable("StencilOp_Keep", ezGALStencilOp::Keep);
    lua.SetVariable("StencilOp_Zero", ezGALStencilOp::Zero);
    lua.SetVariable("StencilOp_Replace", ezGALStencilOp::Replace);
    lua.SetVariable("StencilOp_IncrementSaturated", ezGALStencilOp::IncrementSaturated);
    lua.SetVariable("StencilOp_DecrementSaturated", ezGALStencilOp::DecrementSaturated);
    lua.SetVariable("StencilOp_Invert", ezGALStencilOp::Invert);
    lua.SetVariable("StencilOp_Increment", ezGALStencilOp::Increment);
    lua.SetVariable("StencilOp_Decrement", ezGALStencilOp::Decrement);
  }

  if (lua.ExecuteString(szSource, "ShaderState", ezGlobalLog::GetInstance()).Failed())
    return EZ_FAILURE;

  // Retrieve Blend State
  {
    m_BlendDesc.m_bAlphaToCoverage = lua.GetBoolVariable("AlphaToCoverage", m_BlendDesc.m_bAlphaToCoverage);
    m_BlendDesc.m_bIndependentBlend = lua.GetBoolVariable("IndependentBlend", m_BlendDesc.m_bIndependentBlend);

    ezStringBuilder s;

    for (ezUInt32 i = 0; i < 8; ++i)
    {
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_bBlendingEnabled = lua.GetBoolVariable(InsertNumber("BlendingEnabled%u", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_bBlendingEnabled);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_BlendOp = (ezGALBlendOp::Enum) lua.GetIntVariable(InsertNumber("BlendOp%u", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_BlendOp);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_BlendOpAlpha = (ezGALBlendOp::Enum) lua.GetIntVariable(InsertNumber("BlendOpAlpha%u", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_BlendOpAlpha);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_DestBlend = (ezGALBlend::Enum) lua.GetIntVariable(InsertNumber("DestBlend%u", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_DestBlend);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_DestBlendAlpha = (ezGALBlend::Enum) lua.GetIntVariable(InsertNumber("DestBlendAlpha%u", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_DestBlendAlpha);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_SourceBlend = (ezGALBlend::Enum) lua.GetIntVariable(InsertNumber("SourceBlend%u", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlend);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_SourceBlendAlpha = (ezGALBlend::Enum) lua.GetIntVariable(InsertNumber("SourceBlendAlpha%u", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlendAlpha);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_uiWriteMask = lua.GetIntVariable(InsertNumber("WriteMask%u", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_uiWriteMask);
    }
  }

  // Retrieve Rasterizer State
  {
    m_RasterizerDesc.m_bDepthClip = lua.GetBoolVariable("DepthClip", m_RasterizerDesc.m_bDepthClip);
    m_RasterizerDesc.m_bFrontCounterClockwise = lua.GetBoolVariable("FrontCounterClockwise", m_RasterizerDesc.m_bFrontCounterClockwise);
    m_RasterizerDesc.m_bLineAA = lua.GetBoolVariable("LineAA", m_RasterizerDesc.m_bLineAA);
    m_RasterizerDesc.m_bMSAA = lua.GetBoolVariable("MSAA", m_RasterizerDesc.m_bMSAA);
    m_RasterizerDesc.m_bScissorTest = lua.GetBoolVariable("ScissorTest", m_RasterizerDesc.m_bScissorTest);
    m_RasterizerDesc.m_bWireFrame = lua.GetBoolVariable("WireFrame", m_RasterizerDesc.m_bWireFrame);
    m_RasterizerDesc.m_CullMode = (ezGALCullMode::Enum) lua.GetIntVariable("CullMode", m_RasterizerDesc.m_CullMode);
    m_RasterizerDesc.m_fDepthBiasClamp = lua.GetFloatVariable("DepthBiasClamp", m_RasterizerDesc.m_fDepthBiasClamp);
    m_RasterizerDesc.m_fSlopeScaledDepthBias = lua.GetFloatVariable("SlopeScaledDepthBias", m_RasterizerDesc.m_fSlopeScaledDepthBias);
    m_RasterizerDesc.m_iDepthBias = lua.GetIntVariable("DepthBias", m_RasterizerDesc.m_iDepthBias);
  }

  // Retrieve Depth-Stencil State
  {
    m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp = (ezGALStencilOp::Enum) lua.GetIntVariable("BackFaceDepthFailOp", m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp = (ezGALStencilOp::Enum) lua.GetIntVariable("BackFaceFailOp", m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp = (ezGALStencilOp::Enum) lua.GetIntVariable("BackFacePassOp", m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc = (ezGALCompareFunc::Enum) lua.GetIntVariable("BackFaceStencilFunc", m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc);

    m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp = (ezGALStencilOp::Enum) lua.GetIntVariable("FrontFaceDepthFailOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp = (ezGALStencilOp::Enum) lua.GetIntVariable("FrontFaceFailOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp = (ezGALStencilOp::Enum) lua.GetIntVariable("FrontFacePassOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc = (ezGALCompareFunc::Enum) lua.GetIntVariable("FrontFaceStencilFunc", m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc);

    m_DepthStencilDesc.m_bDepthTest = lua.GetBoolVariable("DepthTest", m_DepthStencilDesc.m_bDepthTest);
    m_DepthStencilDesc.m_bDepthWrite = lua.GetBoolVariable("DepthWrite", m_DepthStencilDesc.m_bDepthWrite);
    m_DepthStencilDesc.m_bSeparateFrontAndBack = lua.GetBoolVariable("SeparateFrontAndBack", m_DepthStencilDesc.m_bSeparateFrontAndBack);
    m_DepthStencilDesc.m_bStencilTest = lua.GetBoolVariable("StencilTest", m_DepthStencilDesc.m_bStencilTest);
    m_DepthStencilDesc.m_DepthTestFunc = (ezGALCompareFunc::Enum) lua.GetIntVariable("DepthTestFunc", m_DepthStencilDesc.m_DepthTestFunc);
    m_DepthStencilDesc.m_uiStencilReadMask = lua.GetIntVariable("StencilReadMask", m_DepthStencilDesc.m_uiStencilReadMask);
    m_DepthStencilDesc.m_uiStencilWriteMask = lua.GetIntVariable("StencilWriteMask", m_DepthStencilDesc.m_uiStencilWriteMask);
  }
  

  return EZ_SUCCESS;
}


