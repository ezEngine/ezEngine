#include <RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/TonemapPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/TonemapConstants.h>

// clang-format off
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
    EZ_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("LUT1Strength", m_fLut1Strength)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("LUT2Strength", m_fLut2Strength)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_ACCESSOR_PROPERTY("LUT1", GetLUT1TextureFile, SetLUT1TextureFile)->AddAttributes(new ezAssetBrowserAttribute("LUT")),
    EZ_ACCESSOR_PROPERTY("LUT2", GetLUT2TextureFile, SetLUT2TextureFile)->AddAttributes(new ezAssetBrowserAttribute("LUT")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTonemapPass::ezTonemapPass()
  : ezRenderPipelinePass("TonemapPass")
{
  m_hVignettingTexture = ezResourceManager::LoadResource<ezTexture2DResource>("White.color");
  m_hNoiseTexture = ezResourceManager::LoadResource<ezTexture2DResource>("Textures/BlueNoise.dds");

  m_MoodColor = ezColor::Orange;
  m_fMoodStrength = 0.0f;
  m_fSaturation = 1.0f;
  m_fContrast = 1.0f;
  m_fLut1Strength = 1.0f;
  m_fLut2Strength = 0.0f;

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Tonemap.ezShader");
  EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load tonemap shader!");

  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezTonemapConstants>();
}

ezTonemapPass::~ezTonemapPass()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool ezTonemapPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
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
      // if (desc.m_uiWidth != pColorInput->m_uiWidth || desc.m_uiHeight != pColorInput->m_uiHeight)
      //{
      //  ezLog::Error("Render target sizes don't match");
      //  return false;
      //}

      outputs[m_PinOutput.m_uiOutputIndex].SetAsRenderTarget(pColorInput->m_uiWidth, pColorInput->m_uiHeight, desc.m_Format);
    }
    else
    {
      ezLog::Error("View '{0}' does not have a valid color target", view.GetName());
      return false;
    }
  }
  else
  {
    ezLog::Error("No input connected to tone map pass!");
    return false;
  }

  return true;
}

void ezTonemapPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pColorInput = inputs[m_PinColorInput.m_uiInputIndex];
  auto pColorOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pColorInput == nullptr || pColorOutput == nullptr)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Setup render target
  ezGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pColorOutput->m_TextureHandle));

  // Bind render target and viewport
  auto pCommandEncoder = ezRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName());

  // Determine how many LUTs are active
  ezUInt32 numLUTs = 0;
  ezTexture3DResourceHandle luts[2] = {};
  float lutStrengths[2] = {};

  if (m_hLUT1.IsValid())
  {
    luts[numLUTs] = m_hLUT1;
    lutStrengths[numLUTs] = m_fLut1Strength;
    numLUTs++;
  }

  if (m_hLUT2.IsValid())
  {
    luts[numLUTs] = m_hLUT2;
    lutStrengths[numLUTs] = m_fLut2Strength;
    numLUTs++;
  }

  {
    ezTonemapConstants* constants = ezRenderContext::GetConstantBufferData<ezTonemapConstants>(m_hConstantBuffer);
    constants->AutoExposureParams.SetZero();
    constants->MoodColor = m_MoodColor;
    constants->MoodStrength = m_fMoodStrength;
    constants->Saturation = m_fSaturation;
    constants->Lut1Strength = lutStrengths[0];
    constants->Lut2Strength = lutStrengths[1];

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
  renderViewContext.m_pRenderContext->BindTexture2D("VignettingTexture", m_hVignettingTexture, ezResourceAcquireMode::BlockTillLoaded);
  renderViewContext.m_pRenderContext->BindTexture2D("NoiseTexture", m_hNoiseTexture, ezResourceAcquireMode::BlockTillLoaded);
  renderViewContext.m_pRenderContext->BindTexture2D("SceneColorTexture", pDevice->GetDefaultResourceView(pColorInput->m_TextureHandle));
  renderViewContext.m_pRenderContext->BindTexture2D("BloomTexture", hBloomTextureView);
  renderViewContext.m_pRenderContext->BindTexture3D("Lut1Texture", luts[0]);
  renderViewContext.m_pRenderContext->BindTexture3D("Lut2Texture", luts[1]);

  ezTempHashedString sLUTModeValues[3] = {"LUT_MODE_NONE", "LUT_MODE_ONE", "LUT_MODE_TWO"};
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LUT_MODE", sLUTModeValues[numLUTs]);

  renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
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


void ezTonemapPass::SetLUT1TextureFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hLUT1 = ezResourceManager::LoadResource<ezTexture3DResource>(szFile);
  }
}

const char* ezTonemapPass::GetLUT1TextureFile() const
{
  if (!m_hLUT1.IsValid())
    return "";

  return m_hLUT1.GetResourceID();
}

void ezTonemapPass::SetLUT2TextureFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hLUT2 = ezResourceManager::LoadResource<ezTexture3DResource>(szFile);
  }
}

const char* ezTonemapPass::GetLUT2TextureFile() const
{
  if (!m_hLUT2.IsValid())
    return "";

  return m_hLUT2.GetResourceID();
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TonemapPass);
