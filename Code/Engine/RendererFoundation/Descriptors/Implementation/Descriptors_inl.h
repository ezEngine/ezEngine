
inline ezGALShaderCreationDescription::ezGALShaderCreationDescription()
  : ezHashableStruct()
{
}

inline ezGALShaderCreationDescription::~ezGALShaderCreationDescription()
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

inline bool ezGALShaderCreationDescription::HasByteCodeForStage(ezGALShaderStage::Enum Stage) const
{
  return m_ByteCodes[Stage] != nullptr && m_ByteCodes[Stage]->IsValid();
}

inline void ezGALTextureCreationDescription::SetAsRenderTarget(
  ezUInt32 uiWidth, ezUInt32 uiHeight, ezGALResourceFormat::Enum format, ezGALMSAASampleCount::Enum sampleCount /*= ezGALMSAASampleCount::None*/)
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
  m_bCreateRenderTarget = true;
  m_bAllowDynamicMipGeneration = false;
  m_ResourceAccess.m_bReadBack = false;
  m_ResourceAccess.m_bImmutable = true;
  m_pExisitingNativeObject = nullptr;
}

EZ_FORCE_INLINE ezGALVertexAttribute::ezGALVertexAttribute(
  ezGALVertexAttributeSemantic::Enum eSemantic, ezGALResourceFormat::Enum eFormat, ezUInt16 uiOffset, ezUInt8 uiVertexBufferSlot, bool bInstanceData)
  : m_eSemantic(eSemantic)
  , m_eFormat(eFormat)
  , m_uiOffset(uiOffset)
  , m_uiVertexBufferSlot(uiVertexBufferSlot)
  , m_bInstanceData(bInstanceData)
{
}
