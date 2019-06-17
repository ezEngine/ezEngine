
#pragma once

#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Types/RefCounted.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>
#include <Texture/Image/ImageEnums.h>

class ezWindowBase;

struct ezGALSwapChainCreationDescription : public ezHashableStruct<ezGALSwapChainCreationDescription>
{
  ezWindowBase* m_pWindow = nullptr;

  // Describes the format that should be used for the backbuffer.
  // Note however, that different platforms may enforce restrictions on this.
  ezGALMSAASampleCount::Enum m_SampleCount = ezGALMSAASampleCount::None;
  ezGALResourceFormat::Enum m_BackBufferFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;

  bool m_bDoubleBuffered = true;
  bool m_bAllowScreenshots = false;
};

struct ezGALDeviceCreationDescription
{
  ezGALSwapChainCreationDescription m_PrimarySwapChainDescription;
  bool m_bDebugDevice = false;
  bool m_bCreatePrimarySwapChain = true;
};

struct ezGALShaderCreationDescription : public ezHashableStruct<ezGALShaderCreationDescription>
{
  ezGALShaderCreationDescription();
  ~ezGALShaderCreationDescription();

  bool HasByteCodeForStage(ezGALShaderStage::Enum Stage) const;

  ezScopedRefPointer<ezGALShaderByteCode> m_ByteCodes[ezGALShaderStage::ENUM_COUNT];
};

struct ezGALRenderTargetBlendDescription : public ezHashableStruct<ezGALRenderTargetBlendDescription>
{
  ezGALBlend::Enum m_SourceBlend = ezGALBlend::One;
  ezGALBlend::Enum m_DestBlend = ezGALBlend::One;
  ezGALBlendOp::Enum m_BlendOp = ezGALBlendOp::Add;

  ezGALBlend::Enum m_SourceBlendAlpha = ezGALBlend::One;
  ezGALBlend::Enum m_DestBlendAlpha = ezGALBlend::One;
  ezGALBlendOp::Enum m_BlendOpAlpha = ezGALBlendOp::Add;

  ezUInt8 m_uiWriteMask = 0xFF; ///< Enables writes to color channels. Bit1 = Red Channel, Bit2 = Green Channel, Bit3 = Blue Channel, Bit4 = Alpha
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
  ezGALStencilOpDescription();

  ezGALStencilOp::Enum m_FailOp;
  ezGALStencilOp::Enum m_DepthFailOp;
  ezGALStencilOp::Enum m_PassOp;

  ezGALCompareFunc::Enum m_StencilFunc;
};

struct ezGALDepthStencilStateCreationDescription : public ezHashableStruct<ezGALDepthStencilStateCreationDescription>
{
  ezGALDepthStencilStateCreationDescription();

  ezGALStencilOpDescription m_FrontFaceStencilOp;
  ezGALStencilOpDescription m_BackFaceStencilOp;

  ezGALCompareFunc::Enum m_DepthTestFunc;

  bool m_bSeparateFrontAndBack; ///< If false, DX11 will use front face values for both front & back face values, GL will not call
                                ///< gl*Separate() funcs
  bool m_bDepthTest;
  bool m_bDepthWrite;
  bool m_bStencilTest;
  ezUInt8 m_uiStencilReadMask;
  ezUInt8 m_uiStencilWriteMask;
};

/// \brief Describes the settings for a new rasterizer state. See ezGALDevice::CreateRasterizerState
struct ezGALRasterizerStateCreationDescription : public ezHashableStruct<ezGALRasterizerStateCreationDescription>
{
  ezGALRasterizerStateCreationDescription();

  ezGALCullMode::Enum m_CullMode; ///< Which sides of a triangle to cull. Default is ezGALCullMode::Back
  ezInt32 m_iDepthBias;           ///< The pixel depth bias. Default is 0
  float m_fDepthBiasClamp;        ///< The pixel depth bias clamp. Default is 0
  float m_fSlopeScaledDepthBias;  ///< The pixel slope scaled depth bias clamp. Default is 0
  bool m_bWireFrame;              ///< Whether triangles are rendered filled or as wireframe. Default is false
  bool m_bFrontCounterClockwise;  ///< Sets which triangle winding order defines the 'front' of a triangle. If true, the front of a triangle
                                  ///< is the one where the vertices appear in counter clockwise order. Default is false
  bool m_bScissorTest;
};

