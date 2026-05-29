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
}

ezStatus ezBloomPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  // Validate input
  ezRenderGraphTextureHandle hColorInput = inputs[m_PinInput.m_uiInputIndex].m_TextureHandle;
  if (hColorInput.IsInvalidated())
    return ezStatus(ezFmt("Input: Not connected"));

  const ezGALTextureCreationDescription inputDesc = ref_graph.GetTextureDesc(hColorInput);

  // Create output (half-res)
  ezGALTextureCreationDescription outputDesc = inputDesc;
  outputDesc.m_uiWidth = outputDesc.m_uiWidth / 2;
  outputDesc.m_uiHeight = outputDesc.m_uiHeight / 2;
  outputDesc.m_Format = m_TextureFormat;

  ezRenderGraphTextureHandle hColorOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hColorOutput;

  // Add passes
  ezUInt32 uiWidth = inputDesc.m_uiWidth;
  ezUInt32 uiHeight = inputDesc.m_uiHeight;
  bool bFastDownscale = ezMath::IsEven(uiWidth) && ezMath::IsEven(uiHeight);

  const float fMaxRes = (float)ezMath::Max(uiWidth, uiHeight);
  const float fRadius = ezMath::Clamp(m_fRadius, 0.01f, 1.0f);
  const float fDownscaledSize = 4.0f / fRadius;
  const float fNumBlurPasses = ezMath::Log2(fMaxRes / fDownscaledSize);
  const ezUInt32 uiNumBlurPasses = (ezUInt32)ezMath::Ceil(fNumBlurPasses);

  // Create temp textures
  ezTempHybridArray<ezVec2, 8> targetSizes;
  ezTempHybridArray<ezRenderGraphTextureHandle, 8> tempDownscaleTextures;
  ezTempHybridArray<ezRenderGraphTextureHandle, 8> tempUpscaleTextures;

  for (ezUInt32 i = 0; i < uiNumBlurPasses; ++i)
  {
    uiWidth = ezMath::Max(uiWidth / 2, 1u);
    uiHeight = ezMath::Max(uiHeight / 2, 1u);
    targetSizes.PushBack(ezVec2((float)uiWidth, (float)uiHeight));
    auto uiSliceCount = outputDesc.m_uiArraySize;

    ezGALTextureCreationDescription descTemp;
    descTemp.SetAsRenderTarget(uiWidth, uiHeight, uiSliceCount, m_TextureFormat, ezGALMSAASampleCount::None);
    tempDownscaleTextures.PushBack(ref_graph.CreateTexture(descTemp));

    // biggest upscale target is the output and lowest is not needed
    if (i > 0 && i < uiNumBlurPasses - 1)
    {
      tempUpscaleTextures.PushBack(ref_graph.CreateTexture(descTemp));
    }
    else
    {
      tempUpscaleTextures.PushBack(ezRenderGraphTextureHandle());
    }
  }

  // Downscale passes
  {
    ezTempHashedString sInitialDownscale = "BLOOM_PASS_MODE_INITIAL_DOWNSCALE";
    ezTempHashedString sInitialDownscaleFast = "BLOOM_PASS_MODE_INITIAL_DOWNSCALE_FAST";
    ezTempHashedString sDownscale = "BLOOM_PASS_MODE_DOWNSCALE";
    ezTempHashedString sDownscaleFast = "BLOOM_PASS_MODE_DOWNSCALE_FAST";

    for (ezUInt32 i = 0; i < uiNumBlurPasses; ++i)
    {
      ezRenderGraphTextureHandle hInput;
      ezTempHashedString sPassMode;
      if (i == 0)
      {
        hInput = hColorInput;
        sPassMode = bFastDownscale ? sInitialDownscaleFast : sInitialDownscale;
      }
      else
      {
        hInput = tempDownscaleTextures[i - 1];
        sPassMode = bFastDownscale ? sDownscaleFast : sDownscale;
      }

      ezRenderGraphTextureHandle hOutput = tempDownscaleTextures[i];
      ezVec2 targetSize = targetSizes[i];
      ezColor tintColor = (i == uiNumBlurPasses - 1) ? ezColor(m_OuterTintColor) : ezColor::White;

      auto pass = ref_graph.AddGraphicsPass("BloomDownscale");
      pass.AddColorTarget(hOutput);
      pass.ReadTexture(hInput, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
      pass.SetStereoscopic(camera.IsStereoscopic());
      pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
        {
        const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();

        UpdateConstantBuffer(ezVec2(1.0f).CompDiv(targetSize), tintColor);

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", sPassMode);

        ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
        bindGroup.BindBuffer("ezBloomConstants", m_hConstantBuffer);
        renderViewContext.m_pRenderContext->BindShader(m_hShader);

        bindGroup.BindTexture("ColorTexture", ctx.ResolveTexture(hInput));
        renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });

      bFastDownscale = ezMath::IsEven((ezInt32)targetSize.x) && ezMath::IsEven((ezInt32)targetSize.y);
    }
  }

  // Upscale passes
  {
    const float fBlurRadius = 2.0f * fNumBlurPasses / uiNumBlurPasses;
    const float fMidPass = (uiNumBlurPasses - 1.0f) / 2.0f;

    for (ezUInt32 i = uiNumBlurPasses - 1; i-- > 0;)
    {
      ezRenderGraphTextureHandle hNextInput = tempDownscaleTextures[i];
      ezRenderGraphTextureHandle hInput;
      if (i == uiNumBlurPasses - 2)
      {
        hInput = tempDownscaleTextures[i + 1];
      }
      else
      {
        hInput = tempUpscaleTextures[i + 1];
      }

      ezRenderGraphTextureHandle hOutput;
      if (i == 0)
      {
        hOutput = hColorOutput;
      }
      else
      {
        hOutput = tempUpscaleTextures[i];
      }

      ezVec2 targetSize = targetSizes[i];

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

      auto pass = ref_graph.AddGraphicsPass("BloomUpscale");
      pass.AddColorTarget(hOutput);
      pass.ReadTexture(hInput, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
      pass.ReadTexture(hNextInput, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
      pass.SetStereoscopic(camera.IsStereoscopic());
      pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
        {
        const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();

        UpdateConstantBuffer(ezVec2(fBlurRadius).CompDiv(targetSize), tintColor);

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", "BLOOM_PASS_MODE_UPSCALE");

        ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
        bindGroup.BindBuffer("ezBloomConstants", m_hConstantBuffer);
        renderViewContext.m_pRenderContext->BindShader(m_hShader);

        bindGroup.BindTexture("NextColorTexture", ctx.ResolveTexture(hNextInput));
        bindGroup.BindTexture("ColorTexture", ctx.ResolveTexture(hInput));
        renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });
    }
  }

  return EZ_SUCCESS;
}

ezStatus ezBloomPass::AddRenderPassesInactive(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezRenderGraphTextureHandle hColorInput = inputs[m_PinInput.m_uiInputIndex].m_TextureHandle;
  if (hColorInput.IsInvalidated())
    return ezStatus(ezFmt("Input: Not connected"));

  ezGALTextureCreationDescription outputDesc = ref_graph.GetTextureDesc(hColorInput);
  outputDesc.m_uiWidth = outputDesc.m_uiWidth / 2;
  outputDesc.m_uiHeight = outputDesc.m_uiHeight / 2;
  outputDesc.m_Format = m_TextureFormat;

  ezRenderGraphTextureHandle hColorOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hColorOutput;

  auto pass = ref_graph.AddGraphicsPass("InactiveBloom");
  pass.AddColorTarget(hColorOutput, {}, ezGALRenderTargetLoadOp::Clear);
  pass.SetClearColor(0, ezColor::Black);
  return EZ_SUCCESS;
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
