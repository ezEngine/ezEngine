#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
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
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Post Processing")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAOPass::ezAOPass()
  : ezRenderPipelinePass("AOPass", true)

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
  ezHybridArray<ezGALTextureRange, 8> hzbResourceViews;
  ezHybridArray<ezGALRenderTargetViewHandle, 8> hzbRenderTargetViews;

  ezGALTextureHandle tempSSAOTexture;

  {
    {
      ezGALTextureCreationDescription desc;
      desc.m_uiWidth = uiHzbWidth / 2;
      desc.m_uiHeight = uiHzbHeight / 2;
      desc.m_uiMipLevelCount = 3;
      desc.m_Type = ezGALTextureType::Texture2DArray;
      desc.m_Format = ezGALResourceFormat::RHalf;
      desc.m_bAllowRenderTargetView = true;
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
        ezGALTextureRange desc;
        desc.m_uiBaseMipLevel = i;
        desc.m_uiMipLevels = 1;
        desc.m_uiArraySlices = pOutput->m_Desc.m_uiArraySize;
        hzbResourceViews.PushBack(desc);
      }

      {
        ezGALRenderTargetViewCreationDescription desc;
        desc.m_hTexture = hzbTexture;
        desc.m_uiMipLevel = i;
        desc.m_uiSliceCount = pOutput->m_Desc.m_uiArraySize;

        hzbRenderTargetViews.PushBack(pDevice->CreateRenderTargetView(desc));
      }
    }

    tempSSAOTexture = ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, ezGALResourceFormat::RGHalf, ezGALMSAASampleCount::None, pOutput->m_Desc.m_uiArraySize, ezGALTextureType::Texture2DArray);
  }

  // Mip map passes
  {
    CreateSamplerState();

    for (ezUInt32 i = 0; i < uiNumMips; ++i)
    {
      ezGALTextureHandle hInputView = i == 0 ? pDepthInput->m_TextureHandle : hzbTexture;
      ezVec2 pixelSize;
      ezGALTextureRange range;

      if (i == 0)
      {
        pixelSize = ezVec2(1.0f / uiWidth, 1.0f / uiHeight);
      }
      else
      {
        range = hzbResourceViews[i - 1];
        pixelSize = ezVec2(1.0f).CompDiv(hzbSizes[i - 1]);
      }

      ezGALRenderTargetViewHandle hOutputView = hzbRenderTargetViews[i];
      ezVec2 targetSize = hzbSizes[i];

      ezGALRenderingSetup renderingSetup;
      renderingSetup.SetColorTarget(0, hOutputView);
      renderViewContext.m_pRenderContext->BeginRendering(renderingSetup, ezRectFloat(targetSize.x, targetSize.y), "SSAOMipMaps", renderViewContext.m_pCamera->IsStereoscopic());

      ezDownscaleDepthConstants* constants = ezRenderContext::GetConstantBufferData<ezDownscaleDepthConstants>(m_hDownscaleConstantBuffer);
      constants->PixelSize = pixelSize;
      constants->FadeOutEnd = m_fFadeOutEnd;
      constants->LinearizeDepth = (i == 0);

      ezBindGroupBuilder& bindGroup = ezRenderContext::GetDefaultInstance()->GetBindGroup();
      bindGroup.BindBuffer("ezDownscaleDepthConstants", m_hDownscaleConstantBuffer);
      renderViewContext.m_pRenderContext->BindShader(m_hDownscaleShader);

      bindGroup.BindTexture("DepthTexture", hInputView, range);
      bindGroup.BindSampler("DepthSampler", m_hSSAOSamplerState);

      renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

      renderViewContext.m_pRenderContext->EndRendering();
    }
  }

  // Update constants
  {
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
    constants->FadeOutEnd = m_fFadeOutEnd;
  }

  // SSAO pass
  {
    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, pDevice->GetDefaultRenderTargetView(tempSSAOTexture));
    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginRenderingScope(renderViewContext, renderingSetup, "SSAO", renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->BindShader(m_hSSAOShader);
    ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
    bindGroup.BindBuffer("ezSSAOConstants", m_hSSAOConstantBuffer);
    bindGroup.BindTexture("DepthTexture", pDepthInput->m_TextureHandle);
    bindGroup.BindTexture("LowResDepthTexture", hzbTexture);
    bindGroup.BindSampler("DepthSampler", m_hSSAOSamplerState);
    bindGroup.BindTexture("NoiseTexture", m_hNoiseTexture, ezResourceAcquireMode::BlockTillLoaded);

    renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }

  // Blur pass
  {
    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginRenderingScope(renderViewContext, renderingSetup, "Blur", renderViewContext.m_pCamera->IsStereoscopic());

    ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
    renderViewContext.m_pRenderContext->BindShader(m_hBlurShader);
    bindGroup.BindBuffer("ezSSAOConstants", m_hSSAOConstantBuffer);
    bindGroup.BindTexture("SSAOTexture", tempSSAOTexture);

    renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

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
  renderingSetup.SetColorTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
  renderingSetup.SetClearColor(0, ezColor::White);

  auto pCommandEncoder = ezRenderContext::BeginRenderingScope(renderViewContext, renderingSetup, GetName());
}

ezResult ezAOPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fRadius;
  inout_stream << m_fMaxScreenSpaceRadius;
  inout_stream << m_fContrast;
  inout_stream << m_fIntensity;
  inout_stream << m_fFadeOutStart;
  inout_stream << m_fFadeOutEnd;
  inout_stream << m_fPositionBias;
  inout_stream << m_fMipLevelScale;
  inout_stream << m_fDepthBlurThreshold;
  return EZ_SUCCESS;
}

ezResult ezAOPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fRadius;
  inout_stream >> m_fMaxScreenSpaceRadius;
  inout_stream >> m_fContrast;
  inout_stream >> m_fIntensity;
  inout_stream >> m_fFadeOutStart;
  inout_stream >> m_fFadeOutEnd;
  inout_stream >> m_fPositionBias;
  inout_stream >> m_fMipLevelScale;
  inout_stream >> m_fDepthBlurThreshold;
  return EZ_SUCCESS;
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
