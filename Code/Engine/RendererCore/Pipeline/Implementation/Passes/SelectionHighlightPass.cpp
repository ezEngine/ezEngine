#include <RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/SelectionHighlightPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/SelectionHighlightConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSelectionHighlightPass, 1, ezRTTIDefaultAllocator<ezSelectionHighlightPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil)->AddAttributes(new ezColorAttribute(ezColor::LightCoral)),

    EZ_MEMBER_PROPERTY("HighlightColor", m_HighlightColor)->AddAttributes(new ezDefaultValueAttribute(ezColor(177.0 / 255.0, 135.0 / 255.0, 27.0 / 255.0))),
    EZ_MEMBER_PROPERTY("OverlayOpacity", m_fOverlayOpacity)->AddAttributes(new ezDefaultValueAttribute(0.1f))
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSelectionHighlightPass::ezSelectionHighlightPass(const char* szName)
  : ezRenderPipelinePass(szName)
{
  // Load shader.
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SelectionHighlight.ezShader");
  EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load selection highlight shader!");

  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezSelectionHighlightConstants>();

  m_HighlightColor = ezColor::Yellow;
  m_fOverlayOpacity = 0.1f;
}

ezSelectionHighlightPass::~ezSelectionHighlightPass()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool ezSelectionHighlightPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
    return true;
  }

  return false;
}

void ezSelectionHighlightPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pColorOutput = outputs[m_PinColor.m_uiOutputIndex];
  if (pColorOutput == nullptr)
  {
    return;
  }

  auto pDepthInput = inputs[m_PinDepthStencil.m_uiInputIndex];
  if (pDepthInput == nullptr)
  {
    return;
  }

  ezRenderDataBatchList renderDataBatchList = GetPipeline()->GetRenderDataBatchesWithCategory(ezDefaultRenderDataCategories::Selection);
  if (renderDataBatchList.GetBatchCount() == 0)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezGALTextureHandle hDepthTexture;

  // render all selection objects to depth target only
  {
    ezUInt32 uiWidth = pColorOutput->m_Desc.m_uiWidth;
    ezUInt32 uiHeight = pColorOutput->m_Desc.m_uiHeight;
    ezGALMSAASampleCount::Enum sampleCount = pColorOutput->m_Desc.m_SampleCount;
    ezUInt32 uiSliceCount = pColorOutput->m_Desc.m_uiArraySize;

    hDepthTexture = ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, ezGALResourceFormat::D24S8, sampleCount, uiSliceCount);

    ezGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(hDepthTexture));
    renderingSetup.m_bClearDepth = true;
    renderingSetup.m_bClearStencil = true;

    auto pCommandEncoder = ezRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName());

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_DEPTH_ONLY");

    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Selection);
  }

  // reconstruct selection overlay from depth target
  {
    auto constants = ezRenderContext::GetConstantBufferData<ezSelectionHighlightConstants>(m_hConstantBuffer);
    constants->HighlightColor = m_HighlightColor;
    constants->OverlayOpacity = m_fOverlayOpacity;

    ezGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pColorOutput->m_TextureHandle));

    auto pCommandEncoder = ezRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName());

    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindConstantBuffer("ezSelectionHighlightConstants", m_hConstantBuffer);
    renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("SelectionDepthTexture", pDevice->GetDefaultResourceView(hDepthTexture));
    renderViewContext.m_pRenderContext->BindTexture2D("SceneDepthTexture", pDevice->GetDefaultResourceView(pDepthInput->m_TextureHandle));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

    ezGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hDepthTexture);
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SelectionHighlightPass);
