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
    Sample = EZ_BIT(0),          ///< The format can be sampled as a texture in a shader or in case of integer textures load can be called.
    Render = EZ_BIT(1),          ///< The format can be used as a render target texture.
    VertexAttribute = EZ_BIT(2), ///< The format can be used as a vertex attribute.
    Default = 0
  };

  struct Bits
  {
    StorageType Sample : 1;
    StorageType Render : 1;
    StorageType VertexAttribute : 1;
  };
};
EZ_DECLARE_FLAGS_OPERATORS(ezGALResourceFormatSupport);

/// \brief This struct holds information about the rendering device capabilities (e.g. what shader stages are supported and more)
/// To get the device capabilities you need to call the GetCapabilities() function on an ezGALDevice object.
struct EZ_RENDERERFOUNDATION_DLL ezGALDeviceCapabilities
{
  ezGALDeviceCapabilities();

  // Device description
  ezString m_sAdapterName = "Unknown";
  ezUInt64 m_uiDedicatedVRAM = 0;
  ezUInt64 m_uiDedicatedSystemRAM = 0;
  ezUInt64 m_uiSharedSystemRAM = 0;
  bool m_bHardwareAccelerated = false;

  // General capabilities
  bool m_bMultithreadedResourceCreation; ///< whether creating resources is allowed on other threads than the main thread
  bool m_bNoOverwriteBufferUpdate;

  // Draw related capabilities
  bool m_bShaderStageSupported[ezGALShaderStage::ENUM_COUNT];
  bool m_bInstancing;
  bool m_b32BitIndices;
  bool m_bIndirectDraw;
  bool m_bConservativeRasterization;
  bool m_bVertexShaderRenderTargetArrayIndex = false;
  ezUInt16 m_uiMaxConstantBuffers;


  // Texture related capabilities
  bool m_bTextureArrays;
  bool m_bCubemapArrays;
  ezUInt16 m_uiMaxTextureDimension;
  ezUInt16 m_uiMaxCubemapDimension;
  ezUInt16 m_uiMax3DTextureDimension;
  ezUInt16 m_uiMaxAnisotropy;
  ezDynamicArray<ezBitflags<ezGALResourceFormatSupport>> m_FormatSupport;

  // Output related capabilities
  ezUInt16 m_uiMaxRendertargets;
  ezUInt16 m_uiUAVCount;
  bool m_bAlphaToCoverage;
};
