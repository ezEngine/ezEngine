#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/Passes/TonemapPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/TonemapConstants.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTonemapPass, 1, ezRTTIDefaultAllocator<ezTonemapPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_PinInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_ACCESSOR_PROPERTY("VignettingTexture", GetVignettingTextureFile, SetVignettingTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_MEMBER_PROPERTY("MoodColor", m_MoodColor)->AddAttributes(new ezDefaultValueAttribute(ezColor::Orange)),
    EZ_MEMBER_PROPERTY("MoodStrength", m_fMoodStrength),
    EZ_MEMBER_PROPERTY("Saturation", m_fSaturation)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new ezDefaultValueAttribute(1.0f))
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezTonemapPass::ezTonemapPass() : ezRenderPipelinePass("TonemapPass")
{
  m_hVignettingTexture = ezResourceManager::LoadResource<ezTextureResource>("White.color");

  m_MoodColor = ezColor::Orange;
  m_fMoodStrength = 0.0f;
  m_fSaturation = 1.0f;
  m_fContrast = 1.0f;

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Tonemap.ezShader");
  EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load tonemap shader!");
  
  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<TonemapConstants>();
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
  auto pColorInput = inputs[m_PinInput.m_uiInputIndex];
  if (pColorInput != nullptr)
  {
    if (const ezGALRenderTargetView* pTargetView = pDevice->GetRenderTargetView(view.GetRenderTargetSetup().GetRenderTarget(0)))
    {
      if (const ezGALTexture* pTexture = pDevice->GetTexture(pTargetView->GetDescription().m_hTexture))
      {
        const ezGALTextureCreationDescription& desc = pTexture->GetDescription();
        if (desc.m_uiWidth != pColorInput->m_uiWidth || desc.m_uiHeight != pColorInput->m_uiHeight)
        {
          ezLog::Error("Render target sizes don't match");
          return false;
        }

        outputs[m_PinOutput.m_uiOutputIndex] = desc;
      }
    }
    else
    {
      ezLog::Error("View '%s' does not have a valid color target", view.GetName());
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
  auto pColorInput = inputs[m_PinInput.m_uiInputIndex];
  auto pColorOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pColorInput == nullptr || pColorOutput == nullptr)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  // Setup render target
  ezGALRenderTagetSetup renderTargetSetup;
  renderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pColorOutput->m_TextureHandle));
  
  pGALContext->SetRenderTargetSetup(renderTargetSetup);

  {
    TonemapConstants* constants = ezRenderContext::GetConstantBufferData<TonemapConstants>(m_hConstantBuffer);
    constants->ExposureBias = 1.0f;
    constants->MoodColor = m_MoodColor;
    constants->MoodStrength = m_fMoodStrength;
    constants->Saturation = m_fSaturation;
    constants->Contrast = m_fContrast;
  }
 
  renderViewContext.m_pRenderContext->BindShader(m_hShader);
  renderViewContext.m_pRenderContext->BindConstantBuffer("TonemapConstants", m_hConstantBuffer);
  renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
  renderViewContext.m_pRenderContext->BindTexture(ezGALShaderStage::PixelShader, "VignettingTexture", m_hVignettingTexture);
  renderViewContext.m_pRenderContext->BindTexture(ezGALShaderStage::PixelShader, "SceneColorTexture", pDevice->GetDefaultResourceView(pColorInput->m_TextureHandle), ezGALSamplerStateHandle());

  renderViewContext.m_pRenderContext->DrawMeshBuffer();

  ///\ todo generalize prevention of resource hazards somehow?
  // Prevent shader resource hazard in DX11
  renderViewContext.m_pRenderContext->BindTexture(ezGALShaderStage::PixelShader, "SceneColorTexture", ezGALResourceViewHandle(), ezGALSamplerStateHandle());
  renderViewContext.m_pRenderContext->ApplyContextStates();
  pGALContext->Flush();
}

void ezTonemapPass::SetVignettingTextureFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hVignettingTexture = ezResourceManager::LoadResource<ezTextureResource>(szFile);
  }
}

const char* ezTonemapPass::GetVignettingTextureFile() const
{
  if (!m_hVignettingTexture.IsValid())
    return "";

  return m_hVignettingTexture.GetResourceID();
}
