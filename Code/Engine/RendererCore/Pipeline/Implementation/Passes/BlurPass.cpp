#include <RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/BlurPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BlurConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBlurPass, 1, ezRTTIDefaultAllocator<ezBlurPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_PinInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezDefaultValueAttribute(15)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezBlurPass::ezBlurPass()
  : ezRenderPipelinePass("BlurPass")
  , m_iRadius(15)
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

bool ezBlurPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
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

void ezBlurPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  if (outputs[m_PinOutput.m_uiOutputIndex])
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    
    // Setup render target
    ezGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle));
    renderingSetup.m_uiRenderTargetClearMask = ezInvalidIndex;
    renderingSetup.m_ClearColor = ezColor(1.0f, 0.0f, 0.0f);

    // Bind render target and viewport
    auto pCommandEncoder = ezRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName());

    // Setup input view and sampler
    ezGALResourceViewCreationDescription rvcd;
    rvcd.m_hTexture = inputs[m_PinInput.m_uiInputIndex]->m_TextureHandle;
    ezGALResourceViewHandle hResourceView = ezGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);

    // Bind shader and inputs
    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("Input", hResourceView);
    renderViewContext.m_pRenderContext->BindConstantBuffer("ezBlurConstants", m_hBlurCB);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
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



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_BlurPass);
