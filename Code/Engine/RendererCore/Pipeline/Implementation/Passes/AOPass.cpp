#include <RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/AOPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/DownscaleDepthConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/SSAOConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAOPass, 1, ezRTTIDefaultAllocator<ezAOPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DepthInput", m_PinDepthInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.01f, 10.0f)),
    EZ_MEMBER_PROPERTY("MaxScreenSpaceRadius", m_fMaxScreenSpaceRadius)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.01f, 2.0f)),
    EZ_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new ezDefaultValueAttribute(2.0f)),
    EZ_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new ezDefaultValueAttribute(0.7f)),
    EZ_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new ezDefaultValueAttribute(80.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("FadeOutEnd", GetFadeOutEnd, SetFadeOutEnd)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("PositionBias", m_fPositionBias)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.0f, 1000.0f)),
    EZ_MEMBER_PROPERTY("MipLevelScale", m_fMipLevelScale)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("DepthBlurThreshold", m_fDepthBlurThreshold)->AddAttributes(new ezDefaultValueAttribute(2.0f), new ezClampValueAttribute(0.01f, ezVariant())),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAOPass::ezAOPass()
  : ezRenderPipelinePass("AOPass")
  , m_fRadius(1.0f)
  , m_fMaxScreenSpaceRadius(1.0f)
  , m_fContrast(2.0f)
  , m_fIntensity(0.7f)
  , m_fFadeOutStart(80.0f)
  , m_fFadeOutEnd(100.0f)
  , m_fPositionBias(5.0f)
  , m_fMipLevelScale(10.0f)
  , m_fDepthBlurThreshold(2.0f)
{
  m_hNoiseTexture = ezResourceManager::LoadResource<ezTexture2DResource>("Textures/SSAONoise.dds");

  m_hDownscaleShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/DownscaleDepth.ezShader");
  EZ_ASSERT_DEV(m_hDownscaleShader.IsValid(), "Could not load downsample shader!");

  m_hSSAOShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SSAO.ezShader");
  EZ_ASSERT_DEV(m_hSSAOShader.IsValid(), "Could not load SSAO shader!");

  m_hBlurShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SSAOBlur.ezShader");
  EZ_ASSERT_DEV(m_hBlurShader.IsValid(), "Could not load SSAO shader!");

  m_hDownscaleConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezDownscaleDepthConstants>();
  m_hSSAOConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezSSAOConstants>();
}

ezAOPass::~ezAOPass()
{
  if (!m_hSSAOSamplerState.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSSAOSamplerState);
    m_hSSAOSamplerState.Invalidate();
  }

  ezRenderContext::DeleteConstantBufferStorage(m_hDownscaleConstantBuffer);
  m_hDownscaleConstantBuffer.Invalidate();

  ezRenderContext::DeleteConstantBufferStorage(m_hSSAOConstantBuffer);
  m_hSSAOConstantBuffer.Invalidate();
}

