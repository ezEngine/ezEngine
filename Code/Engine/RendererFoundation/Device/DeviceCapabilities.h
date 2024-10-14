#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Types/Bitflags.h>

/// \brief Defines which operations can be performed on an ezGALResourceFormat
/// \sa ezGALDeviceCapabilities::m_FormatSupport
struct ezGALResourceFormatSupport
{
  using StorageType = ezUInt8;

  enum Enum
  {
    None = 0,
    Texture = EZ_BIT(0),         ///< Can be used as a texture and bound to a ezGALShaderResourceType::Texture slot.
    RenderTarget = EZ_BIT(1),    ///< Can be used as a texture and bound as a render target.
    TextureRW = EZ_BIT(2),       ///< Can be used as a texture and bound to a ezGALShaderResourceType::TextureRW slot.
    MSAA2x = EZ_BIT(3),          ///< The format supports 2x MSAA
    MSAA4x = EZ_BIT(4),          ///< The format supports 4x MSAA
    MSAA8x = EZ_BIT(5),          ///< The format supports 8x MSAA
    VertexAttribute = EZ_BIT(6), ///< The format can be used as a vertex attribute.
    Default = 0
  };

  struct Bits
  {
    StorageType Texture : 1;
    StorageType RenderTarget : 1;
    StorageType TextureRW : 1;
    StorageType MSAA2x : 1;
    StorageType MSAA4x : 1;
    StorageType MSAA8x : 1;
    StorageType VertexAttribute : 1;
  };
};
EZ_DECLARE_FLAGS_OPERATORS(ezGALResourceFormatSupport);

/// \brief This struct holds information about the rendering device capabilities (e.g. what shader stages are supported and more)
/// To get the device capabilities you need to call the GetCapabilities() function on an ezGALDevice object.
struct EZ_RENDERERFOUNDATION_DLL ezGALDeviceCapabilities
{
  // Device description
  ezString m_sAdapterName = "Unknown";
  ezUInt64 m_uiDedicatedVRAM = 0;
  ezUInt64 m_uiDedicatedSystemRAM = 0;
  ezUInt64 m_uiSharedSystemRAM = 0;
  bool m_bHardwareAccelerated = false;

  // General capabilities
  bool m_bMultithreadedResourceCreation = false; ///< whether creating resources is allowed on other threads than the main thread
  bool m_bNoOverwriteBufferUpdate = false;

  // Draw related capabilities
  bool m_bShaderStageSupported[ezGALShaderStage::ENUM_COUNT] = {};
  bool m_bInstancing = false;
  bool m_b32BitIndices = false;
  bool m_bIndirectDraw = false;
  bool m_bConservativeRasterization = false;
  bool m_bVertexShaderRenderTargetArrayIndex = false;
  bool m_bTexelBuffer = true; ///< Whether ezGALBufferUsageFlags::TexelBuffer is supported. Hardcoded per platform as it must match SUPPORTS_TEXEL_BUFFER shader define.
  ezUInt16 m_uiMaxConstantBuffers = 0;
  ezUInt16 m_uiMaxPushConstantsSize = 0;


  // Texture related capabilities
  bool m_bTextureArrays = false;
  bool m_bCubemapArrays = false;
  bool m_bSharedTextures = false;
  ezUInt16 m_uiMaxTextureDimension = 0;
  ezUInt16 m_uiMaxCubemapDimension = 0;
  ezUInt16 m_uiMax3DTextureDimension = 0;
  ezUInt16 m_uiMaxAnisotropy = 0;
  ezDynamicArray<ezBitflags<ezGALResourceFormatSupport>> m_FormatSupport;

  // Output related capabilities
  ezUInt16 m_uiMaxRendertargets = 0;
  ezUInt16 m_uiUAVCount = 0;
  bool m_bAlphaToCoverage = false;
};
