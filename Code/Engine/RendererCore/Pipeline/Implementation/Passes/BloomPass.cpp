#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/Passes/BloomPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererFoundation/Context/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BloomConstants.h>

namespace
{
  ezProfilingId g_BloomDownscaleId = ezProfilingSystem::CreateId("Downscale");
  ezProfilingId g_BloomUpscaleId = ezProfilingSystem::CreateId("Upscale");
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBloomPass, 1, ezRTTIDefaultAllocator<ezBloomPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_PinInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_MEMBER_PROPERTY("NumBlurPasses", m_uiNumBlurPasses)->AddAttributes(new ezDefaultValueAttribute(5), new ezClampValueAttribute(3, 8)),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.1f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("Threshold", m_fThreshold)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new ezDefaultValueAttribute(0.5f)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezBloomPass::ezBloomPass()
  : ezRenderPipelinePass("BloomPass")
  , m_uiNumBlurPasses(5)
  , m_fRadius(0.1f)
  , m_fThreshold(1.0f)
  , m_fIntensity(0.5f)
{
  {
    // Load shader.
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Bloom.ezShader");
    EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load bloom shader!");
  }

  {
    m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezBloomConstants>();
  }
}

ezBloomPass::~ezBloomPass()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool ezBloomPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription*const> inputs,
  ezArrayPtr<ezGALTextureCreationDescription> outputs)
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
    desc.m_Format = ezGALResourceFormat::RG11B10Float;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    ezLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void ezBloomPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pColorInput = inputs[m_PinInput.m_uiInputIndex];
  auto pColorOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pColorInput == nullptr || pColorOutput == nullptr)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  ezUInt32 uiWidth = pColorInput->m_Desc.m_uiWidth;
  ezUInt32 uiHeight = pColorInput->m_Desc.m_uiHeight;

  // Find temp targets
  ezHybridArray<ezVec2, 8> targetSizes;
  ezHybridArray<ezGALTextureHandle, 8> tempDownscaleTextures;
  ezHybridArray<ezGALTextureHandle, 8> tempUpscaleTextures;

  for (ezUInt32 i = 0; i < m_uiNumBlurPasses; ++i)
  {
    uiWidth = uiWidth / 2;
    uiHeight = uiHeight / 2;
    targetSizes.PushBack(ezVec2((float)uiWidth, (float)uiHeight));

    tempDownscaleTextures.PushBack(ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, ezGALResourceFormat::RG11B10Float));

    // biggest upscale target is the output and lowest is not needed
    if (i > 0 && i < m_uiNumBlurPasses - 1)
    {
      tempUpscaleTextures.PushBack(ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, ezGALResourceFormat::RG11B10Float));
    }
    else
    {
      tempUpscaleTextures.PushBack(ezGALTextureHandle());
    }
  }

  renderViewContext.m_pRenderContext->BindConstantBuffer("ezBloomConstants", m_hConstantBuffer);
  renderViewContext.m_pRenderContext->BindShader(m_hShader);
  renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);

  ezGALRenderTagetSetup renderTargetSetup;

  // Downscale passes
  {
    EZ_PROFILE_AND_MARKER(pGALContext, g_BloomDownscaleId);

    for (ezUInt32 i = 0; i < m_uiNumBlurPasses; ++i)
    {
      ezGALTextureHandle hInput;
      if (i == 0)
      {
        hInput = pColorInput->m_TextureHandle;
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", "INITIAL_DOWNSCALE");
      }
      else
      {
        hInput = tempDownscaleTextures[i - 1];
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", "DOWNSCALE");
      }

      ezGALTextureHandle hOutput = tempDownscaleTextures[i];
      ezVec2 targetSize = targetSizes[i];

      renderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(hOutput));
      pGALContext->SetRenderTargetSetup(renderTargetSetup);
      pGALContext->SetViewport(ezRectFloat(targetSize.x, targetSize.y));

      UpdateConstantBuffer(ezVec2(1.0f).CompDiv(targetSize), ezColor::White);

      renderViewContext.m_pRenderContext->BindTexture(ezGALShaderStage::PixelShader, "ColorTexture", pDevice->GetDefaultResourceView(hInput));
      renderViewContext.m_pRenderContext->DrawMeshBuffer();
    }
  }

  // Upscale passes
  {
    EZ_PROFILE_AND_MARKER(pGALContext, g_BloomUpscaleId);

    const float fMaxRadius = 2.0f; // more than 2 pixels wide blur results in holes
    const float fRadius = ezMath::Min(m_fRadius * targetSizes[m_uiNumBlurPasses - 1].x * 0.5f, fMaxRadius);

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", "UPSCALE");

    for (ezUInt32 i = m_uiNumBlurPasses - 1; i-- > 0;)
    {
      ezGALTextureHandle hNextInput = tempDownscaleTextures[i];
      ezGALTextureHandle hInput;
      if (i == m_uiNumBlurPasses - 2)
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

      renderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(hOutput));
      pGALContext->SetRenderTargetSetup(renderTargetSetup);
      pGALContext->SetViewport(ezRectFloat(targetSize.x, targetSize.y));

      UpdateConstantBuffer(ezVec2(fRadius * 2.0f).CompDiv(targetSize), ezColor::White);

      renderViewContext.m_pRenderContext->BindTexture(ezGALShaderStage::PixelShader, "NextColorTexture", pDevice->GetDefaultResourceView(hNextInput));
      renderViewContext.m_pRenderContext->BindTexture(ezGALShaderStage::PixelShader, "ColorTexture", pDevice->GetDefaultResourceView(hInput));
      renderViewContext.m_pRenderContext->DrawMeshBuffer();
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
  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  ezGALRenderTagetSetup renderTargetSetup;
  renderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pColorOutput->m_TextureHandle));
  pGALContext->SetRenderTargetSetup(renderTargetSetup);

  pGALContext->Clear(ezColor::Black);
}

void ezBloomPass::UpdateConstantBuffer(ezVec2 pixelSize, const ezColor& tintColor)
{
  ezBloomConstants* constants = ezRenderContext::GetConstantBufferData<ezBloomConstants>(m_hConstantBuffer);
  constants->PixelSize = pixelSize;
  constants->BloomThreshold = m_fThreshold;
  constants->BloomIntensity = m_fIntensity;

  constants->TintColor = tintColor;
}
