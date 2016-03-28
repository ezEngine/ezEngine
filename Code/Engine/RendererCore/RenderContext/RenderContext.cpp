#include <RendererCore/PCH.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/TextureResource.h>

ezRenderContext* ezRenderContext::s_DefaultInstance = nullptr;
ezHybridArray<ezRenderContext*, 4> ezRenderContext::s_Instances;

ezRenderContext* ezRenderContext::GetDefaultInstance()
{
  if (s_DefaultInstance == nullptr)
    s_DefaultInstance = CreateInstance();

  return s_DefaultInstance;
}

ezRenderContext* ezRenderContext::CreateInstance()
{
  return EZ_DEFAULT_NEW(ezRenderContext);
}

void ezRenderContext::DestroyInstance(ezRenderContext* pRenderer)
{
  EZ_DEFAULT_DELETE(pRenderer);
}

ezRenderContext::ezRenderContext()
{
  if (s_DefaultInstance == nullptr)
  {
    SetGALContext(ezGALDevice::GetDefaultDevice()->GetPrimaryContext()); // set up with the default device
    s_DefaultInstance = this;
  }

  s_Instances.PushBack(this);

  m_StateFlags = ezRenderContextFlags::AllStatesInvalid;
  m_pCurrentlyModifyingBuffer = nullptr;
  m_uiMeshBufferPrimitiveCount = 0;
  m_uiLastMaterialCBSync = 0;
}

ezRenderContext::~ezRenderContext()
{
  if (s_DefaultInstance == this)
    s_DefaultInstance = nullptr;

  s_Instances.RemoveSwap(this);
}

void ezRenderContext::SetGALContext(ezGALContext* pContext)
{
  m_pGALContext = pContext;
}

void ezRenderContext::BindTexture(ezGALShaderStage::Enum stage, const ezTempHashedString& sSlotName, const ezTextureResourceHandle& hTexture)
{
  ezResourceLock<ezTextureResource> l(hTexture, ezResourceAcquireMode::AllowFallback);
  BindTexture(stage, sSlotName, l->GetGALTextureView(), l->GetGALSamplerState());
}

void ezRenderContext::BindTexture(ezGALShaderStage::Enum stage, const ezTempHashedString& sSlotName, ezGALResourceViewHandle hResourceView, ezGALSamplerStateHandle hSamplerState)
{
  m_BoundTextures[stage][sSlotName.GetHash()] = TextureViewSampler(hResourceView, hSamplerState);

  m_StateFlags.Add(ezRenderContextFlags::TextureBindingChanged);
}

void ezRenderContext::ApplyTextureBindings(ezGALShaderStage::Enum stage, const ezShaderStageBinary* pBinary)
{
  for (const auto& rb : pBinary->m_ShaderResourceBindings)
  {
    if (rb.m_Type == ezShaderStageResource::ConstantBuffer)
      continue;

    const ezUInt32 uiResourceHash = rb.m_Name.GetHash();

    TextureViewSampler* textureTuple;
    if (!m_BoundTextures[stage].TryGetValue(uiResourceHash, textureTuple))
    {
      ezLog::Error("No resource is bound for shader slot '%s'", rb.m_Name.GetData());
      continue;
    }

    if (textureTuple == nullptr || textureTuple->m_hResourceView.IsInvalidated() || textureTuple->m_hSamplerState.IsInvalidated())
    {
      ezLog::Error("An invalid resource is bound for shader slot '%s'", rb.m_Name.GetData());
      continue;
    }

    m_pGALContext->SetResourceView(stage, rb.m_iSlot, textureTuple->m_hResourceView);
    m_pGALContext->SetSamplerState(stage, rb.m_iSlot, textureTuple->m_hSamplerState);
  }
}

void ezRenderContext::BindMeshBuffer(const ezMeshBufferResourceHandle& hMeshBuffer)
{
  if (m_hMeshBuffer == hMeshBuffer)
    return;

  m_StateFlags.Add(ezRenderContextFlags::MeshBufferBindingChanged);
  m_hMeshBuffer = hMeshBuffer;
}

ezRenderContext::Statistics::Statistics()
{
  Reset();
}

void ezRenderContext::Statistics::Reset()
{
  m_uiFailedDrawcalls = 0;
}