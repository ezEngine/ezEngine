#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/SeparatedBilateralBlur.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BilateralBlurConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSeparatedBilateralBlurPass, 2, ezRTTIDefaultAllocator<ezSeparatedBilateralBlurPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("BlurSource", m_PinBlurSourceInput),
    EZ_MEMBER_PROPERTY("Depth", m_PinDepthInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_ACCESSOR_PROPERTY("BlurRadius", GetRadius, SetRadius)->AddAttributes(new ezDefaultValueAttribute(7)),
      // Should we really expose that? This gives the user control over the error compared to a perfect gaussian.
      // In theory we could also compute this for a given error from the blur radius. See http://dev.theomader.com/gaussian-kernel-calculator/ for visualization.
    EZ_ACCESSOR_PROPERTY("GaussianSigma", GetGaussianSigma, SetGaussianSigma)->AddAttributes(new ezDefaultValueAttribute(4.0f)),
    EZ_ACCESSOR_PROPERTY("Sharpness", GetSharpness, SetSharpness)->AddAttributes(new ezDefaultValueAttribute(120.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Utilities")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSeparatedBilateralBlurPass::ezSeparatedBilateralBlurPass()
  : ezRenderPipelinePass("SeparatedBilateral")

{
  {
    // Load shader.
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SeparatedBilateralBlur.ezShader");
    EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load blur shader!");
  }

  {
    m_hBilateralBlurCB = ezRenderContext::CreateConstantBufferStorage<ezBilateralBlurConstants>();
  }
}

ezSeparatedBilateralBlurPass::~ezSeparatedBilateralBlurPass()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hBilateralBlurCB);
}

ezStatus ezSeparatedBilateralBlurPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezRenderGraphTextureHandle hBlurSource = inputs[m_PinBlurSourceInput.m_uiInputIndex].m_TextureHandle;
  ezRenderGraphTextureHandle hDepth = inputs[m_PinDepthInput.m_uiInputIndex].m_TextureHandle;
  if (hBlurSource.IsInvalidated())
    return ezStatus(ezFmt("BlurSource: Not connected"));
  if (hDepth.IsInvalidated())
    return ezStatus(ezFmt("Depth: Not connected"));

  const ezGALTextureCreationDescription blurDesc = ref_graph.GetTextureDesc(hBlurSource);
  const ezGALTextureCreationDescription depthDesc = ref_graph.GetTextureDesc(hDepth);
  if (blurDesc.m_uiWidth != depthDesc.m_uiWidth || blurDesc.m_uiHeight != depthDesc.m_uiHeight)
    return ezStatus(ezFmt("Blur target and depth buffer need same dimensions"));

  // Output
  ezRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(blurDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  // Temp texture for horizontal pass result
  ezGALTextureCreationDescription tempDesc = blurDesc;
  tempDesc.m_TextureFlags.Add(ezGALTextureUsageFlags::ShaderResource | ezGALTextureUsageFlags::RenderTarget);
  ezRenderGraphTextureHandle hTemp = ref_graph.CreateTexture(tempDesc);

  // Horizontal pass
  {
    auto pass = ref_graph.AddGraphicsPass("BilateralBlurH");
    pass.AddColorTarget(hTemp);
    pass.ReadTexture(hBlurSource, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
    pass.ReadTexture(hDepth, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
    pass.SetStereoscopic(camera.IsStereoscopic());
    pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
      {
      const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
      renderViewContext.UpdateViewport();

      renderViewContext.m_pRenderContext->BindShader(m_hShader);
      renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_DIRECTION", "BLUR_DIRECTION_HORIZONTAL");

      ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
      bindGroup.BindTexture("DepthBuffer", ctx.ResolveTexture(hDepth));
      bindGroup.BindBuffer("ezBilateralBlurConstants", m_hBilateralBlurCB);
      bindGroup.BindTexture("BlurSource", ctx.ResolveTexture(hBlurSource));

      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }

  // Vertical pass
  {
    auto pass = ref_graph.AddGraphicsPass("BilateralBlurV");
    pass.AddColorTarget(hOutput);
    pass.ReadTexture(hTemp, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
    pass.ReadTexture(hDepth, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
    pass.SetStereoscopic(camera.IsStereoscopic());
    pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
      {
      const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
      renderViewContext.UpdateViewport();

      renderViewContext.m_pRenderContext->BindShader(m_hShader);
      renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_DIRECTION", "BLUR_DIRECTION_VERTICAL");

      ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
      bindGroup.BindTexture("DepthBuffer", ctx.ResolveTexture(hDepth));
      bindGroup.BindBuffer("ezBilateralBlurConstants", m_hBilateralBlurCB);
      bindGroup.BindTexture("BlurSource", ctx.ResolveTexture(hTemp));

      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }

  return EZ_SUCCESS;
}

ezResult ezSeparatedBilateralBlurPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_uiRadius;
  inout_stream << m_fGaussianSigma;
  inout_stream << m_fSharpness;
  return EZ_SUCCESS;
}

ezResult ezSeparatedBilateralBlurPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_uiRadius;
  inout_stream >> m_fGaussianSigma;
  inout_stream >> m_fSharpness;
  return EZ_SUCCESS;
}

void ezSeparatedBilateralBlurPass::SetRadius(ezUInt32 uiRadius)
{
  m_uiRadius = uiRadius;

  ezBilateralBlurConstants* cb = ezRenderContext::GetConstantBufferData<ezBilateralBlurConstants>(m_hBilateralBlurCB);
  cb->BlurRadius = m_uiRadius;
}

ezUInt32 ezSeparatedBilateralBlurPass::GetRadius() const
{
  return m_uiRadius;
}

void ezSeparatedBilateralBlurPass::SetGaussianSigma(const float fSigma)
{
  m_fGaussianSigma = fSigma;

  ezBilateralBlurConstants* cb = ezRenderContext::GetConstantBufferData<ezBilateralBlurConstants>(m_hBilateralBlurCB);
  cb->GaussianFalloff = 1.0f / (2.0f * m_fGaussianSigma * m_fGaussianSigma);
}

float ezSeparatedBilateralBlurPass::GetGaussianSigma() const
{
  return m_fGaussianSigma;
}

void ezSeparatedBilateralBlurPass::SetSharpness(const float fSharpness)
{
  m_fSharpness = fSharpness;

  ezBilateralBlurConstants* cb = ezRenderContext::GetConstantBufferData<ezBilateralBlurConstants>(m_hBilateralBlurCB);
  cb->Sharpness = m_fSharpness;
}

float ezSeparatedBilateralBlurPass::GetSharpness() const
{
  return m_fSharpness;
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezSeparatedBilateralBlurPassPatch_1_2 : public ezGraphPatch
{
public:
  ezSeparatedBilateralBlurPassPatch_1_2()
    : ezGraphPatch("ezSeparatedBilateralBlurPass", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Blur Radius", "BlurRadius");
    pNode->RenameProperty("Gaussian Standard Deviation", "GaussianSigma");
    pNode->RenameProperty("Bilateral Sharpness", "Sharpness");
  }
};

ezSeparatedBilateralBlurPassPatch_1_2 g_ezSeparatedBilateralBlurPassPatch_1_2;



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SeparatedBilateralBlur);
