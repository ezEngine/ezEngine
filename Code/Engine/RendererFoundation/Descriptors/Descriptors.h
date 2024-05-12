
#pragma once

#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>
#include <Texture/Image/ImageEnums.h>

class ezWindowBase;



struct ezGALWindowSwapChainCreationDescription : public ezHashableStruct<ezGALWindowSwapChainCreationDescription>
{
  ezWindowBase* m_pWindow = nullptr;

  // Describes the format that should be used for the backbuffer.
  // Note however, that different platforms may enforce restrictions on this.
  ezGALMSAASampleCount::Enum m_SampleCount = ezGALMSAASampleCount::None;
  ezGALResourceFormat::Enum m_BackBufferFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
  ezEnum<ezGALPresentMode> m_InitialPresentMode = ezGALPresentMode::VSync;

  bool m_bDoubleBuffered = true;
  bool m_bAllowScreenshots = false;
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
  ~ezGALShaderCreationDescription();

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

struct EZ_RENDERERFOUNDATION_DLL ezGALVertexAttribute
{
  ezGALVertexAttribute() = default;

  ezGALVertexAttribute(ezGALVertexAttributeSemantic::Enum semantic, ezGALResourceFormat::Enum format, ezUInt16 uiOffset, ezUInt8 uiVertexBufferSlot,
    bool bInstanceData);

  ezGALVertexAttributeSemantic::Enum m_eSemantic = ezGALVertexAttributeSemantic::Position;
  ezGALResourceFormat::Enum m_eFormat = ezGALResourceFormat::XYZFloat;
  ezUInt16 m_uiOffset = 0;
  ezUInt8 m_uiVertexBufferSlot = 0;
  bool m_bInstanceData = false;
};

struct EZ_RENDERERFOUNDATION_DLL ezGALVertexDeclarationCreationDescription : public ezHashableStruct<ezGALVertexDeclarationCreationDescription>
{
  ezGALShaderHandle m_hShader;
  ezStaticArray<ezGALVertexAttribute, 16> m_VertexAttributes;
};

// Need to add: immutable (GPU only), default(GPU only, but allows CopyToTempStorage updates), transient (allows ezGALUpdateMode::Discard), staging: read(back), staging: write (constantly mapped), unified memory (mobile, onboard GPU, allows all ops)
// Or use VmaMemoryUsage  + read write flags?
struct ezGALResourceAccess
{
  EZ_ALWAYS_INLINE bool IsImmutable() const { return m_bImmutable; }

  bool m_bReadBack = false;
  bool m_bImmutable = true;
};

struct ezGALBufferCreationDescription : public ezHashableStruct<ezGALBufferCreationDescription>
{
  ezUInt32 m_uiTotalSize = 0;
  ezUInt32 m_uiStructSize = 0; // Struct or texel size
  ezBitflags<ezGALBufferFlags> m_BufferFlags;
  ezGALResourceAccess m_ResourceAccess;
};

struct ezGALTextureCreationDescription : public ezHashableStruct<ezGALTextureCreationDescription>
{
  void SetAsRenderTarget(ezUInt32 uiWidth, ezUInt32 uiHeight, ezGALResourceFormat::Enum format, ezGALMSAASampleCount::Enum sampleCount = ezGALMSAASampleCount::None);

  ezUInt32 m_uiWidth = 0;
  ezUInt32 m_uiHeight = 0;
  ezUInt32 m_uiDepth = 1;
  ezUInt32 m_uiMipLevelCount = 1;
  ezUInt32 m_uiArraySize = 1;

  ezEnum<ezGALResourceFormat> m_Format = ezGALResourceFormat::Invalid;
  ezEnum<ezGALMSAASampleCount> m_SampleCount = ezGALMSAASampleCount::None;
  ezEnum<ezGALTextureType> m_Type = ezGALTextureType::Texture2D;

