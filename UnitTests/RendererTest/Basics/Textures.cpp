#include <PCH.h>

#include "Basics.h"
#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/Context/Context.h>

ezTestAppRun ezRendererTestBasics::SubtestTextures2D()
{
  BeginFrame();

  ezRenderContext::GetDefaultInstance()->SetDefaultTextureFilter(ezTextureFilterSetting::FixedTrilinear);

  const ezInt32 iNumFrames = 14;

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Textured.ezShader");

  if (m_iFrame == 0)
  {
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>("SharedData/Textures/ezLogo_ABGR_Mips_D.dds");
  }

  if (m_iFrame == 1)
  {
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>("SharedData/Textures/ezLogo_ABGR_NoMips_D.dds");
  }

  if (m_iFrame == 2)
  {
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>("SharedData/Textures/ezLogo_ARGB_Mips_D.dds");
  }

  if (m_iFrame == 3)
  {
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>("SharedData/Textures/ezLogo_ARGB_NoMips_D.dds");
  }

  if (m_iFrame == 4)
  {
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>("SharedData/Textures/ezLogo_DXT1_Mips_D.dds");
  }

  if (m_iFrame == 5)
  {
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>("SharedData/Textures/ezLogo_DXT1_NoMips_D.dds");
  }

  if (m_iFrame == 6)
  {
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>("SharedData/Textures/ezLogo_DXT3_Mips_D.dds");
  }

  if (m_iFrame == 7)
  {
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>("SharedData/Textures/ezLogo_DXT3_NoMips_D.dds");
  }

  if (m_iFrame == 8)
  {
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>("SharedData/Textures/ezLogo_DXT5_Mips_D.dds");
  }

  if (m_iFrame == 9)
  {
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>("SharedData/Textures/ezLogo_DXT5_NoMips_D.dds");
  }

  if (m_iFrame == 10)
  {
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>("SharedData/Textures/ezLogo_RGB_Mips_D.dds");
  }

  if (m_iFrame == 11)
  {
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>("SharedData/Textures/ezLogo_RGB_NoMips_D.dds");
  }

  if (m_iFrame == 12)
  {
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>("SharedData/Textures/ezLogo_R5G6B5_NoMips_D.dds");
  }

  if (m_iFrame == 13)
  {
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>("SharedData/Textures/ezLogo_R5G6B5_MipsD.dds");
  }

  ezRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", m_hTexture2D);

  ClearScreen(ezColor::Black);

  RenderObjects(ezShaderBindFlags::Default);

  EZ_TEST_IMAGE(m_iFrame, 100);

  EndFrame();

  return m_iFrame < (iNumFrames - 1) ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}


ezTestAppRun ezRendererTestBasics::SubtestTextures3D()
{
  BeginFrame();

  ezRenderContext::GetDefaultInstance()->SetDefaultTextureFilter(ezTextureFilterSetting::FixedTrilinear);

  const ezInt32 iNumFrames = 1;

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/TexturedVolume.ezShader");

  if (m_iFrame == 0)
  {
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>("SharedData/Textures/Volume/ezLogo_Volume_A8_NoMips_D.dds");
  }

  ezRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", m_hTexture2D);

  ClearScreen(ezColor::Black);

  RenderObjects(ezShaderBindFlags::Default);

  EZ_TEST_IMAGE(m_iFrame, 100);

  EndFrame();

  return m_iFrame < (iNumFrames - 1) ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}


ezTestAppRun ezRendererTestBasics::SubtestTexturesCube()
{
  BeginFrame();

  ezRenderContext::GetDefaultInstance()->SetDefaultTextureFilter(ezTextureFilterSetting::FixedTrilinear);

  const ezInt32 iNumFrames = 12;

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/TexturedCube.ezShader");

  if (m_iFrame == 0)
  {
    m_hTextureCube = ezResourceManager::LoadResource<ezTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_XRGB_NoMips_D.dds");
  }

  if (m_iFrame == 1)
  {
    m_hTextureCube = ezResourceManager::LoadResource<ezTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_XRGB_Mips_D.dds");
  }

  if (m_iFrame == 2)
  {
    m_hTextureCube = ezResourceManager::LoadResource<ezTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_RGBA_NoMips_D.dds");
  }

  if (m_iFrame == 3)
  {
    m_hTextureCube = ezResourceManager::LoadResource<ezTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_RGBA_Mips_D.dds");
  }

  if (m_iFrame == 4)
  {
    m_hTextureCube = ezResourceManager::LoadResource<ezTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_DXT1_NoMips_D.dds");
  }

  if (m_iFrame == 5)
  {
    m_hTextureCube = ezResourceManager::LoadResource<ezTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_DXT1_Mips_D.dds");
  }

  if (m_iFrame == 6)
  {
    m_hTextureCube = ezResourceManager::LoadResource<ezTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_DXT3_NoMips_D.dds");
  }

  if (m_iFrame == 7)
  {
    m_hTextureCube = ezResourceManager::LoadResource<ezTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_DXT3_Mips_D.dds");
  }

  if (m_iFrame == 8)
  {
    m_hTextureCube = ezResourceManager::LoadResource<ezTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_DXT5_NoMips_D.dds");
  }

  if (m_iFrame == 9)
  {
    m_hTextureCube = ezResourceManager::LoadResource<ezTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_DXT5_Mips_D.dds");
  }

  if (m_iFrame == 10)
  {
    m_hTextureCube = ezResourceManager::LoadResource<ezTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_RGB_NoMips_D.dds");
  }

  if (m_iFrame == 11)
  {
    m_hTextureCube = ezResourceManager::LoadResource<ezTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_RGB_Mips_D.dds");
  }

  ezRenderContext::GetDefaultInstance()->BindTextureCube("DiffuseTexture", m_hTextureCube);

  ClearScreen(ezColor::Black);

  RenderObjects(ezShaderBindFlags::Default);

  EZ_TEST_IMAGE(m_iFrame, 100);

  EndFrame();

  return m_iFrame < (iNumFrames - 1) ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}