bool ezAOPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  if (auto pDepthInput = inputs[m_PinDepthInput.m_uiInputIndex])
  {
    if (!pDepthInput->m_bAllowShaderResourceView)
    {
      ezLog::Error("'{0}' input must allow shader resource view.", GetName());
      return false;
    }

    if (pDepthInput->m_SampleCount != ezGALMSAASampleCount::None)
    {
      ezLog::Error("'{0}' input must be resolved", GetName());
      return false;
    }

    ezGALTextureCreationDescription desc = *pDepthInput;
    desc.m_Format = ezGALResourceFormat::RGHalf;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    ezLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void ezAOPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pDepthInput = inputs[m_PinDepthInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pDepthInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALPass* pGALPass = pDevice->BeginPass(GetName());
  EZ_SCOPE_EXIT(pDevice->EndPass(pGALPass));

  ezUInt32 uiWidth = pDepthInput->m_Desc.m_uiWidth;
  ezUInt32 uiHeight = pDepthInput->m_Desc.m_uiHeight;

  ezUInt32 uiNumMips = 3;
  ezUInt32 uiHzbWidth = ezMath::RoundUp(uiWidth, 1u << uiNumMips);
  ezUInt32 uiHzbHeight = ezMath::RoundUp(uiHeight, 1u << uiNumMips);

  float fHzbScaleX = (float)uiWidth / uiHzbWidth;
  float fHzbScaleY = (float)uiHeight / uiHzbHeight;

  // Find temp targets
  ezGALTextureHandle hzbTexture;
  ezHybridArray<ezVec2, 8> hzbSizes;
  ezHybridArray<ezGALResourceViewHandle, 8> hzbResourceViews;
  ezHybridArray<ezGALRenderTargetViewHandle, 8> hzbRenderTargetViews;

  ezGALTextureHandle tempSSAOTexture;

  {
    {
      ezGALTextureCreationDescription desc;
      desc.m_uiWidth = uiHzbWidth / 2;
      desc.m_uiHeight = uiHzbHeight / 2;
      desc.m_uiMipLevelCount = 3;
      desc.m_Type = ezGALTextureType::Texture2D;
      desc.m_Format = ezGALResourceFormat::RHalf;
      desc.m_bCreateRenderTarget = true;
      desc.m_bAllowShaderResourceView = true;
      desc.m_uiArraySize = pOutput->m_Desc.m_uiArraySize;

      hzbTexture = ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(desc);
    }

    for (ezUInt32 i = 0; i < uiNumMips; ++i)
    {
      uiHzbWidth = uiHzbWidth / 2;
      uiHzbHeight = uiHzbHeight / 2;

      hzbSizes.PushBack(ezVec2((float)uiHzbWidth, (float)uiHzbHeight));

      {
        ezGALResourceViewCreationDescription desc;
        desc.m_hTexture = hzbTexture;
        desc.m_uiMostDetailedMipLevel = i;
        desc.m_uiMipLevelsToUse = 1;
        desc.m_uiArraySize = pOutput->m_Desc.m_uiArraySize;

        hzbResourceViews.PushBack(pDevice->CreateResourceView(desc));
      }

      {
        ezGALRenderTargetViewCreationDescription desc;
        desc.m_hTexture = hzbTexture;
        desc.m_uiMipLevel = i;
        desc.m_uiSliceCount = pOutput->m_Desc.m_uiArraySize;

        hzbRenderTargetViews.PushBack(pDevice->CreateRenderTargetView(desc));
      }
    }

    tempSSAOTexture = ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, ezGALResourceFormat::RGHalf, ezGALMSAASampleCount::None, pOutput->m_Desc.m_uiArraySize);
  }

  // Bind common data
  renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);

  // Mip map passes
  {
    renderViewContext.m_pRenderContext->BindConstantBuffer("ezDownscaleDepthConstants", m_hDownscaleConstantBuffer);
    renderViewContext.m_pRenderContext->BindShader(m_hDownscaleShader);

    CreateSamplerState();
    renderViewContext.m_pRenderContext->BindSamplerState("DepthSampler", m_hSSAOSamplerState);

    for (ezUInt32 i = 0; i < uiNumMips; ++i)
    {
      ezGALResourceViewHandle hInputView;
      ezVec2 pixelSize;

      if (i == 0)
      {
        hInputView = pDevice->GetDefaultResourceView(pDepthInput->m_TextureHandle);
        pixelSize = ezVec2(1.0f / uiWidth, 1.0f / uiHeight);
      }
      else
      {
        hInputView = hzbResourceViews[i - 1];
        pixelSize = ezVec2(1.0f).CompDiv(hzbSizes[i - 1]);
      }

      ezGALRenderTargetViewHandle hOutputView = hzbRenderTargetViews[i];
      ezVec2 targetSize = hzbSizes[i];

      ezGALRenderingSetup renderingSetup;
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, hOutputView);
      renderViewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, ezRectFloat(targetSize.x, targetSize.y));

      ezDownscaleDepthConstants* constants = ezRenderContext::GetConstantBufferData<ezDownscaleDepthConstants>(m_hDownscaleConstantBuffer);
      constants->PixelSize = pixelSize;
      constants->LinearizeDepth = (i == 0);

      renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", hInputView);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

      renderViewContext.m_pRenderContext->EndRendering();
    }
  }

  // Update constants
  {
    renderViewContext.m_pRenderContext->BindConstantBuffer("ezSSAOConstants", m_hSSAOConstantBuffer);

    float fadeOutScale = -1.0f / ezMath::Max(0.001f, (m_fFadeOutEnd - m_fFadeOutStart));
    float fadeOutOffset = -fadeOutScale * m_fFadeOutStart + 1.0f;

    ezSSAOConstants* constants = ezRenderContext::GetConstantBufferData<ezSSAOConstants>(m_hSSAOConstantBuffer);
    constants->TexCoordsScale = ezVec2(fHzbScaleX, fHzbScaleY);
    constants->FadeOutParams = ezVec2(fadeOutScale, fadeOutOffset);
    constants->WorldRadius = m_fRadius;
    constants->MaxScreenSpaceRadius = m_fMaxScreenSpaceRadius;
    constants->Contrast = m_fContrast;
    constants->Intensity = m_fIntensity;
    constants->PositionBias = m_fPositionBias / 1000.0f;
    constants->MipLevelScale = m_fMipLevelScale;
    constants->DepthBlurScale = 1.0f / m_fDepthBlurThreshold;
  }

  // SSAO pass
  {
    ezGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(tempSSAOTexture));
    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginRenderingScope(pGALPass, renderViewContext, renderingSetup, "SSAO");

    renderViewContext.m_pRenderContext->BindShader(m_hSSAOShader);

    renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", pDevice->GetDefaultResourceView(pDepthInput->m_TextureHandle));
    renderViewContext.m_pRenderContext->BindTexture2D("LowResDepthTexture", pDevice->GetDefaultResourceView(hzbTexture));
    renderViewContext.m_pRenderContext->BindSamplerState("DepthSampler", m_hSSAOSamplerState);

    renderViewContext.m_pRenderContext->BindTexture2D("NoiseTexture", m_hNoiseTexture, ezResourceAcquireMode::BlockTillLoaded);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }

  // Blur pass
  {
    ezGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginRenderingScope(pGALPass, renderViewContext, renderingSetup, "Blur");

    renderViewContext.m_pRenderContext->BindShader(m_hBlurShader);
    renderViewContext.m_pRenderContext->BindTexture2D("SSAOTexture", pDevice->GetDefaultResourceView(tempSSAOTexture));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }

  // Return temp targets
  if (!hzbTexture.IsInvalidated())
  {
    ezGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hzbTexture);
  }

  if (!tempSSAOTexture.IsInvalidated())
  {
    ezGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(tempSSAOTexture);
  }
}

