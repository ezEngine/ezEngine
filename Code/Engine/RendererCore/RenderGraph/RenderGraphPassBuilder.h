#pragma once

#include <Foundation/Math/Color.h>
#include <RendererCore/RenderGraph/Declarations.h>
#include <RendererCore/RenderGraph/RenderGraphContext.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// Returned by the render graph when adding a pass. The caller declares all resource accesses through this builder, then sets an execution callback. Only one instance of this class can exist at any time for each graph.
///
/// For render passes, the render graph will call BeginRendering/EndRendering automatically based on SetColorTarget/SetDepthStencilTarget declarations. The execute callback only needs to bind shaders, resources, and draw.
class EZ_RENDERERCORE_DLL ezRenderGraphPassBuilder
{
public:
  ezRenderGraphPassBuilder() = default;
  ezRenderGraphPassBuilder(ezRenderGraphPassBuilder&& rhs) noexcept;
  ~ezRenderGraphPassBuilder();
  void operator=(ezRenderGraphPassBuilder&& rhs) noexcept;

  /// \name Texture
  ///@{

  /// Declare that this pass reads a texture. `stage` is optional and should only be used in graphics passes when a later stage reads a texture so allow for more concurrent work on the GPU.
  ///
  /// For depth textures, the correct SRV state is `ezGALResourceState::DepthStencilRead` instead of `ezGALResourceState::ShaderResource`, but this function automatically switches this for your convenience.
  ezRenderGraphPassBuilder& ReadTexture(ezRenderGraphTextureHandle hTexture,
    ezGALTextureRange range = {},
    ezBitflags<ezGALResourceState> access = ezGALResourceState::ShaderResource,
    ezBitflags<ezGALShaderStageFlags> stage = ezGALShaderStageFlags::Auto);

  /// Declare that this pass writes to a texture. `stage` is optional and should only be used in graphics passes when a later stage reads a texture so allow for more concurrent work on the GPU.
  ezRenderGraphPassBuilder& WriteTexture(ezRenderGraphTextureHandle hTexture,
    ezGALTextureRange range = {},
    ezBitflags<ezGALResourceState> access = ezGALResourceState::UnorderedAccess,
    ezBitflags<ezGALShaderStageFlags> stage = ezGALShaderStageFlags::Auto);

  ///@}
  /// \name Buffer
  ///@{

  /// Declare that this pass reads a buffer. `stage` is optional and should only be used in graphics passes when a later stage reads a buffer so allow for more concurrent work on the GPU.
  ezRenderGraphPassBuilder& ReadBuffer(ezRenderGraphBufferHandle hBuffer,
    ezBitflags<ezGALResourceState> access = ezGALResourceState::ShaderResource,
    ezBitflags<ezGALShaderStageFlags> stage = ezGALShaderStageFlags::Auto);

  /// Declare that this pass writes to a buffer. `stage` is optional and should only be used in graphics passes when a later stage reads a buffer so allow for more concurrent work on the GPU.
  ezRenderGraphPassBuilder& WriteBuffer(ezRenderGraphBufferHandle hBuffer,
    ezBitflags<ezGALResourceState> access = ezGALResourceState::UnorderedAccess,
    ezBitflags<ezGALShaderStageFlags> stage = ezGALShaderStageFlags::Auto);

  ///@}
  /// \name Render Targets
  ///@{

  /// Adds a texture as a color render target. This will implicitly result in WriteTexture so there is no need to declare read / write operations for render targets.
  ezRenderGraphPassBuilder& AddColorTarget(ezRenderGraphTextureHandle hTexture,
    ezGALRenderTargetRange range = {},
    ezEnum<ezGALRenderTargetLoadOp> loadOp = {},
    ezEnum<ezGALRenderTargetStoreOp> storeOp = {},
    ezEnum<ezGALResourceFormat> overrideViewFormat = ezGALResourceFormat::Invalid,
    ezEnum<ezGALTextureType> overrideViewType = ezGALTextureType::Invalid);

  /// Sets the clear color for the color target at the given index and switches the load op to `Clear`. SetColorTarget must have been called for this index first.
  ezRenderGraphPassBuilder& SetClearColor(ezUInt8 uiIndex, const ezColor& color = ezColor(0, 0, 0, 0));

  /// Bind a texture as the depth/stencil target. This will implicitly result in WriteTexture so there is no need to declare read / write operations for render targets.
  ezRenderGraphPassBuilder& AddDepthStencilTarget(ezRenderGraphTextureHandle hTexture,
    ezGALRenderTargetRange range = {},
    ezEnum<ezGALRenderTargetLoadOp> depthLoadOp = {},
    ezEnum<ezGALRenderTargetStoreOp> depthStoreOp = {},
    ezEnum<ezGALRenderTargetLoadOp> stencilLoadOp = {},
    ezEnum<ezGALRenderTargetStoreOp> stencilStoreOp = {},
    bool bReadOnly = false);

  /// Sets the clear depth value and switches the depth load op to `Clear`. SetDepthStencilTarget must have been called first.
  ezRenderGraphPassBuilder& SetClearDepth(float fDepthClear = 1.0f);

  /// Sets the clear stencil value and switches the stencil load op to `Clear`. SetDepthStencilTarget must have been called first.
  ezRenderGraphPassBuilder& SetClearStencil(ezUInt8 uiStencilClear = 0);


  ///@}
  /// \name Pass Configuration
  ///@{

  /// Marks this pass as having externally visible side effects (readback, etc.). Prevents the pass from being culled even if no other pass reads its outputs.
  ezRenderGraphPassBuilder& HasSideEffects();

  /// Marks this pass as stereoscopic. Only valid for graphics passes.
  ezRenderGraphPassBuilder& SetStereoscopic(bool bStereoscopic = true);

  /// Set the callback invoked when this pass executes. For render passes, BeginRendering will already be active when the callback is invoked.
  /// As the execute callback is called at a later date, make sure to capture variables by value if using a lambda.
  ezRenderGraphPassBuilder& SetExecuteCallback(ezRenderGraphExecuteFunction callback);

  ///@}

private:
  friend class ezRenderGraph;
  ezRenderGraphPassBuilder(ezRenderGraph* pParent);

private:
  ezRenderGraph* m_pParent = nullptr;
};
