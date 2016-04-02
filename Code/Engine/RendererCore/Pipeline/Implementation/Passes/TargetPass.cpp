#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTargetPass, 1, ezRTTIDefaultAllocator<ezTargetPass>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Color0", m_PinColor0),
    EZ_MEMBER_PROPERTY("Color1", m_PinColor1),
    EZ_MEMBER_PROPERTY("Color2", m_PinColor2),
    EZ_MEMBER_PROPERTY("Color3", m_PinColor3),
    EZ_MEMBER_PROPERTY("Color4", m_PinColor4),
    EZ_MEMBER_PROPERTY("Color5", m_PinColor5),
    EZ_MEMBER_PROPERTY("Color6", m_PinColor6),
    EZ_MEMBER_PROPERTY("Color7", m_PinColor7),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil)
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTargetPass::ezTargetPass(const char* szName) : ezRenderPipelinePass(szName)
{
}

ezTargetPass::~ezTargetPass()
{

}

ezGALTextureHandle ezTargetPass::GetTextureHandle(const ezView& view, const ezNodePin* pPin)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  const ezGALRenderTagetSetup& setup = view.GetRenderTargetSetup();
  
  auto inputs = GetInputPins();
  if (pPin->m_pParent != this)
  {
    ezLog::Error("ezTargetPass::GetTextureHandle: The given pin is not part of this pass!");
    return ezGALTextureHandle();
  }

  ezGALRenderTargetViewHandle hTarget;
  if (pPin->m_uiInputIndex == 8)
  {
    hTarget = setup.GetDepthStencilTarget();
  }
  else
  {
    hTarget = setup.GetRenderTarget(pPin->m_uiInputIndex);
  }

  const ezGALRenderTargetView* pTarget = pDevice->GetRenderTargetView(hTarget);
  if (pTarget)
  {
    const ezGALRenderTargetViewCreationDescription& desc = pTarget->GetDescription();
    return desc.m_hTexture;
  }

  return ezGALTextureHandle();
}

bool ezTargetPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription*const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  const char* pinNames[] = { "Color0", "Color1", "Color2", "Color3", "Color4", "Color5", "Color6", "Color7", "DepthStencil", };

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(pinNames); ++i)
  {
    if (!VerifyInput(view, inputs, pinNames[i]))
      return false;
  }

  return true;
}

void ezTargetPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
}

bool ezTargetPass::VerifyInput(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription*const> inputs, const char* szPinName)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  const ezNodePin* pPin = GetPinByName(szPinName);
  if (inputs[pPin->m_uiInputIndex])
  {
    const ezGALTexture* pTexture = pDevice->GetTexture(GetTextureHandle(view, pPin));
    if (pTexture)
    {
      // TODO: Need a more sophisticated check here what is considered 'matching'
      //if (inputs[pPin->m_uiInputIndex]->CalculateHash() != pTexture->GetDescription().CalculateHash())
      //  return false;
    }
  }

  return true;
}
