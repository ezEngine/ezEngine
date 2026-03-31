#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Lights/ClusteredDataExtractor.h>
#include <RendererCore/Pipeline/Passes/LightShaftsPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include <Shaders/Pipeline/LightShaftsConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLightShaftsPass, 1, ezRTTIDefaultAllocator<ezLightShaftsPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("DepthInput", m_PinDepthInput),
    EZ_ACCESSOR_PROPERTY("DownsampleFactor", GetDownsampleFactor, SetDownsampleFactor)->AddAttributes(new ezDefaultValueAttribute(3), new ezClampValueAttribute(1, 8)),
    EZ_ACCESSOR_PROPERTY("NumBlurPasses", GetNumBlurPasses, SetNumBlurPasses)->AddAttributes(new ezDefaultValueAttribute(3), new ezClampValueAttribute(1, 5)),
    EZ_ACCESSOR_PROPERTY("NumSamples", GetNumSamples, SetNumSamples)->AddAttributes(new ezDefaultValueAttribute(12), new ezClampValueAttribute(8, 16)),
    EZ_ACCESSOR_PROPERTY("MaxBlurDistance", GetMaxBlurDistance, SetMaxBlurDistance)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f))
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezLightShaftsPass::ezLightShaftsPass()
  : ezRenderPipelinePass("LightShaftsPass", true)
{
  m_hMaskShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/LightShaftsMask.ezShader");
  m_hRadialBlurShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/LightShaftsRadialBlur.ezShader");
  m_hApplyShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/LightShaftsApply.ezShader");

  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezLightShaftsConstants>();

  const ezGALDeviceCapabilities& caps = ezGALDevice::GetDefaultDevice()->GetCapabilities();
  const bool bSupportsRG11B10Float = caps.m_FormatSupport[ezGALResourceFormat::RG11B10Float].AreAllSet(ezGALResourceFormatSupport::RenderTarget | ezGALResourceFormatSupport::Texture);
  m_TextureFormat = bSupportsRG11B10Float ? ezGALResourceFormat::RG11B10Float : ezGALResourceFormat::RGBAHalf;
}

ezLightShaftsPass::~ezLightShaftsPass()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

bool ezLightShaftsPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  auto pDepthInput = inputs[m_PinDepthInput.m_uiInputIndex];
  if (!pDepthInput)
  {
    ezLog::Error("No depth input connected to '{0}'!", GetName());
    return false;
  }

  if (pDepthInput->m_SampleCount != ezGALMSAASampleCount::None)
  {
    ezLog::Error("'{0}' depth input must be resolved (non-MSAA). Connect MsaaResolvePass output.", GetName());
    return false;
  }

  auto pColorInput = inputs[m_PinColor.m_uiInputIndex];
  if (!pColorInput)
  {
    ezLog::Error("No color input connected to '{0}'!", GetName());
    return false;
  }

  if (pColorInput->m_SampleCount != ezGALMSAASampleCount::None)
  {
    ezLog::Error("'{0}' color input must be resolved (non-MSAA). Place after AntialiasingPass.", GetName());
    return false;
  }

  outputs[m_PinColor.m_uiOutputIndex] = *pColorInput;

  return true;
}

void ezLightShaftsPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pColor = outputs[m_PinColor.m_uiOutputIndex];
  if (pColor == nullptr)
    return;

  auto pDepthInput = inputs[m_PinDepthInput.m_uiInputIndex];
  if (pDepthInput == nullptr)
    return;

  auto pClusteredData = GetPipeline()->GetRenderData().GetFrameData<ezClusteredDataCPU>();
  if (pClusteredData == nullptr || pClusteredData->m_vLightShaftsDirection.IsZero())
    return;

  const ezVec4 vLightOriginUVs = CalculateOriginUVs(pClusteredData->m_vLightShaftsDirection, renderViewContext);
  if (vLightOriginUVs.w < 0.0f)
    return;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezUInt32 uiDownsampledWidth = ezMath::Max((pColor->m_Desc.m_uiWidth + m_uiDownsampleFactor - 1) / m_uiDownsampleFactor, 1u);
  ezUInt32 uiDownsampledHeight = ezMath::Max((pColor->m_Desc.m_uiHeight + m_uiDownsampleFactor - 1) / m_uiDownsampleFactor, 1u);
  ezUInt32 uiSliceCount = pColor->m_Desc.m_uiArraySize;
  ezRectFloat downsampledRect((float)uiDownsampledWidth, (float)uiDownsampledHeight);

  ezGALTextureHandle hTempTextures[2];
  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(hTempTextures); ++i)
  {
    hTempTextures[i] = ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiDownsampledWidth, uiDownsampledHeight, m_TextureFormat, ezGALMSAASampleCount::None, uiSliceCount);
  }

  EZ_SCOPE_EXIT(
    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(hTempTextures); ++i) {
      if (!hTempTextures[i].IsInvalidated())
        ezGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hTempTextures[i]);
    });

  UpdateConstantBuffer(*pClusteredData, vLightOriginUVs.GetAsVec2(), 0.0f);

  ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
  bindGroup.BindBuffer("ezLightShaftsConstants", m_hConstantBuffer);

  renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

  ezUInt32 uiCurrentInputTempTexture = 0;

  // Pass 1: Generate mask at half resolution
  {
    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, pDevice->GetDefaultRenderTargetView(hTempTextures[0]));
    renderViewContext.m_pRenderContext->BeginRendering(renderingSetup, downsampledRect, "LightShafts Mask", renderViewContext.m_pCamera->IsStereoscopic());

    bindGroup.BindTexture("SceneColor", pColor->m_TextureHandle);
    bindGroup.BindTexture("SceneDepth", pDepthInput->m_TextureHandle);

    renderViewContext.m_pRenderContext->BindShader(m_hMaskShader);
    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

    renderViewContext.m_pRenderContext->EndRendering();
  }

  // Pass 2: Radial blur
  {
    const float fFirstPassBlurDistance = m_fMaxBlurDistance / static_cast<float>(1 << (m_uiNumBlurPasses - 1));

    for (ezUInt32 uiPassIndex = 0; uiPassIndex < m_uiNumBlurPasses; ++uiPassIndex)
    {
      const float fBlurDistance = ezMath::Min((1 << uiPassIndex) * fFirstPassBlurDistance, 1.0f);
      const float fBlurStep = fBlurDistance / static_cast<float>(m_uiNumSamples);
      UpdateConstantBuffer(*pClusteredData, vLightOriginUVs.GetAsVec2(), fBlurStep);

      const ezUInt32 uiCurrentOutputTempTexture = (uiCurrentInputTempTexture + 1) % EZ_ARRAY_SIZE(hTempTextures);

      ezGALRenderingSetup renderingSetup;
      renderingSetup.SetColorTarget(0, pDevice->GetDefaultRenderTargetView(hTempTextures[uiCurrentOutputTempTexture]));
      renderViewContext.m_pRenderContext->BeginRendering(renderingSetup, downsampledRect, "LightShafts RadialBlur", renderViewContext.m_pCamera->IsStereoscopic());

      bindGroup.BindTexture("LightShaftsTexture", hTempTextures[uiCurrentInputTempTexture]);

      renderViewContext.m_pRenderContext->BindShader(m_hRadialBlurShader);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

      renderViewContext.m_pRenderContext->EndRendering();

      uiCurrentInputTempTexture = uiCurrentOutputTempTexture;
    }
  }

  // Pass 3: Apply to scene color at full resolution
  {
    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, pDevice->GetDefaultRenderTargetView(pColor->m_TextureHandle));
    renderViewContext.m_pRenderContext->BeginRendering(renderingSetup, renderViewContext.m_pViewData->m_ViewPortRect, "LightShafts Apply", renderViewContext.m_pCamera->IsStereoscopic());

    bindGroup.BindTexture("LightShaftsTexture", hTempTextures[uiCurrentInputTempTexture]);

    renderViewContext.m_pRenderContext->BindShader(m_hApplyShader);
    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

    renderViewContext.m_pRenderContext->EndRendering();
  }
}

