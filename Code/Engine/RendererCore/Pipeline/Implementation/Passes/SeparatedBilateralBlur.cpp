#include <RendererCorePCH.h>

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
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSeparatedBilateralBlurPass::ezSeparatedBilateralBlurPass()
  : ezRenderPipelinePass("SeparatedBilateral")
  , m_uiRadius(7)
  , m_fGaussianSigma(3.5f)
  , m_fSharpness(120.0f)
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
  m_hBilateralBlurCB.Invalidate();
}

bool ezSeparatedBilateralBlurPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  EZ_ASSERT_DEBUG(inputs.GetCount() == 2, "Unexpected number of inputs for ezSeparatedBilateralBlurPass.");

  // Color
  if (!inputs[m_PinBlurSourceInput.m_uiInputIndex])
  {
    ezLog::Error("No blur target connected to bilateral blur pass!");
    return false;
  }
  if (!inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_bAllowShaderResourceView)
  {
    ezLog::Error("All bilateral blur pass inputs must allow shader resoure view.");
    return false;
  }

  // Depth
  if (!inputs[m_PinDepthInput.m_uiInputIndex])
  {
    ezLog::Error("No depth connected to bilateral blur pass!");
    return false;
  }
  if (!inputs[m_PinDepthInput.m_uiInputIndex]->m_bAllowShaderResourceView)
  {
    ezLog::Error("All bilateral blur pass inputs must allow shader resoure view.");
    return false;
  }
  if (inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_uiWidth != inputs[m_PinDepthInput.m_uiInputIndex]->m_uiWidth || inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_uiHeight != inputs[m_PinDepthInput.m_uiInputIndex]->m_uiHeight)
  {
    ezLog::Error("Blur target and depth buffer for bilateral blur pass need to have the same dimensions.");
    return false;
  }


  // Output format maches input format.
  outputs[m_PinOutput.m_uiOutputIndex] = *inputs[m_PinBlurSourceInput.m_uiInputIndex];

  return true;
}

void ezSeparatedBilateralBlurPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  if (outputs[m_PinOutput.m_uiOutputIndex])
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    ezGALPass* pGALPass = pDevice->BeginPass(GetName());
    EZ_SCOPE_EXIT(pDevice->EndPass(pGALPass));

    // Setup input view and sampler
    ezGALResourceViewCreationDescription rvcd;
    rvcd.m_hTexture = inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_TextureHandle;
    ezGALResourceViewHandle hBlurSourceInputView = ezGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);
    rvcd.m_hTexture = inputs[m_PinDepthInput.m_uiInputIndex]->m_TextureHandle;
    ezGALResourceViewHandle hDepthInputView = ezGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);

    // Get temp texture for horizontal target / vertical source.
    ezGALTextureCreationDescription tempTextureDesc = outputs[m_PinBlurSourceInput.m_uiInputIndex]->m_Desc;
    tempTextureDesc.m_bAllowShaderResourceView = true;
    tempTextureDesc.m_bCreateRenderTarget = true;
    ezGALTextureHandle tempTexture = ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(tempTextureDesc);
    rvcd.m_hTexture = tempTexture;
    ezGALResourceViewHandle hTempTextureRView = ezGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);

    ezGALRenderingSetup renderingSetup;

    // Bind shader and inputs
    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("DepthBuffer", hDepthInputView);
    renderViewContext.m_pRenderContext->BindConstantBuffer("ezBilateralBlurConstants", m_hBilateralBlurCB);

    // Horizontal
    {
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(tempTexture));
      auto pCommandEncoder = ezRenderContext::BeginRenderingScope(pGALPass, renderViewContext, renderingSetup);

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_DIRECTION", "BLUR_DIRECTION_HORIZONTAL");
      renderViewContext.m_pRenderContext->BindTexture2D("BlurSource", hBlurSourceInputView);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
    }

    // Vertical
    {
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle));
      auto pCommandEncoder = ezRenderContext::BeginRenderingScope(pGALPass, renderViewContext, renderingSetup);

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_DIRECTION", "BLUR_DIRECTION_VERTICAL");
      renderViewContext.m_pRenderContext->BindTexture2D("BlurSource", hTempTextureRView);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
    }

    // Give back temp texture.
    ezGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(tempTexture);
  }
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

void ezSeparatedBilateralBlurPass::SetGaussianSigma(const float sigma)
{
  m_fGaussianSigma = sigma;

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

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class ezSeparatedBilateralBlurPassPatch_1_2 : public ezGraphPatch
{
public:
  ezSeparatedBilateralBlurPassPatch_1_2()
    : ezGraphPatch("ezSeparatedBilateralBlurPass", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Blur Radius", "BlurRadius");
    pNode->RenameProperty("Gaussian Standard Deviation", "GaussianSigma");
    pNode->RenameProperty("Bilateral Sharpness", "Sharpness");
  }
};

ezSeparatedBilateralBlurPassPatch_1_2 g_ezSeparatedBilateralBlurPassPatch_1_2;



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SeparatedBilateralBlur);
