#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
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
    EZ_RESOURCE_MEMBER_PROPERTY("VignettingTexture", m_hVignettingTexture)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_2D"), new ezDefaultValueAttribute("White.color")),
    EZ_MEMBER_PROPERTY("MoodColor", m_MoodColor)->AddAttributes(new ezDefaultValueAttribute(ezColor::Orange)),
    EZ_MEMBER_PROPERTY("MoodStrength", m_fMoodStrength)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Saturation", m_fSaturation)->AddAttributes(new ezClampValueAttribute(0.0f, 2.0f), new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("LUT1Strength", m_fLut1Strength)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("LUT2Strength", m_fLut2Strength)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_RESOURCE_MEMBER_PROPERTY("LUT1", m_hLUT1)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_3D")),
    EZ_RESOURCE_MEMBER_PROPERTY("LUT2", m_hLUT2)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_3D")),
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

ezTonemapPass::ezTonemapPass()
  : ezRenderPipelinePass("TonemapPass", true)
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
  const ezGALRenderTargets& renderTargets = view.GetActiveRenderTargets();

  // Color
  auto pColorInput = inputs[m_PinColorInput.m_uiInputIndex];
  if (pColorInput != nullptr)
  {
    if (const ezGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hRTs[0]))
    {
      const ezGALTextureCreationDescription& desc = pTexture->GetDescription();
      // if (desc.m_uiWidth != pColorInput->m_uiWidth || desc.m_uiHeight != pColorInput->m_uiHeight)
      //{
      //  ezLog::Error("Render target sizes don't match");
      //  return false;
      //}

      outputs[m_PinOutput.m_uiOutputIndex].SetAsRenderTarget(pColorInput->m_uiWidth, pColorInput->m_uiHeight, desc.m_Format);
      outputs[m_PinOutput.m_uiOutputIndex].m_Type = pColorInput->m_Type;
      outputs[m_PinOutput.m_uiOutputIndex].m_uiArraySize = pColorInput->m_uiArraySize;
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
  renderingSetup.SetColorTarget(0, pDevice->GetDefaultRenderTargetView(pColorOutput->m_TextureHandle));

  // Bind render target and viewport
  auto pCommandEncoder = ezRenderContext::BeginRenderingScope(renderViewContext, renderingSetup, GetName(), renderViewContext.m_pCamera->IsStereoscopic());

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

  ezGALTextureHandle hBloomTextureView;
  auto pBloomInput = inputs[m_PinBloomInput.m_uiInputIndex];
  if (pBloomInput != nullptr)
  {
    hBloomTextureView = pBloomInput->m_TextureHandle;
  }

  renderViewContext.m_pRenderContext->BindShader(m_hShader);

  ezBindGroupBuilder& bindGroup = ezRenderContext::GetDefaultInstance()->GetBindGroup();
  bindGroup.BindBuffer("ezTonemapConstants", m_hConstantBuffer);
  renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
  bindGroup.BindTexture("VignettingTexture", m_hVignettingTexture, ezResourceAcquireMode::BlockTillLoaded);
  bindGroup.BindTexture("NoiseTexture", m_hNoiseTexture, ezResourceAcquireMode::BlockTillLoaded);
  bindGroup.BindTexture("SceneColorTexture", pColorInput->m_TextureHandle);
  bindGroup.BindTexture("BloomTexture", hBloomTextureView);
  bindGroup.BindTexture("Lut1Texture", luts[0]);
  bindGroup.BindTexture("Lut2Texture", luts[1]);

  ezTempHashedString sLUTModeValues[3] = {"LUT_MODE_NONE", "LUT_MODE_ONE", "LUT_MODE_TWO"};
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LUT_MODE", sLUTModeValues[numLUTs]);

  renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
}

ezResult ezTonemapPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  ezStringBuilder sTemp = GetVignettingTextureFile();
  inout_stream << sTemp;
  inout_stream << m_MoodColor;
  inout_stream << m_fMoodStrength;
  inout_stream << m_fSaturation;
  inout_stream << m_fContrast;
  inout_stream << m_fLut1Strength;
  inout_stream << m_fLut2Strength;
  sTemp = GetLUT1TextureFile();
  inout_stream << sTemp;
  sTemp = GetLUT2TextureFile();
  inout_stream << sTemp;
  return EZ_SUCCESS;
}

ezResult ezTonemapPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  ezStringBuilder sTemp;
  inout_stream >> sTemp;
  SetVignettingTextureFile(sTemp);
  inout_stream >> m_MoodColor;
  inout_stream >> m_fMoodStrength;
  inout_stream >> m_fSaturation;
  inout_stream >> m_fContrast;
  inout_stream >> m_fLut1Strength;
  inout_stream >> m_fLut2Strength;
  inout_stream >> sTemp;
  SetLUT1TextureFile(sTemp);
  inout_stream >> sTemp;
  SetLUT2TextureFile(sTemp);
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TonemapPass);
