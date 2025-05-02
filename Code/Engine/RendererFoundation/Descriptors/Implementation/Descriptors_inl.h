

inline ezGALShaderCreationDescription::ezGALShaderCreationDescription()
  : ezHashableStruct()
{
}

inline ezGALShaderCreationDescription::ezGALShaderCreationDescription(const ezGALShaderCreationDescription& other)
{
  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; ++i)
  {
    m_ByteCodes[i] = other.m_ByteCodes[i];
  }
}

inline ezGALShaderCreationDescription::~ezGALShaderCreationDescription()
{
  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; ++i)
  {
    m_ByteCodes[i] = nullptr;
  }
}

inline void ezGALShaderCreationDescription::operator=(const ezGALShaderCreationDescription& other)
{
  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; ++i)
  {
    m_ByteCodes[i] = other.m_ByteCodes[i];
  }
}

inline bool ezGALShaderCreationDescription::HasByteCodeForStage(ezGALShaderStage::Enum stage) const
{
  return m_ByteCodes[stage] != nullptr && m_ByteCodes[stage]->IsValid();
}

inline void ezGALTextureCreationDescription::SetAsRenderTarget(ezUInt32 uiWidth, ezUInt32 uiHeight, ezGALResourceFormat::Enum format, ezGALMSAASampleCount::Enum sampleCount /*= ezGALMSAASampleCount::None*/)
{
  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;
  m_uiDepth = 1;
  m_uiMipLevelCount = 1;
  m_uiArraySize = 1;
  m_SampleCount = sampleCount;
  m_Format = format;
  m_Type = ezGALTextureType::Texture2D;
  m_bAllowShaderResourceView = true;
  m_bAllowUAV = false;
  m_bAllowRenderTargetView = true;
  m_bAllowDynamicMipGeneration = false;
  m_ResourceAccess.m_bImmutable = false;
  m_pExisitingNativeObject = nullptr;
}

EZ_FORCE_INLINE ezGALVertexAttribute::ezGALVertexAttribute(ezGALVertexAttributeSemantic::Enum semantic, ezGALResourceFormat::Enum format, ezUInt16 uiOffset, ezUInt8 uiVertexBufferSlot)
  : m_eSemantic(semantic)
  , m_eFormat(format)
  , m_uiOffset(uiOffset)
  , m_uiVertexBufferSlot(uiVertexBufferSlot)
{
}
