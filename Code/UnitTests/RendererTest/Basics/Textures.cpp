#include <RendererTest/RendererTestPCH.h>

#include "Basics.h"
#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>

ezTestAppRun ezRendererTestBasics::SubtestTextures2D()
{
  BeginFrame();
  BeginCommands("Textures2D");

  ezRenderContext::GetDefaultInstance()->SetDefaultTextureFilter(ezTextureFilterSetting::FixedTrilinear);

  const ezInt32 iNumFrames = 14;

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Textured.ezShader");
  ezEnum<ezGALResourceFormat> textureFormat;
  ezStringView sTextureResourceId;

  if (m_iFrame == 0)
  {
    textureFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/ezLogo_ABGR_Mips_D.dds";
  }

  if (m_iFrame == 1)
  {
    textureFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/ezLogo_ABGR_NoMips_D.dds";
  }

  if (m_iFrame == 2)
  {
    textureFormat = ezGALResourceFormat::BGRAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/ezLogo_ARGB_Mips_D.dds";
  }

  if (m_iFrame == 3)
  {
    textureFormat = ezGALResourceFormat::BGRAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/ezLogo_ARGB_NoMips_D.dds";
  }

  if (m_iFrame == 4)
  {
    textureFormat = ezGALResourceFormat::BC1sRGB;
    sTextureResourceId = "SharedData/Textures/ezLogo_DXT1_Mips_D.dds";
  }

  if (m_iFrame == 5)
  {
    textureFormat = ezGALResourceFormat::BC1sRGB;
    sTextureResourceId = "SharedData/Textures/ezLogo_DXT1_NoMips_D.dds";
  }

  if (m_iFrame == 6)
  {
    textureFormat = ezGALResourceFormat::BC2sRGB;
    sTextureResourceId = "SharedData/Textures/ezLogo_DXT3_Mips_D.dds";
  }

  if (m_iFrame == 7)
  {
    textureFormat = ezGALResourceFormat::BC2sRGB;
    sTextureResourceId = "SharedData/Textures/ezLogo_DXT3_NoMips_D.dds";
  }

  if (m_iFrame == 8)
  {
    textureFormat = ezGALResourceFormat::BC3sRGB;
    sTextureResourceId = "SharedData/Textures/ezLogo_DXT5_Mips_D.dds";
  }

  if (m_iFrame == 9)
  {
    textureFormat = ezGALResourceFormat::BC3sRGB;
    sTextureResourceId = "SharedData/Textures/ezLogo_DXT5_NoMips_D.dds";
  }

  if (m_iFrame == 10)
  {
    textureFormat = ezGALResourceFormat::BGRAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/ezLogo_RGB_Mips_D.dds";
  }

  if (m_iFrame == 11)
  {
    textureFormat = ezGALResourceFormat::BGRAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/ezLogo_RGB_NoMips_D.dds";
  }

  if (m_iFrame == 12)
  {
    textureFormat = ezGALResourceFormat::B5G6R5UNormalized;
    sTextureResourceId = "SharedData/Textures/ezLogo_R5G6B5_NoMips_D.dds";
  }

  if (m_iFrame == 13)
  {
    textureFormat = ezGALResourceFormat::B5G6R5UNormalized;
    sTextureResourceId = "SharedData/Textures/ezLogo_R5G6B5_MipsD.dds";
  }

  const bool bSupported = m_pDevice->GetCapabilities().m_FormatSupport[textureFormat].AreAllSet(ezGALResourceFormatSupport::Texture);
  if (bSupported)
  {
    ezBindGroupBuilder& bindGroup = ezRenderContext::GetDefaultInstance()->GetBindGroup();
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>(sTextureResourceId);
    bindGroup.BindTexture("DiffuseTexture", m_hTexture2D);
  }
  BeginRendering(ezColor::Black);

  if (bSupported)
  {
    RenderObjects(ezShaderBindFlags::Default);
  }

  EndRendering();

  if (bSupported)
  {
    EZ_TEST_IMAGE(m_iFrame, 300);
  }

  EndCommands();
  EndFrame();

  return m_iFrame < (iNumFrames - 1) ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}


ezTestAppRun ezRendererTestBasics::SubtestTextures3D()
{
  BeginFrame();
  BeginCommands("Textures3D");
  ezRenderContext::GetDefaultInstance()->SetDefaultTextureFilter(ezTextureFilterSetting::FixedTrilinear);

  const ezInt32 iNumFrames = 1;

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/TexturedVolume.ezShader");

  if (m_iFrame == 0)
  {
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>("SharedData/Textures/Volume/ezLogo_Volume_A8_NoMips_D.dds");
  }
  ezBindGroupBuilder& bindGroup = ezRenderContext::GetDefaultInstance()->GetBindGroup();
  bindGroup.BindTexture("DiffuseTexture", m_hTexture2D);

  BeginRendering(ezColor::Black);

  RenderObjects(ezShaderBindFlags::Default);

  EZ_TEST_IMAGE(m_iFrame, 100);
  EndRendering();
  EndCommands();
  EndFrame();

  return m_iFrame < (iNumFrames - 1) ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}


ezTestAppRun ezRendererTestBasics::SubtestTexturesCube()
{
  BeginFrame();
  BeginCommands("TexturesCube");

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

  ezBindGroupBuilder& bindGroup = ezRenderContext::GetDefaultInstance()->GetBindGroup();
  bindGroup.BindTexture("DiffuseTexture", m_hTextureCube);

  BeginRendering(ezColor::Black);

  RenderObjects(ezShaderBindFlags::Default);

  EndRendering();
  EZ_TEST_IMAGE(m_iFrame, 200);
  EndCommands();
  EndFrame();

  return m_iFrame < (iNumFrames - 1) ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}
