#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/Passes/TonemapPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/TonemapConstants.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTonemapPass, 1, ezRTTIDefaultAllocator<ezTonemapPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColorInput),
    EZ_MEMBER_PROPERTY("Bloom", m_PinBloomInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_ACCESSOR_PROPERTY("VignettingTexture", GetVignettingTextureFile, SetVignettingTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_MEMBER_PROPERTY("MoodColor", m_MoodColor)->AddAttributes(new ezDefaultValueAttribute(ezColor::Orange)),
    EZ_MEMBER_PROPERTY("MoodStrength", m_fMoodStrength)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Saturation", m_fSaturation)->AddAttributes(new ezClampValueAttribute(0.0f, 2.0f), new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f))
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezTonemapPass::ezTonemapPass() : ezRenderPipelinePass("TonemapPass")
{
  m_hVignettingTexture = ezResourceManager::LoadResource<ezTexture2DResource>("White.color");
  m_hNoiseTexture = ezResourceManager::LoadResource<ezTexture2DResource>("Textures/BlueNoise.dds");

  m_MoodColor = ezColor::Orange;
  m_fMoodStrength = 0.0f;
  m_fSaturation = 1.0f;
  m_fContrast = 1.0f;

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Tonemap.ezShader");
  EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load tonemap shader!");

  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezTonemapConstants>();
}

ezTonemapPass::~ezTonemapPass()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool ezTonemapPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription*const> inputs,
  ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Color
  auto pColorInput = inputs[m_PinColorInput.m_uiInputIndex];
  if (pColorInput != nullptr)
  {
    if (const ezGALRenderTargetView* pTargetView = pDevice->GetRenderTargetView(view.GetRenderTargetSetup().GetRenderTarget(0)))
    {
      const ezGALTexture* pTexture = pTargetView->GetTexture();
      const ezGALTextureCreationDescription& desc = pTexture->GetDescription();
      if (desc.m_uiWidth != pColorInput->m_uiWidth || desc.m_uiHeight != pColorInput->m_uiHeight)
      {
        ezLog::Error("Render target sizes don't match");
        return false;
      }

      outputs[m_PinOutput.m_uiOutputIndex].SetAsRenderTarget(desc.m_uiWidth, desc.m_uiHeight, desc.m_Format);
    }
    else
    {
      ezLog::Error("View '{0}' does not have a valid color target", view.GetName());
      return false;
    }
  }
  else
  {
    ezLog::Error("No input connected to tonemap pass!");
    return false;
  }

  return true;
}

void ezTonemapPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pColorInput = inputs[m_PinColorInput.m_uiInputIndex];
  auto pColorOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pColorInput == nullptr || pColorOutput == nullptr)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Setup render target
  ezGALRenderTagetSetup renderTargetSetup;
  renderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pColorOutput->m_TextureHandle));

  renderViewContext.m_pRenderContext->SetViewportAndRenderTargetSetup(renderViewContext.m_pViewData->m_ViewPortRect, renderTargetSetup);

  {
    ezTonemapConstants* constants = ezRenderContext::GetConstantBufferData<ezTonemapConstants>(m_hConstantBuffer);
    constants->AutoExposureParams.SetZero();
    constants->MoodColor = m_MoodColor;
    constants->MoodStrength = m_fMoodStrength;
    constants->Saturation = m_fSaturation;

    // Pre-calculate factors of a s-shaped polynomial-function
    const float m = (0.5f - 0.5f * m_fContrast) / (0.5f + 0.5f * m_fContrast);
    const float a = 2.0f * m - 2.0f;
    const float b = -3.0f * m + 3.0f;

    constants->ContrastParams = ezVec4(a, b, m, 0.0f);
  }

  ezGALResourceViewHandle hBloomTextureView;
  auto pBloomInput = inputs[m_PinBloomInput.m_uiInputIndex];
  if (pBloomInput != nullptr)
  {
    hBloomTextureView = pDevice->GetDefaultResourceView(pBloomInput->m_TextureHandle);
  }

  renderViewContext.m_pRenderContext->BindShader(m_hShader);
  renderViewContext.m_pRenderContext->BindConstantBuffer("ezTonemapConstants", m_hConstantBuffer);
  renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
  renderViewContext.m_pRenderContext->BindTexture2D(ezGALShaderStage::PixelShader, "VignettingTexture", m_hVignettingTexture, ezResourceAcquireMode::NoFallback);
  renderViewContext.m_pRenderContext->BindTexture2D(ezGALShaderStage::PixelShader, "NoiseTexture", m_hNoiseTexture, ezResourceAcquireMode::NoFallback);
  renderViewContext.m_pRenderContext->BindTexture2D(ezGALShaderStage::PixelShader, "SceneColorTexture", pDevice->GetDefaultResourceView(pColorInput->m_TextureHandle));
  renderViewContext.m_pRenderContext->BindTexture2D(ezGALShaderStage::PixelShader, "BloomTexture", hBloomTextureView);

  renderViewContext.m_pRenderContext->DrawMeshBuffer();
}

void ezTonemapPass::SetVignettingTextureFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hVignettingTexture = ezResourceManager::LoadResource<ezTexture2DResource>(szFile);
  }
}

const char* ezTonemapPass::GetVignettingTextureFile() const
{
  if (!m_hVignettingTexture.IsValid())
    return "";

  return m_hVignettingTexture.GetResourceID();
}