void ezAOPass::ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_ClearColor = ezColor::White;

  auto pCommandEncoder = ezRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName());
}

void ezAOPass::SetFadeOutStart(float fStart)
{
  m_fFadeOutStart = ezMath::Clamp(fStart, 0.0f, m_fFadeOutEnd);
}

float ezAOPass::GetFadeOutStart() const
{
  return m_fFadeOutStart;
}

void ezAOPass::SetFadeOutEnd(float fEnd)
{
  if (m_fFadeOutEnd == fEnd)
    return;

  m_fFadeOutEnd = ezMath::Max(fEnd, m_fFadeOutStart);

  if (!m_hSSAOSamplerState.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSSAOSamplerState);
    m_hSSAOSamplerState.Invalidate();
  }
}

float ezAOPass::GetFadeOutEnd() const
{
  return m_fFadeOutEnd;
}

void ezAOPass::CreateSamplerState()
{
  if (m_hSSAOSamplerState.IsInvalidated())
  {
    ezGALSamplerStateCreationDescription desc;
    desc.m_MinFilter = ezGALTextureFilterMode::Point;
    desc.m_MagFilter = ezGALTextureFilterMode::Point;
    desc.m_MipFilter = ezGALTextureFilterMode::Point;
    desc.m_AddressU = ezImageAddressMode::ClampBorder;
    desc.m_AddressV = ezImageAddressMode::ClampBorder;
    desc.m_AddressW = ezImageAddressMode::ClampBorder;
    desc.m_BorderColor = ezColor::White * m_fFadeOutEnd;

    m_hSSAOSamplerState = ezGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_AOPass);
