#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>

struct ezForwardRenderShadingQuality
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    Normal,
    Simplified,

    Default = Normal,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezForwardRenderShadingQuality);

/// \brief A standard forward render pass that renders into the color target.
class EZ_RENDERERCORE_DLL ezForwardRenderPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezForwardRenderPass, ezRenderPipelinePass);

public:
  ezForwardRenderPass(const char* szName = "ForwardRenderPass");
  ~ezForwardRenderPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

protected:
  virtual void SetupResources(ezGALPass* pGALPass, const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs);
  virtual void SetupPermutationVars(const ezRenderViewContext& renderViewContext);
  virtual void SetupLighting(const ezRenderViewContext& renderViewContext);

  virtual void RenderObjects(const ezRenderViewContext& renderViewContext) = 0;

  ezRenderPipelineNodePassThrougPin m_PinColor;
  ezRenderPipelineNodePassThrougPin m_PinDepthStencil;

  ezEnum<ezForwardRenderShadingQuality> m_ShadingQuality;
};
