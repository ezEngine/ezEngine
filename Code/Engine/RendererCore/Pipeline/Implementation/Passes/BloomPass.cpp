#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/BloomPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BloomConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBloomPass, 1, ezRTTIDefaultAllocator<ezBloomPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_PinInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.01f, 1.0f)),
    EZ_MEMBER_PROPERTY("Threshold", m_fThreshold)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new ezDefaultValueAttribute(0.3f)),
    EZ_MEMBER_PROPERTY("InnerTintColor", m_InnerTintColor),
    EZ_MEMBER_PROPERTY("MidTintColor", m_MidTintColor),
    EZ_MEMBER_PROPERTY("OuterTintColor", m_OuterTintColor),
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

ezBloomPass::ezBloomPass()
  : ezRenderPipelinePass("BloomPass", true)
{
  // Load shader.
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Bloom.ezShader");
  EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load bloom shader!");

  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezBloomConstants>();

  const ezGALDeviceCapabilities& caps = ezGALDevice::GetDefaultDevice()->GetCapabilities();
  const bool bSupportsRG11B10Float = caps.m_FormatSupport[ezGALResourceFormat::RG11B10Float].AreAllSet(ezGALResourceFormatSupport::RenderTarget | ezGALResourceFormatSupport::Texture);
  m_TextureFormat = bSupportsRG11B10Float ? ezGALResourceFormat::RG11B10Float : ezGALResourceFormat::RGBAHalf;
}

ezBloomPass::~ezBloomPass()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool ezBloomPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinInput.m_uiInputIndex])
  {
    if (!inputs[m_PinInput.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      ezLog::Error("'{0}' input must allow shader resource view.", GetName());
      return false;
    }

    // Output is half-res
    ezGALTextureCreationDescription desc = *inputs[m_PinInput.m_uiInputIndex];
    desc.m_uiWidth = desc.m_uiWidth / 2;
    desc.m_uiHeight = desc.m_uiHeight / 2;
    desc.m_Format = m_TextureFormat;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    ezLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void ezBloomPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pColorInput = inputs[m_PinInput.m_uiInputIndex];
  auto pColorOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pColorInput == nullptr || pColorOutput == nullptr)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezUInt32 uiWidth = pColorInput->m_Desc.m_uiWidth;
  ezUInt32 uiHeight = pColorInput->m_Desc.m_uiHeight;
  bool bFastDownscale = ezMath::IsEven(uiWidth) && ezMath::IsEven(uiHeight);

  const float fMaxRes = (float)ezMath::Max(uiWidth, uiHeight);
  const float fRadius = ezMath::Clamp(m_fRadius, 0.01f, 1.0f);
  const float fDownscaledSize = 4.0f / fRadius;
  const float fNumBlurPasses = ezMath::Log2(fMaxRes / fDownscaledSize);
  const ezUInt32 uiNumBlurPasses = (ezUInt32)ezMath::Ceil(fNumBlurPasses);

  // Find temp targets
  ezHybridArray<ezVec2, 8> targetSizes;
  ezHybridArray<ezGALTextureHandle, 8> tempDownscaleTextures;
  ezHybridArray<ezGALTextureHandle, 8> tempUpscaleTextures;

  for (ezUInt32 i = 0; i < uiNumBlurPasses; ++i)
  {
    uiWidth = ezMath::Max(uiWidth / 2, 1u);
    uiHeight = ezMath::Max(uiHeight / 2, 1u);
    targetSizes.PushBack(ezVec2((float)uiWidth, (float)uiHeight));
    auto uiSliceCount = pColorOutput->m_Desc.m_uiArraySize;

    tempDownscaleTextures.PushBack(ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, m_TextureFormat, ezGALMSAASampleCount::None, uiSliceCount));

    // biggest upscale target is the output and lowest is not needed
    if (i > 0 && i < uiNumBlurPasses - 1)
    {
      tempUpscaleTextures.PushBack(ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, m_TextureFormat, ezGALMSAASampleCount::None, uiSliceCount));
    }
    else
    {
      tempUpscaleTextures.PushBack(ezGALTextureHandle());
    }
  }

  renderViewContext.m_pRenderContext->BindConstantBuffer("ezBloomConstants", m_hConstantBuffer);
  renderViewContext.m_pRenderContext->BindShader(m_hShader);

  renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);

  // Downscale passes
  {
    ezTempHashedString sInitialDownscale = "BLOOM_PASS_MODE_INITIAL_DOWNSCALE";
    ezTempHashedString sInitialDownscaleFast = "BLOOM_PASS_MODE_INITIAL_DOWNSCALE_FAST";
    ezTempHashedString sDownscale = "BLOOM_PASS_MODE_DOWNSCALE";
    ezTempHashedString sDownscaleFast = "BLOOM_PASS_MODE_DOWNSCALE_FAST";

    for (ezUInt32 i = 0; i < uiNumBlurPasses; ++i)
    {
      ezGALTextureHandle hInput;
      if (i == 0)
      {
        hInput = pColorInput->m_TextureHandle;
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", bFastDownscale ? sInitialDownscaleFast : sInitialDownscale);
      }
      else
      {
        hInput = tempDownscaleTextures[i - 1];
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", bFastDownscale ? sDownscaleFast : sDownscale);
      }

      ezGALTextureHandle hOutput = tempDownscaleTextures[i];
      ezVec2 targetSize = targetSizes[i];

      ezGALRenderingSetup renderingSetup;
      renderingSetup.SetColorTarget(0, pDevice->GetDefaultRenderTargetView(hOutput));
      renderViewContext.m_pRenderContext->BeginRendering(renderingSetup, ezRectFloat(targetSize.x, targetSize.y), "Downscale", renderViewContext.m_pCamera->IsStereoscopic());

      ezColor tintColor = (i == uiNumBlurPasses - 1) ? ezColor(m_OuterTintColor) : ezColor::White;
      UpdateConstantBuffer(ezVec2(1.0f).CompDiv(targetSize), tintColor);

      renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(hInput));
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

      renderViewContext.m_pRenderContext->EndRendering();

      bFastDownscale = ezMath::IsEven((ezInt32)targetSize.x) && ezMath::IsEven((ezInt32)targetSize.y);
    }
  }

  // Upscale passes
  {
    const float fBlurRadius = 2.0f * fNumBlurPasses / uiNumBlurPasses;
    const float fMidPass = (uiNumBlurPasses - 1.0f) / 2.0f;

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", "BLOOM_PASS_MODE_UPSCALE");

    for (ezUInt32 i = uiNumBlurPasses - 1; i-- > 0;)
    {
      ezGALTextureHandle hNextInput = tempDownscaleTextures[i];
      ezGALTextureHandle hInput;
      if (i == uiNumBlurPasses - 2)
      {
        hInput = tempDownscaleTextures[i + 1];
      }
      else
      {
        hInput = tempUpscaleTextures[i + 1];
      }

      ezGALTextureHandle hOutput;
      if (i == 0)
      {
        hOutput = pColorOutput->m_TextureHandle;
      }
      else
      {
        hOutput = tempUpscaleTextures[i];
      }

      ezVec2 targetSize = targetSizes[i];

      ezGALRenderingSetup renderingSetup;
      renderingSetup.SetColorTarget(0, pDevice->GetDefaultRenderTargetView(hOutput));
      renderViewContext.m_pRenderContext->BeginRendering(renderingSetup, ezRectFloat(targetSize.x, targetSize.y), "Upscale", renderViewContext.m_pCamera->IsStereoscopic());

      ezColor tintColor;
      float fPass = (float)i;
      if (fPass < fMidPass)
      {
        tintColor = ezMath::Lerp<ezColor>(m_InnerTintColor, m_MidTintColor, fPass / fMidPass);
      }
      else
      {
        tintColor = ezMath::Lerp<ezColor>(m_MidTintColor, m_OuterTintColor, (fPass - fMidPass) / fMidPass);
      }

      UpdateConstantBuffer(ezVec2(fBlurRadius).CompDiv(targetSize), tintColor);

      renderViewContext.m_pRenderContext->BindTexture2D("NextColorTexture", pDevice->GetDefaultResourceView(hNextInput));
      renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(hInput));
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

      renderViewContext.m_pRenderContext->EndRendering();
    }
  }

  // Return temp targets
  for (auto hTexture : tempDownscaleTextures)
  {
    if (!hTexture.IsInvalidated())
    {
      ezGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hTexture);
    }
  }

  for (auto hTexture : tempUpscaleTextures)
  {
    if (!hTexture.IsInvalidated())
    {
      ezGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hTexture);
    }
  }
}

