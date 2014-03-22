
#pragma once

#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Basics/Types/RefCounted.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Containers/HybridArray.h>
#include <RendererFoundation/Resources/ResourceFormats.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>



class ezWindow;

struct ezGALSwapChainCreationDescription : public ezHashableStruct<ezGALSwapChainCreationDescription>
{
  inline ezGALSwapChainCreationDescription();

  ezWindow* m_pWindow;

  ezGALMSAASampleCount::Enum m_SampleCount;
  ezGALResourceFormat::Enum m_BackBufferFormat;

  bool m_bDoubleBuffered;
  bool m_bFullscreen;
  bool m_bAllowScreenshots;
};

struct ezGALDeviceCreationDescription
{
  inline ezGALDeviceCreationDescription();

  ezGALSwapChainCreationDescription m_PrimarySwapChainDescription;
  bool m_bDebugDevice;
  bool m_bCreatePrimarySwapChain;
};

struct ezGALShaderCreationDescription : public ezHashableStruct<ezGALShaderCreationDescription>
{
  inline ezGALShaderCreationDescription();

  inline bool HasByteCodeForStage(ezGALShaderStage::Enum Stage) const;

  ezScopedRefPointer<ezGALShaderByteCode> m_ByteCodes[ezGALShaderStage::ENUM_COUNT];
};

struct ezGALRenderTargetBlendDescription : public ezHashableStruct<ezGALRenderTargetBlendDescription>
{
  inline ezGALRenderTargetBlendDescription();

  ezGALBlend::Enum m_SourceBlend;
  ezGALBlend::Enum m_DestBlend;
  ezGALBlendOp::Enum m_BlendOp;

  ezGALBlend::Enum m_SourceBlendAlpha;
  ezGALBlend::Enum m_DestBlendAlpha;
  ezGALBlendOp::Enum m_BlendOpAlpha;

  ezUInt8 m_uiWriteMask;
  bool m_bBlendingEnabled;
};

struct ezGALBlendStateCreationDescription : public ezHashableStruct<ezGALBlendStateCreationDescription>
{
  inline ezGALBlendStateCreationDescription();

  ezGALRenderTargetBlendDescription m_RenderTargetBlendDescriptions[EZ_GAL_MAX_RENDERTARGET_COUNT];

  bool m_bAlphaToCoverage;
  bool m_bIndependentBlend;
};

struct ezGALStencilOpDescription : public ezHashableStruct<ezGALStencilOpDescription>
{
  inline ezGALStencilOpDescription();

  ezGALStencilOp::Enum m_FailOp;
  ezGALStencilOp::Enum m_DepthFailOp;
  ezGALStencilOp::Enum m_PassOp;

  ezGALCompareFunc::Enum m_StencilFunc;
};

struct ezGALDepthStencilStateCreationDescription : public ezHashableStruct<ezGALDepthStencilStateCreationDescription>
{
  inline ezGALDepthStencilStateCreationDescription();

  ezGALStencilOpDescription m_FrontFaceStencilOp;
  ezGALStencilOpDescription m_BackFaceStencilOp;

  ezGALCompareFunc::Enum m_DepthTestFunc;

  bool m_bSeparateFrontAndBack; ///< If false, DX11 will use front face values for both front & back face values, GL will not call gl*Separate() funcs
  bool m_bDepthTest;
  bool m_bDepthWrite;
  bool m_bStencilTest;
  ezUInt8 m_uiStencilReadMask;
  ezUInt8 m_uiStencilWriteMask;
};

struct ezGALRasterizerStateCreationDescription : public ezHashableStruct<ezGALRasterizerStateCreationDescription>
{
  inline ezGALRasterizerStateCreationDescription();

  ezGALCullMode::Enum m_CullMode;
  ezInt32 m_iDepthBias;
  float m_fDepthBiasClamp;
  float m_fSlopeScaledDepthBias;
  bool m_bWireFrame;
  bool m_bFrontCounterClockwise;
  bool m_bDepthClip;
  bool m_bScissorTest;
  bool m_bMSAA;
  bool m_bLineAA;
};

struct ezGALSamplerStateCreationDescription : public ezHashableStruct<ezGALSamplerStateCreationDescription>
{
  inline ezGALSamplerStateCreationDescription();

