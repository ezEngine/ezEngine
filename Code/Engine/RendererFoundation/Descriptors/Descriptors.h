
#pragma once

#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Resources/ResourceFormats.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>
#include <Texture/Image/ImageEnums.h>

class ezWindowBase;

/// \brief Bind group layout for a single bind group.
/// Auto created by shader resource. Mostly used to quickly determine if a bind group still matches after e.g. switching the shader.
struct ezGALBindGroupLayoutCreationDescription
{
  ezUInt32 CalculateHash() const;

  ezDynamicArray<ezShaderResourceBinding> m_ResourceBindings;    ///< Must be sorted by m_iSlot. m_iBindGroup must be the same for all bindings in this array.
  ezHybridArray<ezShaderResourceBinding, 1> m_ImmutableSamplers; ///< If supported by the platform, contains immutable samplers. See ezGALImmutableSamplers.
};

/// \brief Push constant info
/// Used by ezGALPipelineLayoutCreationDescription.
struct ezGALPushConstant
{
  ezUInt16 m_uiSize = 0;
  ezUInt16 m_uiOffset = 0;
  ezBitflags<ezGALShaderStageFlags> m_Stages;
};

/// \brief Pipeline layout.
/// Auto created by shader resource. Mostly used for de-duplication of native resources in case pipelines share the same layout.
struct ezGALPipelineLayoutCreationDescription : public ezHashableStruct<ezGALPipelineLayoutCreationDescription>
{
  ezGALBindGroupLayoutHandle m_BindGroups[EZ_GAL_MAX_BIND_GROUPS]; ///< One for each bind group used in the shader. BG_FRAME, BG_RENDER_PASS, BG_MATERIAL, BG_DRAW_CALL.
  ezGALPushConstant m_PushConstants;                        ///< Only one push constant block is supported right now.
};

/// \brief Defines the complete state of a graphics pipeline, excluding bound resources (e.g. textures, buffers) and dynamic states (e.g. viewport).
/// All handles must be set except for m_hVertexDeclaration which is optional. Creating a graphics pipeline increases the reference count on all valid handles.
struct ezGALGraphicsPipelineCreationDescription : public ezHashableStruct<ezGALGraphicsPipelineCreationDescription>
{
  ezGALShaderHandle m_hShader;                       ///< Also defines pipeline layout
  ezGALVertexDeclarationHandle m_hVertexDeclaration; ///< Optional

  ezGALRasterizerStateHandle m_hRasterizerState;
  ezGALBlendStateHandle m_hBlendState;
  ezGALDepthStencilStateHandle m_hDepthStencilState;

  ezEnum<ezGALPrimitiveTopology> m_Topology;

  ezGALRenderPassDescriptor m_RenderPass; ///< Use ezGALRenderingSetup::GetRenderPass to set this.
};

/// \brief Defines the complete state of a compute pipeline, excluding bound resources (e.g. textures, buffers).
/// Creating a compute pipeline increases the reference count on the shader handle.
struct ezGALComputePipelineCreationDescription : public ezHashableStruct<ezGALComputePipelineCreationDescription>
{
  ezGALShaderHandle m_hShader; ///< Also defines pipeline layout
};

struct ezGALWindowSwapChainCreationDescription : public ezHashableStruct<ezGALWindowSwapChainCreationDescription>
{
  ezWindowBase* m_pWindow = nullptr;

  // Describes the format that should be used for the backbuffer.
  // Note however, that different platforms may enforce restrictions on this.
  ezGALMSAASampleCount::Enum m_SampleCount = ezGALMSAASampleCount::None;
  ezGALResourceFormat::Enum m_BackBufferFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
  ezEnum<ezGALPresentMode> m_InitialPresentMode = ezGALPresentMode::VSync;

  bool m_bDoubleBuffered = true;
};

struct ezGALSwapChainCreationDescription : public ezHashableStruct<ezGALSwapChainCreationDescription>
{
  const ezRTTI* m_pSwapChainType = nullptr;
};

