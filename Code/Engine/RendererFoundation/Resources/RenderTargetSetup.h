#pragma once

#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Basics.h>
#include <Foundation/Math/Size.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

/// \brief This class can be used to define the render targets to be used by an ezView.
struct EZ_RENDERERFOUNDATION_DLL ezGALRenderTargets
{
  bool operator==(const ezGALRenderTargets& other) const;
  bool operator!=(const ezGALRenderTargets& other) const;

  ezGALTextureHandle m_hRTs[EZ_GAL_MAX_RENDERTARGET_COUNT];
  ezGALTextureHandle m_hDSTarget;
};

/// \brief This class is used to describe the render pass setup of a graphics pipeline.
/// This should not be filled out manually, but rather be created by the ezGALRenderingSetup.
/// The render pass descriptor is eventually used by the renderer to generate both a render pass as well as compatible graphics pipeline state objects.
/// \sa ezGALRenderingSetup
struct ezGALRenderPassDescriptor : public ezHashableStruct<ezGALRenderPassDescriptor>
{
  ezUInt8 m_uiRTCount = 0;
  ezEnum<ezGALMSAASampleCount> m_Msaa;
  ezEnum<ezGALResourceFormat> m_DepthFormat = ezGALResourceFormat::Invalid;
  ezEnum<ezGALRenderTargetLoadOp> m_DepthLoadOp;
  ezEnum<ezGALRenderTargetStoreOp> m_DepthStoreOp;
  ezEnum<ezGALRenderTargetLoadOp> m_StencilLoadOp;
  ezEnum<ezGALRenderTargetStoreOp> m_StencilStoreOp;
  ezEnum<ezGALResourceFormat> m_ColorFormat[EZ_GAL_MAX_RENDERTARGET_COUNT];
  ezEnum<ezGALRenderTargetLoadOp> m_ColorLoadOp[EZ_GAL_MAX_RENDERTARGET_COUNT];
  ezEnum<ezGALRenderTargetStoreOp> m_ColorStoreOp[EZ_GAL_MAX_RENDERTARGET_COUNT];
};

/// \brief This class is used to describe the frame buffer of a graphics pipeline.
/// This should not be filled out manually, but rather be created by the ezGALRenderingSetup.
/// The frame buffer descriptor is used by the renderer to set up render targets to render into.
/// \sa ezGALRenderingSetup
struct ezGALFrameBufferDescriptor : public ezHashableStruct<ezGALFrameBufferDescriptor>
{
  ezGALRenderTargetViewHandle m_hColorTarget[EZ_GAL_MAX_RENDERTARGET_COUNT];
  ezGALRenderTargetViewHandle m_hDepthTarget;
  ezSizeU32 m_Size = {0, 0};
  ezUInt32 m_uiSliceCount = 0;
};

/// \brief This class sets up the render targets for a graphics pipeline, used by ezRenderContext::BeginRendering or ezGALCommandEncoder::BeginRendering.
///
/// The usage pattern is very strict to prevent creating invalid render target configs: SetColorTarget must be called starting at uiIndex 0 before you can call it with index 1 and so fourth. To call any of the SetClear* functions, you first must have called the corresponding SetColorTarget or SetDepthStencilTarget functions. All clear methods have reasonable default values (color: black, depth: 1.0, stencil: 0). SetColorTarget and SetDepthStencilTarget can be called multiple times as long as the format of the view remains identical to the previously assigned one. Thus, while not very expensive, it is still a good idea to cache these objects and only swap out the textures when they are not fixed, e.g. when using the ezGPUResourcePool.
/// The class produces a ezGALRenderPassDescriptor and a ezGALFrameBufferDescriptor that can be retrieved via GetRenderPass and GetFrameBuffer respectively.
///
/// Example usage:
/// \code{.cpp}
/// renderTargetSetup.SetColorTarget(0, hRT)).SetClearColor(0, ezColor::White)
/// renderTargetSetup.SetDepthStencilTarget(hDepth)).SetClearDepth().SetClearStencil();
/// \endcode
///
/// \sa ezGALRenderPassDescriptor, ezGALFrameBufferDescriptor, ezRenderContext::BeginRendering, ezGALCommandEncoder::BeginRendering
struct EZ_RENDERERFOUNDATION_DLL ezGALRenderingSetup : public ezHashableStruct<ezGALRenderingSetup>
{
public:
  /// \name Setup render targets
  ///@{