struct ezGALSamplerStateCreationDescription : public ezHashableStruct<ezGALSamplerStateCreationDescription>
{
  ezEnum<ezGALTextureFilterMode> m_MinFilter;
  ezEnum<ezGALTextureFilterMode> m_MagFilter;
  ezEnum<ezGALTextureFilterMode> m_MipFilter;

  ezEnum<ezImageAddressMode> m_AddressU;
  ezEnum<ezImageAddressMode> m_AddressV;
  ezEnum<ezImageAddressMode> m_AddressW;

  ezGALCompareFunc::Enum m_SampleCompareFunc = ezGALCompareFunc::Never;

  ezColor m_BorderColor = ezColor::Black;

  float m_fMipLodBias = 0.0f;
  float m_fMinMip = -1.0f;
  float m_fMaxMip = 42000.0f;

  ezUInt32 m_uiMaxAnisotropy = 4;
};

struct ezGALVertexAttributeSemantic
{
  enum Enum
  {
    Position,
    Normal,
    Tangent,
    Color,
    TexCoord0,
    TexCoord1,
    TexCoord2,
    TexCoord3,
    TexCoord4,
    TexCoord5,
    TexCoord6,
    TexCoord7,
    TexCoord8,
    TexCoord9,

    BiTangent,
    BoneIndices0,
    BoneIndices1,
    BoneWeights0,
    BoneWeights1,

    ENUM_COUNT
  };
};

struct ezGALVertexAttribute
{
  ezGALVertexAttribute();

  ezGALVertexAttribute(ezGALVertexAttributeSemantic::Enum eSemantic, ezGALResourceFormat::Enum eFormat, ezUInt16 uiOffset,
    ezUInt8 uiVertexBufferSlot, bool bInstanceData);

  ezGALVertexAttributeSemantic::Enum m_eSemantic = ezGALVertexAttributeSemantic::Position;
  ezGALResourceFormat::Enum m_eFormat = ezGALResourceFormat::XYZFloat;
  ezUInt16 m_uiOffset = 0;
  ezUInt8 m_uiVertexBufferSlot = 0;
  bool m_bInstanceData = false;
};

struct EZ_RENDERERFOUNDATION_DLL ezGALVertexDeclarationCreationDescription
  : public ezHashableStruct<ezGALVertexDeclarationCreationDescription>
{
  ezGALShaderHandle m_hShader;
  ezStaticArray<ezGALVertexAttribute, 16> m_VertexAttributes;
};

struct ezGALResourceAccess
{
  ezGALResourceAccess();

  bool IsImmutable() const;

  bool m_bReadBack;
  bool m_bImmutable;
};

struct ezGALBufferType
{
  enum Enum
  {
    Generic = 0,
    VertexBuffer,
    IndexBuffer,
    ConstantBuffer,

    ENUM_COUNT
  };
};

struct ezGALBufferCreationDescription : public ezHashableStruct<ezGALBufferCreationDescription>
{
  ezGALBufferCreationDescription();

  ezUInt32 m_uiStructSize;
  ezUInt32 m_uiTotalSize;

  ezGALBufferType::Enum m_BufferType;

  bool m_bUseForIndirectArguments;
  bool m_bUseAsStructuredBuffer;
  bool m_bAllowRawViews;
  bool m_bStreamOutputTarget;
  bool m_bAllowShaderResourceView;
  bool m_bAllowUAV;

  ezGALResourceAccess m_ResourceAccess;
};

struct ezGALTextureCreationDescription : public ezHashableStruct<ezGALTextureCreationDescription>
{
  ezGALTextureCreationDescription();

  void SetAsRenderTarget(ezUInt32 uiWidth, ezUInt32 uiHeight, ezGALResourceFormat::Enum format,
    ezGALMSAASampleCount::Enum sampleCount = ezGALMSAASampleCount::None);

