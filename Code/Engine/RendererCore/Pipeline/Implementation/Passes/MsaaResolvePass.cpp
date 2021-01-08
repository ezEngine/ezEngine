#include <RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/MsaaResolvePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsaaResolvePass, 1, ezRTTIDefaultAllocator<ezMsaaResolvePass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_PinInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezMsaaResolvePass::ezMsaaResolvePass()
  : ezRenderPipelinePass("MsaaResolvePass")
  , m_bIsDepth(false)
  , m_MsaaSampleCount(ezGALMSAASampleCount::None)
{
  {
    // Load shader.
    m_hDepthResolveShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/MsaaDepthResolve.ezShader");
    EZ_ASSERT_DEV(m_hDepthResolveShader.IsValid(), "Could not load depth resolve shader!");
  }
}

ezMsaaResolvePass::~ezMsaaResolvePass() {}

bool ezMsaaResolvePass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  if (pInput != nullptr)
  {
    if (pInput->m_SampleCount == ezGALMSAASampleCount::None)
    {
      ezLog::Error("Input is not a valid msaa target");
      return false;
    }

    m_bIsDepth = ezGALResourceFormat::IsDepthFormat(pInput->m_Format);
    m_MsaaSampleCount = pInput->m_SampleCount;

    ezGALTextureCreationDescription desc = *pInput;
    desc.m_SampleCount = ezGALMSAASampleCount::None;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    ezLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void ezMsaaResolvePass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  if (m_bIsDepth)
  {
    // Setup render target
    ezGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));

    // Bind render target and viewport
    auto pCommandEncoder = ezRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName());

    auto& globals = renderViewContext.m_pRenderContext->WriteGlobalConstants();
    globals.NumMsaaSamples = m_MsaaSampleCount;

    renderViewContext.m_pRenderContext->BindShader(m_hDepthResolveShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }
  else
  {
    auto pCommandEncoder = ezRenderContext::BeginPassAndRenderingScope(renderViewContext, ezGALRenderingSetup(), GetName());

    ezGALTextureSubresource subresource;
    subresource.m_uiMipLevel = 0;
    subresource.m_uiArraySlice = 0;

    pCommandEncoder->ResolveTexture(pOutput->m_TextureHandle, subresource, pInput->m_TextureHandle, subresource);
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_MsaaResolvePass);