  bool m_bAllowShaderResourceView = true;
  bool m_bAllowUAV = false;
  bool m_bCreateRenderTarget = false;
  bool m_bAllowDynamicMipGeneration = false;

  ezGALResourceAccess m_ResourceAccess;

  void* m_pExisitingNativeObject = nullptr; ///< Can be used to encapsulate existing native textures in objects usable by the GAL
};

struct ezGALTextureResourceViewCreationDescription : public ezHashableStruct<ezGALTextureResourceViewCreationDescription>
{
  ezGALTextureHandle m_hTexture;
  ezEnum<ezGALResourceFormat> m_OverrideViewFormat = ezGALResourceFormat::Invalid;
  ezUInt32 m_uiMostDetailedMipLevel = 0;
  ezUInt32 m_uiMipLevelsToUse = 0xFFFFFFFFu;
  ezUInt32 m_uiFirstArraySlice = 0; // For cubemap array: index of first 2d slice to start with
  ezUInt32 m_uiArraySize = 1;       // For cubemap array: number of cubemaps
};

struct ezGALBufferResourceViewCreationDescription : public ezHashableStruct<ezGALBufferResourceViewCreationDescription>
{
  ezGALBufferHandle m_hBuffer;
  ezEnum<ezGALResourceFormat> m_OverrideViewFormat = ezGALResourceFormat::Invalid;
  ezUInt32 m_uiFirstElement = 0;
  ezUInt32 m_uiNumElements = 0;
  bool m_bRawView = false;
};

struct ezGALRenderTargetViewCreationDescription : public ezHashableStruct<ezGALRenderTargetViewCreationDescription>
{
  ezGALTextureHandle m_hTexture;

  ezEnum<ezGALResourceFormat> m_OverrideViewFormat = ezGALResourceFormat::Invalid;

  ezUInt32 m_uiMipLevel = 0;

  ezUInt32 m_uiFirstSlice = 0;
  ezUInt32 m_uiSliceCount = 1;

  bool m_bReadOnly = false; ///< Can be used for depth stencil views to create read only views (e.g. for soft particles using the native depth buffer)
};

struct ezGALTextureUnorderedAccessViewCreationDescription : public ezHashableStruct<ezGALTextureUnorderedAccessViewCreationDescription>
{
  ezGALTextureHandle m_hTexture;
  ezUInt32 m_uiFirstArraySlice = 0; ///< First depth slice for 3D Textures.
  ezUInt32 m_uiArraySize = 1;       ///< Number of depth slices for 3D textures.
  ezUInt16 m_uiMipLevelToUse = 0;   ///< Which MipLevel is accessed with this UAV
  ezEnum<ezGALResourceFormat> m_OverrideViewFormat = ezGALResourceFormat::Invalid;
};

struct ezGALBufferUnorderedAccessViewCreationDescription : public ezHashableStruct<ezGALBufferUnorderedAccessViewCreationDescription>
{
  ezGALBufferHandle m_hBuffer;
  ezUInt32 m_uiFirstElement = 0;
  ezUInt32 m_uiNumElements = 0;
  ezEnum<ezGALResourceFormat> m_OverrideViewFormat = ezGALResourceFormat::Invalid;
  bool m_bRawView = false;
};

struct ezGALQueryCreationDescription : public ezHashableStruct<ezGALQueryCreationDescription>
{
  ezEnum<ezGALQueryType> m_type = ezGALQueryType::NumSamplesPassed;

  /// In case this query is used for occlusion culling (type AnySamplesPassed), this determines whether drawing should be done if the query
  /// status is still unknown.
  bool m_bDrawIfUnknown = true;
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
    BeforeBeginPipeline,
    AfterBeginPipeline,
    BeforeEndPipeline,
    AfterEndPipeline,
    // could add resource creation/destruction events, if this would be useful
  };

  Type m_Type;
  class ezGALDevice* m_pDevice = nullptr;
  ezGALSwapChainHandle m_hSwapChain;
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