struct ezGALDeviceCreationDescription
{
  bool m_bDebugDevice = false;
};

struct ezGALShaderCreationDescription : public ezHashableStruct<ezGALShaderCreationDescription>
{
  ezGALShaderCreationDescription();
  /// \brief Needs to be overwritten as the base class impl can only handle pod types.
  ezGALShaderCreationDescription(const ezGALShaderCreationDescription& other);
  ~ezGALShaderCreationDescription();
  /// \brief Needs to be overwritten as the base class impl can only handle pod types.
  void operator=(const ezGALShaderCreationDescription& other);

  bool HasByteCodeForStage(ezGALShaderStage::Enum stage) const;

  ezSharedPtr<ezGALShaderByteCode> m_ByteCodes[ezGALShaderStage::ENUM_COUNT];
};

struct ezGALRenderTargetBlendDescription : public ezHashableStruct<ezGALRenderTargetBlendDescription>
{
  ezEnum<ezGALBlend> m_SourceBlend = ezGALBlend::One;
  ezEnum<ezGALBlend> m_DestBlend = ezGALBlend::One;
  ezEnum<ezGALBlendOp> m_BlendOp = ezGALBlendOp::Add;

  ezEnum<ezGALBlend> m_SourceBlendAlpha = ezGALBlend::One;
  ezEnum<ezGALBlend> m_DestBlendAlpha = ezGALBlend::One;
  ezEnum<ezGALBlendOp> m_BlendOpAlpha = ezGALBlendOp::Add;

  ezUInt8 m_uiWriteMask = 0xFF;    ///< Enables writes to color channels. Bit1 = Red Channel, Bit2 = Green Channel, Bit3 = Blue Channel, Bit4 = Alpha
                                   ///< Channel, Bit 5-8 are unused
  bool m_bBlendingEnabled = false; ///< If enabled, the color will be blended into the render target. Otherwise it will overwrite the render target.
                                   ///< Set m_uiWriteMask to 0 to disable all writes to the render target.
};

struct ezGALBlendStateCreationDescription : public ezHashableStruct<ezGALBlendStateCreationDescription>
{
  ezGALRenderTargetBlendDescription m_RenderTargetBlendDescriptions[EZ_GAL_MAX_RENDERTARGET_COUNT];

  bool m_bAlphaToCoverage = false;  ///< Alpha-to-coverage can only be used with MSAA render targets. Default is false.
  bool m_bIndependentBlend = false; ///< If disabled, the blend state of the first render target is used for all render targets. Otherwise each
                                    ///< render target uses a different blend state.
};

struct ezGALStencilOpDescription : public ezHashableStruct<ezGALStencilOpDescription>
{
  ezEnum<ezGALStencilOp> m_FailOp = ezGALStencilOp::Keep;
  ezEnum<ezGALStencilOp> m_DepthFailOp = ezGALStencilOp::Keep;
  ezEnum<ezGALStencilOp> m_PassOp = ezGALStencilOp::Keep;

  ezEnum<ezGALCompareFunc> m_StencilFunc = ezGALCompareFunc::Always;
};

struct ezGALDepthStencilStateCreationDescription : public ezHashableStruct<ezGALDepthStencilStateCreationDescription>
{
  ezGALStencilOpDescription m_FrontFaceStencilOp;
  ezGALStencilOpDescription m_BackFaceStencilOp;

  ezEnum<ezGALCompareFunc> m_DepthTestFunc = ezGALCompareFunc::Less;

  bool m_bSeparateFrontAndBack = false; ///< If false, DX11 will use front face values for both front & back face values, GL will not call
                                        ///< gl*Separate() funcs
  bool m_bDepthTest = true;
  bool m_bDepthWrite = true;
  bool m_bStencilTest = false;
  ezUInt8 m_uiStencilReadMask = 0xFF;
  ezUInt8 m_uiStencilWriteMask = 0xFF;
};

