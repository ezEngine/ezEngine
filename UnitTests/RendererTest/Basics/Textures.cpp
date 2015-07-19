#include <PCH.h>
#include "Basics.h"
#include <RendererFoundation/Context/Context.h>
#include <CoreUtils/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Shader/ShaderResource.h>

ezTestAppRun ezRendererTestBasics::SubtestTextures2D()
{
  BeginFrame();

  bool bFinish = true;

  const ezInt32 iNumFrames = 10;

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Textured.ezShader");

  if (m_iFrame == 0)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/ezLogo_ABGR_Mips_D.dds");
  }

  if (m_iFrame == 1)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/ezLogo_ABGR_NoMips_D.dds");
  }

  if (m_iFrame == 2)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/ezLogo_ARGB_Mips_D.dds");
  }

  if (m_iFrame == 3)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/ezLogo_ARGB_NoMips_D.dds");
  }

  if (m_iFrame == 4)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/ezLogo_DXT1_Mips_D.dds");
  }

  if (m_iFrame == 5)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/ezLogo_DXT1_NoMips_D.dds");
  }

  if (m_iFrame == 6)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/ezLogo_DXT3_Mips_D.dds");
  }

  if (m_iFrame == 7)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/ezLogo_DXT3_NoMips_D.dds");
  }

  if (m_iFrame == 8)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/ezLogo_DXT5_Mips_D.dds");
  }

  if (m_iFrame == 9)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/ezLogo_DXT5_NoMips_D.dds");
  }

  if (m_iFrame == 10)
  {
    /// \todo Christian: This format cannot be read by ezImage: The number of bits per pixel specified in the file (%d) does not match the expected value of %d for the format '%s'
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/ezLogo_RGB_Mips_D.dds");
  }

  if (m_iFrame == 11)
  {
    /// \todo Christian: This format cannot be read by ezImage: The number of bits per pixel specified in the file (%d) does not match the expected value of %d for the format '%s'
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/ezLogo_RGB_NoMips_D.dds");
  }

  if (m_iFrame == 12)
  {
    // My GPU seems not to support this format in general
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/ezLogo_R5G6B5_NoMips_D.dds");
  }

  if (m_iFrame == 13)
  {
    // This format generally does not support mipmaps
    //m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/ezLogo_R5G6B5_MipsD.dds");
  }

  ezRenderContext::GetDefaultInstance()->BindTexture("TexDiffuse", m_hTexture);

  ClearScreen(ezColor::Black);

  RenderObjects(ezShaderBindFlags::Default);

  EZ_TEST_IMAGE(100);

  EndFrame();

  return m_iFrame < (iNumFrames-1) ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}


ezTestAppRun ezRendererTestBasics::SubtestTextures3D()
{
  BeginFrame();

  bool bFinish = true;

  const ezInt32 iNumFrames = 1;

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/TexturedVolume.ezShader");

  if (m_iFrame == 0)
  {
    /// \todo Christian: Error "The image format is neither specified as a pixel mask nor as a FourCC code"
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/Volume/ezLogo_Volume_A8_NoMips_D.dds");
  }

  ezRenderContext::GetDefaultInstance()->BindTexture("TexDiffuse", m_hTexture);

  ClearScreen(ezColor::Black);

  RenderObjects(ezShaderBindFlags::Default);

  EZ_TEST_IMAGE(100);

  EndFrame();

  return m_iFrame < (iNumFrames-1) ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}


ezTestAppRun ezRendererTestBasics::SubtestTexturesCube()
{
  BeginFrame();

  bool bFinish = true;

  const ezInt32 iNumFrames = 10;

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/TexturedCube.ezShader");

  if (m_iFrame == 0)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/Cubemap/ezLogo_Cube_XRGB_NoMips_D.dds");
  }

  if (m_iFrame == 1)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/Cubemap/ezLogo_Cube_XRGB_Mips_D.dds");
  }

  if (m_iFrame == 2)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/Cubemap/ezLogo_Cube_RGBA_NoMips_D.dds");
  }

  if (m_iFrame == 3)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/Cubemap/ezLogo_Cube_RGBA_Mips_D.dds");
  }

  if (m_iFrame == 4)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/Cubemap/ezLogo_Cube_DXT1_NoMips_D.dds");
  }

  if (m_iFrame == 5)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/Cubemap/ezLogo_Cube_DXT1_Mips_D.dds");
  }

  if (m_iFrame == 6)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/Cubemap/ezLogo_Cube_DXT3_NoMips_D.dds");
  }

  if (m_iFrame == 7)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/Cubemap/ezLogo_Cube_DXT3_Mips_D.dds");
  }

  if (m_iFrame == 8)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/Cubemap/ezLogo_Cube_DXT5_NoMips_D.dds");
  }

  if (m_iFrame == 9)
  {
    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/Cubemap/ezLogo_Cube_DXT5_Mips_D.dds");
  }

  if (m_iFrame == 10)
  {
    /// \todo Christian: These formats cannot be loaded

    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/Cubemap/ezLogo_Cube_RGB_NoMips_D.dds");
  }

  if (m_iFrame == 11)
  {
    /// \todo Christian: These formats cannot be loaded

    m_hTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/Cubemap/ezLogo_Cube_RGB_Mips_D.dds");
  }

  ezRenderContext::GetDefaultInstance()->BindTexture("TexDiffuse", m_hTexture);

  ClearScreen(ezColor::Black);

  RenderObjects(ezShaderBindFlags::Default);

  EZ_TEST_IMAGE(100);

  EndFrame();

  return m_iFrame < (iNumFrames-1) ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}

