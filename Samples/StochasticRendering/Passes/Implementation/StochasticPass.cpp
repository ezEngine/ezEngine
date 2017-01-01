#include <StochasticRendering/PCH.h>
#include <StochasticRendering/Passes/StochasticPass.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Resources/Resource.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererCore/RenderContext/RenderContext.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStochasticPass, 1, ezRTTIDefaultAllocator<ezStochasticPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("SampleCount", m_PinSampleCount),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil)
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezStochasticPass::ezStochasticPass(const char* name /*= "StoachsticPass"*/)
  : ezRenderPipelinePass(name)
{
  m_lastViewProjection.SetIdentity();
  m_randomGenerator.InitializeFromCurrentTime();
}

ezStochasticPass::~ezStochasticPass()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  if (!m_randomNumberBuffer.IsInvalidated())
  {
    pDevice->DestroyBuffer(m_randomNumberBuffer);
  }
}

bool ezStochasticPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription * const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  const ezGALRenderTagetSetup& setup = view.GetRenderTargetSetup();

  // Color
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
  }
  else
  {
    return false;
  }


  // Sample count
  if (inputs[m_PinSampleCount.m_uiInputIndex])
  {
    outputs[m_PinSampleCount.m_uiOutputIndex] = *inputs[m_PinSampleCount.m_uiInputIndex];
  }
  else
  {
    return false;
  }

  // Depth stencil
  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    outputs[m_PinDepthStencil.m_uiOutputIndex] = *inputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else
  {
    // If no input is available, we use the render target setup instead.
    const ezGALRenderTargetView* pTarget = pDevice->GetRenderTargetView(setup.GetDepthStencilTarget());
    if (pTarget)
    {
      const ezGALRenderTargetViewCreationDescription& desc = pTarget->GetDescription();
      const ezGALTexture* pTexture = pDevice->GetTexture(desc.m_hTexture);
      if (pTexture)
      {
        outputs[m_PinDepthStencil.m_uiOutputIndex] = pTexture->GetDescription();
      }
    }
  }

  return true;
}

void ezStochasticPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection * const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection * const> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  // Setup render target
  ezGALRenderTagetSetup renderTargetSetup;
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    renderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(inputs[m_PinColor.m_uiInputIndex]->m_TextureHandle));
  }

  if (inputs[m_PinSampleCount.m_uiInputIndex])
  {
    renderTargetSetup.SetRenderTarget(1, pDevice->GetDefaultRenderTargetView(inputs[m_PinSampleCount.m_uiInputIndex]->m_TextureHandle));
  }

  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    renderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(inputs[m_PinDepthStencil.m_uiInputIndex]->m_TextureHandle));
  }

  renderViewContext.m_pRenderContext->SetViewportAndRenderTargetSetup(renderViewContext.m_pViewData->m_ViewPortRect, renderTargetSetup);

  if (!m_lastViewProjection.IsIdentical(renderViewContext.m_pViewData->m_ViewProjectionMatrix))
  {
    m_lastViewProjection = renderViewContext.m_pViewData->m_ViewProjectionMatrix;
    pGALContext->Clear(ezColor(0.0f, 0.0f, 0.0f, 0.0f), 0x1, false, false); // Clear render target with index 0 to 0.0f
    pGALContext->Clear(ezColor(1.0f, 1.0f, 1.0f, 1.0f), 0x2, false, false); // Clear render target with index 1 to 1.0f
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("STOCHASTIC_INITIAL", "TRUE");
  }
  else
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("STOCHASTIC_INITIAL", "FALSE");
  }
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("STOCHASTIC_PASS", "TRUE");
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "FORWARD");

  {
    for (auto& number : m_randomNumbers)
    {
      number = (float)m_randomGenerator.DoubleZeroToOneInclusive();
    }

    pGALContext->UpdateBuffer(m_randomNumberBuffer, 0, m_randomNumbers.GetByteArrayPtr());

    renderViewContext.m_pRenderContext->BindBuffer(ezGALShaderStage::PixelShader, "randomNumbers", pDevice->GetDefaultResourceView(m_randomNumberBuffer));
  }

  // Execute render functions
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitOpaque);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("STOCHASTIC_PASS", "FALSE");
}

void ezStochasticPass::InitRenderPipelinePass(const ezArrayPtr<ezRenderPipelinePassConnection * const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection * const> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  m_randomNumbers.SetCount(8196);

  ezGALBufferCreationDescription desc;
  desc.m_bAllowShaderResourceView = true;
  desc.m_BufferType = ezGALBufferType::Generic;
  desc.m_bUseAsStructuredBuffer = true;
  desc.m_uiStructSize = sizeof(float);
  desc.m_uiTotalSize = m_randomNumbers.GetCount() * desc.m_uiStructSize;
  desc.m_ResourceAccess.m_bImmutable = false;

  m_randomNumberBuffer = pDevice->CreateBuffer(desc);
}