  ezGALTextureFilterMode::Enum m_MinFilter;
  ezGALTextureFilterMode::Enum m_MagFilter;
  ezGALTextureFilterMode::Enum m_MipFilter;

  ezGALTextureAddressMode::Enum m_AddressU;
  ezGALTextureAddressMode::Enum m_AddressV;
  ezGALTextureAddressMode::Enum m_AddressW;

  ezGALCompareFunc::Enum m_SampleCompareFunc;

  ezColor m_BorderColor;

  float m_fMipLodBias;
  float m_fMinMip;
  float m_fMaxMip;

  ezUInt32 m_uiMaxAnisotropy;
};

struct ezGALRenderTargetConfigCreationDescription : public ezHashableStruct<ezGALRenderTargetConfigCreationDescription>
{
  ezGALRenderTargetViewHandle m_hColorTargets[EZ_GAL_MAX_RENDERTARGET_COUNT];
  ezGALRenderTargetViewHandle m_hDepthStencilTarget;

  ezUInt32 m_uiColorTargetCount;

  inline bool IsValid() const
  {
    return m_uiColorTargetCount > 0 || !m_hDepthStencilTarget.IsInvalidated();
  }
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

    ENUM_COUNT
  };
};

struct ezGALVertexAttribute
{
  EZ_FORCE_INLINE ezGALVertexAttribute();

  EZ_FORCE_INLINE ezGALVertexAttribute(ezGALVertexAttributeSemantic::Enum eSemantic, ezGALResourceFormat::Enum eFormat, ezUInt16 uiOffset, ezUInt8 uiVertexBufferSlot, bool bInstanceData);

  ezGALVertexAttributeSemantic::Enum m_eSemantic;
  ezGALResourceFormat::Enum m_eFormat;
  ezUInt16 m_uiOffset;
  ezUInt8 m_uiVertexBufferSlot;
  bool m_bInstanceData;
};

struct ezGALVertexDeclarationCreationDescription : public ezHashableStruct<ezGALVertexDeclarationCreationDescription>
{
  ezGALShaderHandle m_hShader;
  ezHybridArray<ezGALVertexAttribute, 8> m_VertexAttributes;
};

struct ezGALResourceAccess
{
  inline ezGALResourceAccess();

  inline bool IsImmutable() const;


  bool m_bReadBack;
  bool m_bImmutable;
};

struct ezGALBufferType
{
  enum Enum
  {
    Storage = 0,
    VertexBuffer,
    IndexBuffer,
    ConstantBuffer,

    ENUM_COUNT
  };
};

struct ezGALBufferCreationDescription : public ezHashableStruct<ezGALBufferCreationDescription>
{
  inline ezGALBufferCreationDescription();

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
  inline ezGALTextureCreationDescription();

  ezUInt32 m_uiWidth;
  ezUInt32 m_uiHeight;
  ezUInt32 m_uiDepth;

  ezUInt32 m_uiMipSliceCount;

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
  inline ezGALResourceViewCreationDescription();

  ezGALTextureHandle m_hTexture;

  ezGALBufferHandle m_hBuffer;

  ezGALResourceFormat::Enum m_OverrideViewFormat;

  ezUInt32 m_uiMostDetailedMipLevel;
  ezUInt32 m_uiMipLevelsToUse;

  ezUInt32 m_uiFirstArraySlice; // For cubemap array: index of first 2d slice to start with
  ezUInt32 m_uiArraySize; // For cubemap array: number of cubemaps

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
  inline ezGALRenderTargetViewCreationDescription();

  ezGALTextureHandle m_hTexture;

  ezGALBufferHandle m_hBuffer;

  ezGALRenderTargetType::Enum m_RenderTargetType;

  ezGALResourceFormat::Enum m_OverrideViewFormat;

  ezUInt32 m_uiMipSlice;

  ezUInt32 m_uiFirstSlice;
  ezUInt32 m_uiSliceCount;

  bool m_bReadOnly; ///< Can be used for depth stencil views to create read only views (e.g. for soft particles using the native depth buffer)
};


#include <RendererFoundation/Descriptors/Implementation/Descriptors_inl.h>