ezResult ezLightShaftsPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_uiDownsampleFactor;
  inout_stream << m_uiNumBlurPasses;
  inout_stream << m_uiNumSamples;
  inout_stream << m_fMaxBlurDistance;
  return EZ_SUCCESS;
}

ezResult ezLightShaftsPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  inout_stream >> m_uiDownsampleFactor;
  inout_stream >> m_uiNumBlurPasses;
  inout_stream >> m_uiNumSamples;
  inout_stream >> m_fMaxBlurDistance;
  return EZ_SUCCESS;
}

void ezLightShaftsPass::SetDownsampleFactor(ezUInt32 uiFactor)
{
  m_uiDownsampleFactor = ezMath::Clamp(uiFactor, 1u, 8u);
}

void ezLightShaftsPass::SetNumBlurPasses(ezUInt32 uiPasses)
{
  m_uiNumBlurPasses = ezMath::Clamp(uiPasses, 1u, 5u);
}

void ezLightShaftsPass::SetNumSamples(ezUInt32 uiSamples)
{
  m_uiNumSamples = ezMath::Clamp(uiSamples, 8u, 16u);
}

void ezLightShaftsPass::SetMaxBlurDistance(float fDistance)
{
  m_fMaxBlurDistance = ezMath::Saturate(fDistance);
}

ezVec4 ezLightShaftsPass::CalculateOriginUVs(const ezVec3& vLightDirection, const ezRenderViewContext& renderViewContext) const
{
  const ezCamera* pCamera = renderViewContext.m_pCamera;
  const ezVec3 vLightPos = vLightDirection * (pCamera->GetFarPlane() * 0.999f) + pCamera->GetCenterPosition();

  const ezVec4 vLightPosClipSpace = renderViewContext.m_pViewData->m_ViewProjectionMatrix[0] * vLightPos.GetAsPositionVec4();
  const float fInvW = 1.0f / vLightPosClipSpace.w;

  const ezVec2 vProjected = vLightPosClipSpace.GetAsVec2() * fInvW;
  const ezVec2 vOriginUVs = vProjected.CompMul(ezVec2(0.5f, -0.5f)) + ezVec2(0.5f);

  return ezVec4(vOriginUVs.x, vOriginUVs.y, 0.0f, vLightPosClipSpace.w);
}

void ezLightShaftsPass::UpdateConstantBuffer(const ezClusteredDataCPU& clusteredData, const ezVec2& vLightOriginUVs, float fBlurStep)
{
  ezLightShaftsConstants* pConstants = ezRenderContext::GetConstantBufferData<ezLightShaftsConstants>(m_hConstantBuffer);

  pConstants->LightShaftsIntensity = clusteredData.m_fLightShaftsIntensity;
  pConstants->LightShaftsMaxBrightness = clusteredData.m_fLightShaftsMaxBrightness;
  pConstants->LightShaftsBrightnessThreshold = clusteredData.m_fLightShaftsBrightnessThreshold;
  pConstants->LightShaftsDiskMaskRadius = clusteredData.m_fLightShaftsDiskMaskRadius;

  pConstants->LightShaftsOriginUVs = vLightOriginUVs;
  pConstants->LightShaftsNumBlurSamples = m_uiNumSamples;
  pConstants->LightShaftsBlurStep = fBlurStep;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_LightShaftsPass);