void ezBloomPass::ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pColorOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pColorOutput == nullptr)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezGALRenderingSetup renderingSetup;
  renderingSetup.SetColorTarget(0, pDevice->GetDefaultRenderTargetView(pColorOutput->m_TextureHandle));
  renderingSetup.SetClearColor(0, ezColor::Black);

  auto pCommandEncoder = ezRenderContext::BeginRenderingScope(renderViewContext, renderingSetup, "Clear");
}

ezResult ezBloomPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fRadius;
  inout_stream << m_fThreshold;
  inout_stream << m_fIntensity;
  inout_stream << m_InnerTintColor;
  inout_stream << m_MidTintColor;
  inout_stream << m_OuterTintColor;
  return EZ_SUCCESS;
}

ezResult ezBloomPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fRadius;
  inout_stream >> m_fThreshold;
  inout_stream >> m_fIntensity;
  inout_stream >> m_InnerTintColor;
  inout_stream >> m_MidTintColor;
  inout_stream >> m_OuterTintColor;
  return EZ_SUCCESS;
}

void ezBloomPass::UpdateConstantBuffer(ezVec2 pixelSize, const ezColor& tintColor)
{
  ezBloomConstants* constants = ezRenderContext::GetConstantBufferData<ezBloomConstants>(m_hConstantBuffer);
  constants->PixelSize = pixelSize;
  constants->BloomThreshold = m_fThreshold;
  constants->BloomIntensity = m_fIntensity;

  constants->TintColor = tintColor;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_BloomPass);
