
#pragma once

/// \brief This struct holds information about the rendering device capabilities (e.g. what shader stages are supported and more)
/// To get the device capabilities you need to call the GetCapabilities() function on an ezGALDevice object.
struct EZ_RENDERERFOUNDATION_DLL ezGALDeviceCapabilities
{
  ezGALDeviceCapabilities();


  // Draw related capabilities
  bool m_bShaderStageSupported[ezGALShaderStage::ENUM_COUNT];
  bool m_bInstancing;
  bool m_b32BitIndices;
  bool m_bIndirectDraw;
  bool m_bStreamOut;
  ezUInt16 m_uiMaxConstantBuffers;
  

  // Texture related capabilities
  bool m_bTextureArrays;
  bool m_bCubemapArrays;
  bool m_bB5G6R5Textures;
  ezUInt16 m_uiMaxTextureDimension;
  ezUInt16 m_uiMaxCubemapDimension;
  ezUInt16 m_uiMax3DTextureDimension;
  ezUInt16 m_uiMaxAnisotropy;


  // Output related capabilities
  ezUInt16 m_uiMaxRendertargets;
  ezUInt16 m_uiUAVCount;
  bool m_bAlphaToCoverage;
};