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

struct ezGALBufferLayout
{
  using StorageType = ezUInt8;
  enum Enum
  {
    Vulkan_Std140_relaxed, // Vulkan uniform buffer
    Vulkan_Std430_relaxed, // Vulkan structured buffer
    DirectX_ConstantButter,
    DirectX_StructuredButter,
    Default = DirectX_ConstantButter
  };
};

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
  bool m_bSupportsMultithreadedResourceCreation = false; ///< whether creating resources is allowed on other threads than the main thread
  bool m_bSupportsNoOverwriteBufferUpdate = false;
  ezEnum<ezGALBufferLayout> m_materialBufferLayout;

  // Draw related capabilities
  bool m_bShaderStageSupported[ezGALShaderStage::ENUM_COUNT] = {};
  bool m_bSupportsIndirectDraw = false;
  bool m_bSupportsConservativeRasterization = false;
  bool m_bSupportsVSRenderTargetArrayIndex = false;
  bool m_bSupportsTexelBuffer = false; ///< Whether ezGALBufferUsageFlags::TexelBuffer is supported. Hardcoded per platform as it must match SUPPORTS_TEXEL_BUFFER shader define.
  bool m_bSupportsMultipleSRVTypes = true; ///< Whether more than one of ezGALBufferUsageFlags::StructuredBuffer, ezGALBufferUsageFlags::TexelBuffer and ezGALBufferUsageFlags::ByteAddressBuffer is supported on a buffer.
  bool m_bSupportsMultiSampledArrays = false;
  ezUInt16 m_uiMaxPushConstantsSize = 0;


  // Texture related capabilities
  bool m_bSupportsSharedTextures = false;
  ezDynamicArray<ezBitflags<ezGALResourceFormatSupport>> m_FormatSupport;
};