  /// \brief Sets the color render target at the given index.
  /// Note that you must set index 0 before you can call this with index 1 and so fourth. You can call the function for an already set index again as long as the format / size matches.
  /// If not set, the load and store operations default to 'Load' and 'Store' respectively.
  ezGALRenderingSetup& SetColorTarget(ezUInt8 uiIndex, ezGALRenderTargetViewHandle hRenderTarget, ezEnum<ezGALRenderTargetLoadOp> loadOp = {}, ezEnum<ezGALRenderTargetStoreOp> storeOp = {});

  /// \brief Sets the depth / stencil render target.
  /// If not set, the load and store operations default to 'Load' and 'Store' respectively for both depth and stencil.
  ezGALRenderingSetup& SetDepthStencilTarget(ezGALRenderTargetViewHandle hDSTarget, ezEnum<ezGALRenderTargetLoadOp> depthLoadOp = {}, ezEnum<ezGALRenderTargetStoreOp> depthStoreOp = {}, ezEnum<ezGALRenderTargetLoadOp> stencilLoadOp = {}, ezEnum<ezGALRenderTargetStoreOp> stencilStoreOp = {});

  /// \brief Sets the clear color of the given render target and switches the load op to clear.
  /// Note that you first must have called SetColorTarget with the same uiIndex to be able to set the clear color.
  ezGALRenderingSetup& SetClearColor(ezUInt8 uiIndex, const ezColor& color = ezColor(0, 0, 0, 0));

  /// \brief Sets the clear depth value.
  /// Note that you first must have called SetDepthStencilTarget to be able to set the clear depth value.
  ezGALRenderingSetup& SetClearDepth(float fDepthClear = 1.0f);

  /// \brief Sets the clear depth value.
  /// Note that you first must have called SetDepthStencilTarget to be able to set the clear depth value.
  ezGALRenderingSetup& SetClearStencil(ezUInt8 uiStencilClear = 0);

  ///@}
  /// \name Accessors
  ///@{

  inline ezUInt8 GetColorTargetCount() const;
  /// \brief Returns the clear color of the render target at the given index. Note that uiIndex must be less than GetColorTargetCount().
  inline const ezColor& GetClearColor(ezUInt8 uiIndex) const;

  inline bool HasDepthStencilTarget() const;
  /// \brief Returns the depth clear value. This call is only valid if HasDepthStencilTarget() returns true.
  inline float GetClearDepth() const;
  /// \brief Returns the stencil clear value. This call is only valid if HasDepthStencilTarget() returns true.
  inline ezUInt8 GetClearStencil() const;

  /// \brief Returns the render pass description. Used to create a render pass or feed into the creation of a graphics pipeline state object.
  inline const ezGALRenderPassDescriptor& GetRenderPass() const;
  /// \brief Returns the frame buffer description. Used by the renderer to setup render targets.
  inline const ezGALFrameBufferDescriptor& GetFrameBuffer() const;

  ///@}

  /// \brief Same as creating a new instance.
  void Reset();
  /// \brief Same as Reset, but destroys all attached views first.
  void DestroyAllAttachedViews();

protected:
  ezUInt8 m_uiClearStencil = 0;
  float m_fClearDepth = 1.0f;
  ezColor m_ClearColor[EZ_GAL_MAX_RENDERTARGET_COUNT] = {ezColor(0, 0, 0, 0)};

  ezGALRenderPassDescriptor m_RenderPass;
  ezGALFrameBufferDescriptor m_FrameBuffer;
};

#include <RendererFoundation/Resources/Implementation/RenderTargetSetup_inl.h>