  ezUInt32 m_uiWidth;
  ezUInt32 m_uiHeight;
  ezUInt32 m_uiDepth;

  ezUInt32 m_uiMipLevelCount;

  ezUInt32 m_uiArraySize;

  ezGALMSAASampleCount::Enum m_SampleCount;

  ezGALResourceFormat::Enum m_Format;

  ezGALTextureType::Enum m_Type;

  bool m_bAllowShaderResourceView;
  bool m_bAllowUAV;
  bool m_bCreateRenderTarget;
  bool m_bAllowDynamicMipGeneration;

  ezGALResourceAccess m_ResourceAccess;

  void* m_pExisitingNativeObject; ///< Can be used to encapsulate existing native textures in objects usable by the GAL
};

struct ezGALResourceViewCreationDescription : public ezHashableStruct<ezGALResourceViewCreationDescription>
{
  ezGALResourceViewCreationDescription();

  ezGALTextureHandle m_hTexture;

  ezGALBufferHandle m_hBuffer;

  ezGALResourceFormat::Enum m_OverrideViewFormat;

  ezUInt32 m_uiMostDetailedMipLevel;
  ezUInt32 m_uiMipLevelsToUse;

  ezUInt32 m_uiFirstArraySlice; // For cubemap array: index of first 2d slice to start with
  ezUInt32 m_uiArraySize;       // For cubemap array: number of cubemaps

  // Buffer only
  ezUInt32 m_uiFirstElement;
  ezUInt32 m_uiNumElements;
  bool m_bRawView;
};

struct ezGALRenderTargetType
{
  enum Enum
  {
    Color = 0,
    DepthStencil
  };
};

struct ezGALRenderTargetViewCreationDescription : public ezHashableStruct<ezGALRenderTargetViewCreationDescription>
{
  ezGALRenderTargetViewCreationDescription();

  ezGALTextureHandle m_hTexture;

  ezGALResourceFormat::Enum m_OverrideViewFormat;

  ezUInt32 m_uiMipLevel;

  ezUInt32 m_uiFirstSlice;
  ezUInt32 m_uiSliceCount;

  bool
    m_bReadOnly; ///< Can be used for depth stencil views to create read only views (e.g. for soft particles using the native depth buffer)
};

struct ezGALUnorderedAccessViewCreationDescription : public ezHashableStruct<ezGALUnorderedAccessViewCreationDescription>
{
  ezGALUnorderedAccessViewCreationDescription();

  ezGALTextureHandle m_hTexture;

  ezGALBufferHandle m_hBuffer;

  ezGALResourceFormat::Enum m_OverrideViewFormat;

  // Texture only
  ezUInt32 m_uiMipLevelToUse;   ///< Which MipLevel is accessed with this UAV
  ezUInt32 m_uiFirstArraySlice; ///< First depth slice for 3D Textures.
  ezUInt32 m_uiArraySize;       ///< Number of depth slices for 3D textures.

  // Buffer only
  ezUInt32 m_uiFirstElement;
  ezUInt32 m_uiNumElements;
  bool m_bRawView;
  bool m_bAppend; // Allows appending data to the end of the buffer.
};

struct ezGALQueryType
{
  enum Enum
  {
    /// Number of samples that passed the depth and stencil test between begin and end (on a context).
    NumSamplesPassed,
    /// Boolean version of NumSamplesPassed.
    AnySamplesPassed,

    // Note:
    // GALFence provides an implementation of "event queries".
  };
};

struct ezGALQueryCreationDescription : public ezHashableStruct<ezGALQueryCreationDescription>
{
  ezGALQueryCreationDescription();

  ezGALQueryType::Enum m_type;

  /// In case this query is used for occlusion culling (type AnySamplesPassed), this determines whether drawing should be done if the query
  /// status is still unknown.
  bool m_bDrawIfUnknown;
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
    // could add resource creation/destruction events, if this would be useful
  };

  Type m_Type;
  class ezGALDevice* m_pDevice;
};

#include <RendererFoundation/Descriptors/Implementation/Descriptors_inl.h>