/// \brief Describes the settings for a new rasterizer state. See ezGALDevice::CreateRasterizerState
struct ezGALRasterizerStateCreationDescription : public ezHashableStruct<ezGALRasterizerStateCreationDescription>
{
  ezEnum<ezGALCullMode> m_CullMode = ezGALCullMode::Back; ///< Which sides of a triangle to cull. Default is ezGALCullMode::Back
  ezInt32 m_iDepthBias = 0;                               ///< The pixel depth bias. Default is 0
  float m_fDepthBiasClamp = 0.0f;                         ///< The pixel depth bias clamp. Default is 0
  float m_fSlopeScaledDepthBias = 0.0f;                   ///< The pixel slope scaled depth bias clamp. Default is 0
  bool m_bWireFrame = false;                              ///< Whether triangles are rendered filled or as wireframe. Default is false
  bool m_bFrontCounterClockwise = false;                  ///< Sets which triangle winding order defines the 'front' of a triangle. If true, the front of a triangle
                                                          ///< is the one where the vertices appear in counter clockwise order. Default is false
  bool m_bScissorTest = false;
  bool m_bConservativeRasterization = false;              ///< Whether conservative rasterization is enabled
};

struct ezGALSamplerStateCreationDescription : public ezHashableStruct<ezGALSamplerStateCreationDescription>
{
  ezEnum<ezGALTextureFilterMode> m_MinFilter;
  ezEnum<ezGALTextureFilterMode> m_MagFilter;
  ezEnum<ezGALTextureFilterMode> m_MipFilter;

  ezEnum<ezImageAddressMode> m_AddressU;
  ezEnum<ezImageAddressMode> m_AddressV;
  ezEnum<ezImageAddressMode> m_AddressW;

  ezEnum<ezGALCompareFunc> m_SampleCompareFunc;

  ezColor m_BorderColor = ezColor::Black;

  float m_fMipLodBias = 0.0f;
  float m_fMinMip = -1.0f;
  float m_fMaxMip = 42000.0f;

  ezUInt32 m_uiMaxAnisotropy = 4;
};

struct EZ_RENDERERFOUNDATION_DLL ezGALVertexBinding
{
  ezUInt32 m_uiStride = 0;
  ezEnum<ezGALVertexBindingRate> m_Rate;
};

struct EZ_RENDERERFOUNDATION_DLL ezGALVertexAttribute
{
  EZ_DECLARE_POD_TYPE();

  ezGALVertexAttribute() = default;

  constexpr ezGALVertexAttribute(ezGALVertexAttributeSemantic::Enum semantic, ezGALResourceFormat::Enum format, ezUInt8 uiOffset, ezUInt8 uiVertexBufferSlot);

  ezGALVertexAttributeSemantic::Enum m_eSemantic = ezGALVertexAttributeSemantic::Position;
  ezGALResourceFormat::Enum m_eFormat = ezGALResourceFormat::XYZFloat;
  ezUInt8 m_uiOffset = 0;
  ezUInt8 m_uiVertexBufferSlot = 0;
};

struct EZ_RENDERERFOUNDATION_DLL ezGALVertexDeclarationCreationDescription : public ezHashableStruct<ezGALVertexDeclarationCreationDescription>
{
  ezGALShaderHandle m_hShader; // Needed for attribute indices
  ezStaticArray<ezGALVertexAttribute, EZ_GAL_MAX_VERTEX_ATTRIBUTE_COUNT> m_VertexAttributes;
  ezStaticArray<ezGALVertexBinding, EZ_GAL_MAX_VERTEX_BUFFER_COUNT> m_VertexBindings;
};

struct ezGALResourceAccess
{
  EZ_ALWAYS_INLINE bool IsImmutable() const { return m_bImmutable; }

  bool m_bImmutable = true;
};

