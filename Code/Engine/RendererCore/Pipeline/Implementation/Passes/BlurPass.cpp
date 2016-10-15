#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/Passes/BlurPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <CoreUtils/Geometry/GeomUtils.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BlurConstants.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBlurPass, 1, ezRTTIDefaultAllocator<ezBlurPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_PinInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezDefaultValueAttribute(15)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezBlurPass::ezBlurPass() : ezRenderPipelinePass("BlurPass"), m_iRadius(15)
{
  {
    // Load shader.
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Blur.ezShader");
    EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load blur shader!");
  }

  {
    m_hBlurCB = ezRenderContext::CreateConstantBufferStorage<ezBlurConstants>();
  }
}

ezBlurPass::~ezBlurPass()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hBlurCB);
  m_hBlurCB.Invalidate();
}

bool ezBlurPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription*const> inputs,
  ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinInput.m_uiInputIndex])
  {
    if (!inputs[m_PinInput.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      ezLog::Error("Blur pass input must allow shader resoure view.");
      return false;
    }

    outputs[m_PinOutput.m_uiOutputIndex] = *inputs[m_PinInput.m_uiInputIndex];
  }
  else
  {
    ezLog::Error("No input connected to blur pass!");
    return false;
  }

  return true;
}

void ezBlurPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  if (outputs[m_PinOutput.m_uiOutputIndex])
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

    // Setup render target
    ezGALRenderTagetSetup renderTargetSetup;
    renderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle));

    // Bind render target and viewport
    pGALContext->SetRenderTargetSetup(renderTargetSetup);
    pGALContext->SetViewport(renderViewContext.m_pViewData->m_ViewPortRect);

    pGALContext->Clear(ezColor(1.0f, 0.0f, 0.0f));
    // Setup input view and sampler
    ezGALResourceViewCreationDescription rvcd;
    rvcd.m_hTexture = inputs[m_PinInput.m_uiInputIndex]->m_TextureHandle;
    ezGALResourceViewHandle hResourceView = ezGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);

    // Bind shader and inputs
    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->BindTexture(ezGALShaderStage::PixelShader, "Input", hResourceView);
    renderViewContext.m_pRenderContext->BindConstantBuffer("ezBlurConstants", m_hBlurCB);

    renderViewContext.m_pRenderContext->DrawMeshBuffer();
  }
}

void ezBlurPass::SetRadius(ezInt32 iRadius)
{
  m_iRadius = iRadius;

  ezBlurConstants* cb = ezRenderContext::GetConstantBufferData<ezBlurConstants>(m_hBlurCB);
  cb->BlurRadius = m_iRadius;
}

ezInt32 ezBlurPass::GetRadius() const
{
  return m_iRadius;
}
