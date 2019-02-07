#include <RendererFoundationPCH.h>

#include <RendererFoundation/Device/DeviceCapabilities.h>
#include <RendererFoundation/Shader/Shader.h>

ezGALDeviceCapabilities::ezGALDeviceCapabilities()
{
  // General capabilities
  m_bMultithreadedResourceCreation = false;
  m_bNoOverwriteBufferUpdate = false;

  // Draw related capabilities
  for (int i = 0; i < ezGALShaderStage::ENUM_COUNT; ++i)
  {
    m_bShaderStageSupported[i] = false;
  }

  m_bInstancing = false;
  m_b32BitIndices = false;
  m_bIndirectDraw = false;
  m_bStreamOut = false;
  m_uiMaxConstantBuffers = 0;


  // Texture related capabilities
  m_bTextureArrays = false;
  m_bCubemapArrays = false;
  m_bB5G6R5Textures = false;
  m_uiMaxTextureDimension = 0;
  m_uiMaxCubemapDimension = 0;
  m_uiMax3DTextureDimension = 0;
  m_uiMaxAnisotropy = 0;


  // Output related capabilities
  m_uiMaxRendertargets = 0;
  m_uiUAVCount = 0;
  m_bAlphaToCoverage = false;
}



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_DeviceCapabilities);

