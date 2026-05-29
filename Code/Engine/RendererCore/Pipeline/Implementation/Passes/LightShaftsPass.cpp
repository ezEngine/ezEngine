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

ezStatus ezLightShaftsPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezRenderGraphTextureHandle hColorInput = inputs[m_PinColor.m_uiInputIndex].m_TextureHandle;
  if (hColorInput.IsInvalidated())
    return ezStatus(ezFmt("Color: Not connected"));

  ezRenderGraphTextureHandle hDepthInput = inputs[m_PinDepthInput.m_uiInputIndex].m_TextureHandle;
  if (hDepthInput.IsInvalidated())
    return ezStatus(ezFmt("DepthInput: Not connected"));

  const ezGALTextureCreationDescription depthDesc = ref_graph.GetTextureDesc(hDepthInput);
  if (depthDesc.m_SampleCount != ezGALMSAASampleCount::None)
    return ezStatus(ezFmt("DepthInput: Must be resolved (non-MSAA)"));

  const ezGALTextureCreationDescription colorDesc = ref_graph.GetTextureDesc(hColorInput);
  if (colorDesc.m_SampleCount != ezGALMSAASampleCount::None)
    return ezStatus(ezFmt("Color: Must be resolved (non-MSAA)"));

  // Pass-through color
  outputs[m_PinColor.m_uiOutputIndex].m_TextureHandle = hColorInput;

  // Compute light origin UVs from camera and light direction
  auto pClusteredData = GetPipeline()->GetRenderData().GetFrameData<ezClusteredDataCPU>();
  if (pClusteredData == nullptr || pClusteredData->m_vLightShaftsDirection.IsZero())
    return EZ_SUCCESS;

  // Note these computations here make any graph using this pass uncachable as the logic depends on the current camera.
  const ezVec4 vLightOriginUVs = CalculateOriginUVs(pClusteredData->m_vLightShaftsDirection, viewData, camera);
  if (vLightOriginUVs.w < 0.0f)
    return EZ_SUCCESS;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Setup downsampled dimensions
  ezUInt32 uiDownsampledWidth = ezMath::Max((colorDesc.m_uiWidth + m_uiDownsampleFactor - 1) / m_uiDownsampleFactor, 1u);
  ezUInt32 uiDownsampledHeight = ezMath::Max((colorDesc.m_uiHeight + m_uiDownsampleFactor - 1) / m_uiDownsampleFactor, 1u);
  ezUInt32 uiSliceCount = colorDesc.m_uiArraySize;
  ezRectFloat downsampledRect((float)uiDownsampledWidth, (float)uiDownsampledHeight);

  // Create temp textures
  ezGALTextureCreationDescription tempDesc;
  tempDesc.SetAsRenderTarget(uiDownsampledWidth, uiDownsampledHeight, uiSliceCount, m_TextureFormat, ezGALMSAASampleCount::None);

  ezRenderGraphTextureHandle hTempTextures[2];
  hTempTextures[0] = ref_graph.CreateTexture(tempDesc);
  hTempTextures[1] = ref_graph.CreateTexture(tempDesc);

  ezUInt32 uiCurrentInputTempTexture = 0;

  // Pass 1: Generate mask at downsampled resolution
  {
    auto pass = ref_graph.AddGraphicsPass("LightShafts Mask");
    pass.AddColorTarget(hTempTextures[0]);
    pass.ReadTexture(hColorInput, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
    pass.ReadTexture(hDepthInput, {}, ezGALResourceState::DepthStencilRead, ezGALShaderStageFlags::PixelShader);
    pass.SetStereoscopic(camera.IsStereoscopic());
    pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
      {
      const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();

      UpdateConstantBuffer(*pClusteredData, vLightOriginUVs.GetAsVec2(), 0.0f);

      ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
      bindGroup.BindBuffer("ezLightShaftsConstants", m_hConstantBuffer);
      bindGroup.BindTexture("SceneColor", ctx.ResolveTexture(hColorInput));
      bindGroup.BindTexture("SceneDepth", ctx.ResolveTexture(hDepthInput));

      renderViewContext.m_pRenderContext->BindShader(m_hMaskShader);
      renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }

  // Pass 2: Radial blur passes
  {
    const float fFirstPassBlurDistance = m_fMaxBlurDistance / static_cast<float>(1 << (m_uiNumBlurPasses - 1));

    for (ezUInt32 uiPassIndex = 0; uiPassIndex < m_uiNumBlurPasses; ++uiPassIndex)
    {
      const float fBlurDistance = ezMath::Min((1 << uiPassIndex) * fFirstPassBlurDistance, 1.0f);
      const float fBlurStep = fBlurDistance / static_cast<float>(m_uiNumSamples);

      const ezUInt32 uiCurrentOutputTempTexture = (uiCurrentInputTempTexture + 1) % 2;

      ezRenderGraphTextureHandle hInput = hTempTextures[uiCurrentInputTempTexture];
      ezRenderGraphTextureHandle hOutput = hTempTextures[uiCurrentOutputTempTexture];

      auto pass = ref_graph.AddGraphicsPass("LightShafts RadialBlur");
      pass.AddColorTarget(hOutput);
      pass.ReadTexture(hInput, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
      pass.SetStereoscopic(camera.IsStereoscopic());
      pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
        {
        const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
        UpdateConstantBuffer(*pClusteredData, vLightOriginUVs.GetAsVec2(), fBlurStep);
        ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
        bindGroup.BindBuffer("ezLightShaftsConstants", m_hConstantBuffer);
        bindGroup.BindTexture("LightShaftsTexture", ctx.ResolveTexture(hInput));

        renderViewContext.m_pRenderContext->BindShader(m_hRadialBlurShader);
        renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });

      uiCurrentInputTempTexture = uiCurrentOutputTempTexture;
    }
  }

  // Pass 3: Apply to scene color at full resolution
  {
    ezRenderGraphTextureHandle hBlurResult = hTempTextures[uiCurrentInputTempTexture];

    auto pass = ref_graph.AddGraphicsPass("LightShafts Apply");
    pass.AddColorTarget(hColorInput);
    pass.ReadTexture(hBlurResult, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
    pass.SetStereoscopic(camera.IsStereoscopic());
    pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
      {
      const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();

      ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
      bindGroup.BindBuffer("ezLightShaftsConstants", m_hConstantBuffer);
      bindGroup.BindTexture("LightShaftsTexture", ctx.ResolveTexture(hBlurResult));

      renderViewContext.m_pRenderContext->BindShader(m_hApplyShader);
      renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }

  return EZ_SUCCESS;
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

ezVec4 ezLightShaftsPass::CalculateOriginUVs(const ezVec3& vLightDirection, const ezViewData& viewData, const ezCamera& camera) const
{
  const ezVec3 vLightPos = vLightDirection * (camera.GetFarPlane() * 0.999f) + camera.GetCenterPosition();

  const ezVec4 vLightPosClipSpace = viewData.m_ViewProjectionMatrix[0] * vLightPos.GetAsPositionVec4();
  const float fInvW = 1.0f / vLightPosClipSpace.w;

  const ezVec2 vProjected = vLightPosClipSpace.GetAsVec2() * fInvW;
  const ezVec2 vOriginUVs = vProjected.CompMul(ezVec2(0.5f, -0.5f)) + ezVec2(0.5f);

  return ezVec4(vOriginUVs.x, vOriginUVs.y, 0.0f, vLightPosClipSpace.w);
}

void ezLightShaftsPass::UpdateConstantBuffer(const ezClusteredDataCPU& clusteredData, const ezVec2& vLightOriginUVs, float fBlurStep)
{
  ezLightShaftsConstants* pConstants = ezRenderContext::GetConstantBufferData<ezLightShaftsConstants>(m_hConstantBuffer);

  pConstants->LightShaftsTintColor = clusteredData.m_LightShaftsTintColor;

  pConstants->LightShaftsIntensity = clusteredData.m_fLightShaftsIntensity;
  pConstants->LightShaftsMaxBrightness = clusteredData.m_fLightShaftsMaxBrightness;
  pConstants->LightShaftsBrightnessThreshold = clusteredData.m_fLightShaftsBrightnessThreshold;
  pConstants->LightShaftsDiskMaskRadius = clusteredData.m_fLightShaftsDiskMaskRadius;

  pConstants->LightShaftsOriginUVs = vLightOriginUVs;
  pConstants->LightShaftsNumBlurSamples = m_uiNumSamples;
  pConstants->LightShaftsBlurStep = fBlurStep;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_LightShaftsPass);