struct ezGALBufferCreationDescription : public ezHashableStruct<ezGALBufferCreationDescription>
{
  ezUInt32 m_uiTotalSize = 0;           ///< Total size in bytes. Must always be set > 0.
  ezUInt32 m_uiStructSize = 0;          ///< Struct, Index or Vertex size in bytes. Only valid if StructuredBuffer, VertexBuffer or IndexBuffer flag is set.
  ezBitflags<ezGALBufferUsageFlags> m_BufferFlags;
  ezGALResourceAccess m_ResourceAccess;
  ezEnum<ezGALResourceFormat> m_Format; ///< Only relevant for TexelBuffer to create the default view.
};

struct ezGALTextureCreationDescription : public ezHashableStruct<ezGALTextureCreationDescription>
{
  void SetAsRenderTarget(ezUInt32 uiWidth, ezUInt32 uiHeight, ezGALResourceFormat::Enum format, ezGALMSAASampleCount::Enum sampleCount = ezGALMSAASampleCount::None);

  ezUInt32 m_uiWidth = 0;
  ezUInt32 m_uiHeight = 0;
  ezUInt32 m_uiDepth = 1;
  ezUInt32 m_uiMipLevelCount = 1;
  ezUInt32 m_uiArraySize = 1; ///< In case of cube maps, the number of cubes instead of faces.

  ezEnum<ezGALResourceFormat> m_Format = ezGALResourceFormat::Invalid;
  ezEnum<ezGALMSAASampleCount> m_SampleCount = ezGALMSAASampleCount::None;
  ezEnum<ezGALTextureType> m_Type = ezGALTextureType::Texture2D;

  bool m_bAllowShaderResourceView = true;
  bool m_bAllowUAV = false;
  bool m_bAllowRenderTargetView = false;
  bool m_bAllowDynamicMipGeneration = false;

  ezGALResourceAccess m_ResourceAccess;

  void* m_pExisitingNativeObject = nullptr; ///< Can be used to encapsulate existing native textures in objects usable by the GAL
};

struct ezGALRenderTargetViewCreationDescription : public ezHashableStruct<ezGALRenderTargetViewCreationDescription>
{
  ezGALTextureHandle m_hTexture;

  ezEnum<ezGALResourceFormat> m_OverrideViewFormat = ezGALResourceFormat::Invalid;
  ezEnum<ezGALTextureType> m_OverrideViewType = ezGALTextureType::Invalid;

  ezUInt32 m_uiMipLevel = 0;

  ezUInt32 m_uiFirstSlice = 0;
  ezUInt32 m_uiSliceCount = 1;

  bool m_bReadOnly = false; ///< Can be used for depth stencil views to create read only views (e.g. for soft particles using the native depth buffer)
};

/// \brief Type for important GAL events.
struct ezGALDeviceEvent
{
  enum Type
  {
    AfterInit,
    BeforeShutdown,
    BeforeBeginFrame,
    AfterBeginFrame,
    BeforeEndFrame,
    AfterEndFrame,
    BeforeBeginCommands,
    AfterBeginCommands,
    BeforeEndCommands,
    AfterEndCommands,
    // could add resource creation/destruction events, if this would be useful
  };

  Type m_Type;
  class ezGALDevice* m_pDevice = nullptr;
  ezGALCommandEncoder* m_pCommandEncoder = nullptr;
};

// Opaque platform specific handle
// Typically holds a platform specific handle for the texture and it's synchronization primitive
struct ezGALPlatformSharedHandle : public ezHashableStruct<ezGALPlatformSharedHandle>
{
  ezUInt64 m_hSharedTexture = 0;
  ezUInt64 m_hSemaphore = 0;
  ezUInt32 m_uiProcessId = 0;
  ezUInt32 m_uiMemoryTypeIndex = 0;
  ezUInt64 m_uiSize = 0;
};

#include <RendererFoundation/Descriptors/Implementation/Descriptors_inl.h>
