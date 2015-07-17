
ezGALSwapChainCreationDescription::ezGALSwapChainCreationDescription()
  : ezHashableStruct(),
    m_pWindow(nullptr),
    m_SampleCount(ezGALMSAASampleCount::None),
    m_BackBufferFormat(ezGALResourceFormat::RGBAUByteNormalizedsRGB),
    m_DepthStencilBufferFormat(ezGALResourceFormat::D24S8),
    m_bDoubleBuffered(true),
    m_bVerticalSynchronization(false),
    m_bCreateDepthStencilBuffer(true),
    m_bFullscreen(false),
    m_bAllowScreenshots(false)
{
}


ezGALDeviceCreationDescription::ezGALDeviceCreationDescription()
  : m_PrimarySwapChainDescription(),
    m_bDebugDevice(false),
    m_bCreatePrimarySwapChain(true)
{
}


ezGALShaderCreationDescription::ezGALShaderCreationDescription()
  : ezHashableStruct()
{
}

ezGALShaderCreationDescription::~ezGALShaderCreationDescription()
{
  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; ++i)
  {
    ezGALShaderByteCode* pByteCode = m_ByteCodes[i];
    m_ByteCodes[i] = nullptr;

    if (pByteCode != nullptr && pByteCode->GetRefCount() == 0)
    {
      EZ_DEFAULT_DELETE(pByteCode);
    }
  }
}

bool ezGALShaderCreationDescription::HasByteCodeForStage(ezGALShaderStage::Enum Stage) const
{
  return m_ByteCodes[Stage] != nullptr && m_ByteCodes[Stage]->IsValid();
}

ezGALRenderTargetBlendDescription::ezGALRenderTargetBlendDescription()
  : m_SourceBlend(ezGALBlend::One),
    m_DestBlend(ezGALBlend::One),
    m_BlendOp(ezGALBlendOp::Add),
    m_SourceBlendAlpha(ezGALBlend::One),
    m_DestBlendAlpha(ezGALBlend::One),
    m_BlendOpAlpha(ezGALBlendOp::Add),
    m_uiWriteMask(0xFF),
    m_bBlendingEnabled(false)
{
}

ezGALBlendStateCreationDescription::ezGALBlendStateCreationDescription()
  : m_bAlphaToCoverage(false),
    m_bIndependentBlend(false)
{
}

ezGALResourceAccess::ezGALResourceAccess()
  : m_bReadBack(false), m_bImmutable(true)
{
}

bool ezGALResourceAccess::IsImmutable() const
{
  return m_bImmutable;
}


ezGALBufferCreationDescription::ezGALBufferCreationDescription()
  : ezHashableStruct(),
    m_uiStructSize(0),
    m_uiTotalSize(0),
    m_BufferType(ezGALBufferType::Storage),
    m_bUseForIndirectArguments(false),
    m_bUseAsStructuredBuffer(false),
    m_bAllowRawViews(false),
    m_bStreamOutputTarget(false),
    m_bAllowShaderResourceView(false),
    m_bAllowUAV(false),
    m_ResourceAccess()
{
}


ezGALTextureCreationDescription::ezGALTextureCreationDescription()
  : ezHashableStruct(),
    m_uiWidth(0),
    m_uiHeight(0),
    m_uiDepth(1),
    m_uiMipSliceCount(1),
    m_uiArraySize(1),
    m_SampleCount(ezGALMSAASampleCount::None),
    m_Format(ezGALResourceFormat::Invalid),
    m_Type(ezGALTextureType::Texture2D),
    m_bAllowShaderResourceView(true),
    m_bAllowUAV(false),
    m_bCreateRenderTarget(false),
    m_bAllowDynamicMipGeneration(false),
    m_ResourceAccess(),
    m_pExisitingNativeObject(nullptr)
{
}


ezGALResourceViewCreationDescription::ezGALResourceViewCreationDescription()
  : m_hTexture(),
    m_hBuffer(),
    m_OverrideViewFormat(ezGALResourceFormat::Invalid),
    m_uiMostDetailedMipLevel(0),
    m_uiMipLevelsToUse(0xFFFFFFFFu),
    m_uiFirstArraySlice(0),
    m_uiArraySize(1),
    m_uiFirstElement(0),
    m_uiNumElements(0),
    m_bRawView(false)
{
}


ezGALRenderTargetViewCreationDescription::ezGALRenderTargetViewCreationDescription()
  : ezHashableStruct(),
    m_hTexture(),
    m_hBuffer(),
    m_RenderTargetType(ezGALRenderTargetType::Color),
    m_OverrideViewFormat(ezGALResourceFormat::Invalid),
    m_uiMipSlice(0),
    m_uiFirstSlice(0),
    m_uiSliceCount(1),
    m_bReadOnly(false)
{
}

ezGALVertexAttribute::ezGALVertexAttribute()
  : m_eSemantic(ezGALVertexAttributeSemantic::Position),
  m_eFormat(ezGALResourceFormat::XYZFloat),
  m_uiOffset(0),
  m_uiVertexBufferSlot(0),
  m_bInstanceData(false)
{
}

ezGALVertexAttribute::ezGALVertexAttribute(ezGALVertexAttributeSemantic::Enum eSemantic, ezGALResourceFormat::Enum eFormat, ezUInt16 uiOffset, ezUInt8 uiVertexBufferSlot, bool bInstanceData)
  : m_eSemantic(eSemantic),
    m_eFormat(eFormat),
    m_uiOffset(uiOffset),
    m_uiVertexBufferSlot(uiVertexBufferSlot),
    m_bInstanceData(bInstanceData)
{
}

ezGALRasterizerStateCreationDescription::ezGALRasterizerStateCreationDescription()
  : m_CullMode(ezGALCullMode::Back),
    m_iDepthBias(0),
    m_fDepthBiasClamp(0.0f),
    m_fSlopeScaledDepthBias(0.0f),
    m_bWireFrame(false),
    m_bFrontCounterClockwise(false),
    m_bDepthClip(true),
    m_bScissorTest(false),
    m_bMSAA(false),
    m_bLineAA(false)
{
}

ezGALStencilOpDescription::ezGALStencilOpDescription()
  : m_FailOp(ezGALStencilOp::Keep),
    m_DepthFailOp(ezGALStencilOp::Keep),
    m_PassOp(ezGALStencilOp::Keep),
    m_StencilFunc(ezGALCompareFunc::Always)
{
}

ezGALDepthStencilStateCreationDescription::ezGALDepthStencilStateCreationDescription()
  : m_DepthTestFunc(ezGALCompareFunc::Less),
    m_bSeparateFrontAndBack(false),
    m_bDepthTest(true),
    m_bDepthWrite(true),
    m_bStencilTest(false),
    m_uiStencilReadMask(0xFF),
    m_uiStencilWriteMask(0xFF)
{
}

ezGALSamplerStateCreationDescription::ezGALSamplerStateCreationDescription()
  : m_MinFilter(ezGALTextureFilterMode::Linear),
    m_MagFilter(ezGALTextureFilterMode::Linear),
    m_MipFilter(ezGALTextureFilterMode::Linear),
    m_AddressU(ezGALTextureAddressMode::Wrap),
    m_AddressV(ezGALTextureAddressMode::Wrap),
    m_AddressW(ezGALTextureAddressMode::Wrap),
    m_SampleCompareFunc(ezGALCompareFunc::Never),
    m_BorderColor(0.0f, 0.0f, 0.0f, 0.0f),
    m_fMipLodBias(0),
    m_fMinMip(-1.0f),
    m_fMaxMip(42000.0f),
    m_uiMaxAnisotropy(16)
{
}