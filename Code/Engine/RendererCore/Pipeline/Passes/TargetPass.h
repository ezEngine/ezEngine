#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

struct ezGALRenderTargets;

/// Render pass that outputs to final render targets or swap chain.
///
/// Terminal pass that writes the final pipeline output to the specified render targets.
/// Can output to multiple color targets and depth-stencil, typically used as the final
/// stage in a render pipeline to present results to the screen or external targets.
class EZ_RENDERERCORE_DLL ezTargetPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTargetPass, ezRenderPipelinePass);

public:
  ezTargetPass(const char* szName = "TargetPass");
  ~ezTargetPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  /// Provides the actual target texture handles for connected pins.
  virtual ezGALTextureHandle QueryTextureProvider(const ezRenderPipelineNodePin* pPin, const ezGALTextureCreationDescription& desc) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

private:
  /// Validates that an input pin's texture matches expected dimensions and format.
  bool VerifyInput(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, const char* szPinName);

protected:
  ezRenderPipelineNodeInputProviderPin m_PinColor0;       ///< Color target 0 input.
  ezRenderPipelineNodeInputProviderPin m_PinColor1;       ///< Color target 1 input.
  ezRenderPipelineNodeInputProviderPin m_PinColor2;       ///< Color target 2 input.
  ezRenderPipelineNodeInputProviderPin m_PinColor3;       ///< Color target 3 input.
  ezRenderPipelineNodeInputProviderPin m_PinColor4;       ///< Color target 4 input.
  ezRenderPipelineNodeInputProviderPin m_PinColor5;       ///< Color target 5 input.
  ezRenderPipelineNodeInputProviderPin m_PinColor6;       ///< Color target 6 input.
  ezRenderPipelineNodeInputProviderPin m_PinColor7;       ///< Color target 7 input.
  ezRenderPipelineNodeInputProviderPin m_PinDepthStencil; ///< Depth-stencil target input.

  ezGALRenderTargets m_RenderTargets;                     ///< Configured render target setup.
  ezGALSwapChainHandle m_hSwapChain;                      ///< Swap chain handle if rendering to screen.
};
