
ezGALSwapChainCreationDescription::ezGALSwapChainCreationDescription()
  : ezHashableStruct(),
    m_pWindow(NULL),
    m_SampleCount(ezGALMSAASampleCount::None),
    m_BackBufferFormat(ezGALResourceFormat::RGBAUByteNormalizedsRGB),
    m_bDoubleBuffered(true),
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

bool ezGALShaderCreationDescription::HasByteCodeForStage(ezGALShaderStage::Enum Stage) const
{
  return m_ByteCodes[Stage] != NULL && m_ByteCodes[Stage]->IsValid();
}


ezGALResourceAccess::ezGALResourceAccess()
  : m_bImmutable(true), m_bReadBack(false)
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
    m_pExisitingNativeObject(NULL)
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
    m_bFrontCounterClockwise(true),
    m_bDepthClip(true),
    m_bScissorTest(false),
    m_bMSAA(false),
    m_bLineAA(false)
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