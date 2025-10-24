#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// Shading quality settings for forward rendering.
struct ezForwardRenderShadingQuality
{
  using StorageType = ezInt8;

  enum Enum
  {
    Normal,     ///< Full lighting and shading calculations.
    Simplified, ///< Reduced quality for performance.

    Default = Normal,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezForwardRenderShadingQuality);

/// Base class for forward rendering passes.
///
/// Forward rendering evaluates lighting for each rendered object during the geometry pass.
/// Derived classes implement specific object filtering (opaque, transparent, etc.).
class EZ_RENDERERCORE_DLL ezForwardRenderPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezForwardRenderPass, ezRenderPipelinePass);

public:
  ezForwardRenderPass(const char* szName = "ForwardRenderPass");
  ~ezForwardRenderPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  /// Binds input textures and sets up shader resources.
  virtual void SetupResources(ezGALCommandEncoder* pCommandEncoder, const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs);

  /// Configures shader permutation variables based on render settings.
  virtual void SetupPermutationVars(const ezRenderViewContext& renderViewContext);

  /// Binds lighting data for the current view.
  virtual void SetupLighting(const ezRenderViewContext& renderViewContext);

  /// Renders the objects for this pass. Must be implemented by derived classes.
  virtual void RenderObjects(const ezRenderViewContext& renderViewContext) = 0;

  ezRenderPipelineNodePassThroughPin m_PinColor;          ///< Color target for rendering.
  ezRenderPipelineNodePassThroughPin m_PinDepthStencil;   ///< Depth-stencil target.

  ezEnum<ezForwardRenderShadingQuality> m_ShadingQuality; ///< Quality level for shading calculations.
};
