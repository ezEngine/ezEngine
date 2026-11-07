#include <RendererTest/RendererTestPCH.h>

#include "Basics.h"
#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <Texture/Image/Image.h>

namespace
{
  void CreateBlockCompressedMipChain(ezImage& ref_image, ezUInt32 uiDepth)
  {
    ezImageHeader header;
    header.SetWidth(26);
    header.SetHeight(17);
    header.SetDepth(uiDepth);
    header.SetNumMipLevels(2);
    header.SetImageFormat(ezImageFormat::BC4_UNORM);

    ref_image.ResetAndAlloc(header);
    ezMemoryUtils::ZeroFill(ref_image.GetByteBlobPtr().GetPtr(), ref_image.GetByteBlobPtr().GetCount());
  }

} // namespace

ezTestAppRun ezRendererTestBasics::SubtestTextures2D()
{
  BeginFrame();
  BeginCommands("Textures2D");
  TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);

  ezRenderContext::GetDefaultInstance()->SetDefaultTextureQuality(ezGALTextureQuality::Trilinear);

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
    ezBindGroupBuilder& bindGroupTest = ezRenderContext::GetDefaultInstance()->GetBindGroup();
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>(sTextureResourceId);
    bindGroupTest.BindTexture("DiffuseTexture", m_hTexture2D);
  }
  BeginRendering(ezColor::Black);

  if (bSupported)
  {
    RenderObjects(ezShaderBindFlags::Default);
  }

  EndRendering();

  if (bSupported)
  {
    TransitionTexture(GetBackbuffer(), ezGALResourceState::CopySource);
    EZ_TEST_IMAGE(m_iFrame, 300);
  }

  EndCommands();
  EndFrame();

  return m_iFrame < (iNumFrames - 1) ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}

ezTestAppRun ezRendererTestBasics::SubtestBlockCompressedMipChain2D()
{
  BeginFrame();

  ezImage image;
  CreateBlockCompressedMipChain(image, 1);
  ezTexture2DResourceDescriptor descriptor;
  ezHybridArray<ezGALSystemMemoryDescription, 32> initialData;
  ezUInt32 uiMemoryUsed = 0;
  ezTexture2DResource::FillOutDescriptor(descriptor, &image, false, 2, uiMemoryUsed, initialData);
  EZ_TEST_INT(descriptor.m_DescGAL.m_uiWidth, 24);
  EZ_TEST_INT(descriptor.m_DescGAL.m_uiHeight, 16);
  EZ_TEST_INT(initialData.GetCount(), 2);
  EZ_TEST_INT(initialData[1].m_uiRowPitch, 32);
  m_hBlockCompressedTexture = m_pDevice->CreateTexture(descriptor.m_DescGAL, descriptor.m_InitialContent);
  EZ_TEST_BOOL(!m_hBlockCompressedTexture.IsInvalidated());

  EndFrame();
  return ezTestAppRun::Quit;
}

ezTestAppRun ezRendererTestBasics::SubtestBlockCompressedMipChain3D()
{
  BeginFrame();

  ezImage image;
  CreateBlockCompressedMipChain(image, 32);
  ezTexture3DResourceDescriptor descriptor;
  ezHybridArray<ezGALSystemMemoryDescription, 32> initialData;
  ezUInt32 uiMemoryUsed = 0;
  ezTexture3DResource::FillOutDescriptor(descriptor, &image, false, 2, uiMemoryUsed, initialData);
  EZ_TEST_INT(descriptor.m_DescGAL.m_uiWidth, 24);
  EZ_TEST_INT(descriptor.m_DescGAL.m_uiHeight, 16);
  EZ_TEST_INT(initialData.GetCount(), 2);
  EZ_TEST_INT(initialData[1].m_uiRowPitch, 32);
  m_hBlockCompressedTexture = m_pDevice->CreateTexture(descriptor.m_DescGAL, descriptor.m_InitialContent);
  EZ_TEST_BOOL(!m_hBlockCompressedTexture.IsInvalidated());

  EndFrame();
  return ezTestAppRun::Quit;
}

ezTestAppRun ezRendererTestBasics::SubtestTextures3D()
{
  BeginFrame();
  BeginCommands("Textures3D");
  TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);
  ezRenderContext::GetDefaultInstance()->SetDefaultTextureQuality(ezGALTextureQuality::Trilinear);

  const ezInt32 iNumFrames = 1;

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/TexturedVolume.ezShader");

  if (m_iFrame == 0)
  {
    m_hTexture2D = ezResourceManager::LoadResource<ezTexture2DResource>("SharedData/Textures/Volume/ezLogo_Volume_A8_NoMips_D.dds");
  }
  ezBindGroupBuilder& bindGroupTest = ezRenderContext::GetDefaultInstance()->GetBindGroup();
  bindGroupTest.BindTexture("DiffuseTexture", m_hTexture2D);

  BeginRendering(ezColor::Black);

  RenderObjects(ezShaderBindFlags::Default);

  TransitionTexture(GetBackbuffer(), ezGALResourceState::CopySource);
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
  TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);

  ezRenderContext::GetDefaultInstance()->SetDefaultTextureQuality(ezGALTextureQuality::Trilinear);

  const ezInt32 iNumFrames = 12;

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/TexturedCube.ezShader");
  ezEnum<ezGALResourceFormat> textureFormat;
  ezStringView sTextureResourceId;

  if (m_iFrame == 0)
  {
    textureFormat = ezGALResourceFormat::BGRAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/ezLogo_Cube_XRGB_NoMips_D.dds";
  }

  if (m_iFrame == 1)
  {
    textureFormat = ezGALResourceFormat::BGRAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/ezLogo_Cube_XRGB_Mips_D.dds";
  }

  if (m_iFrame == 2)
  {
    textureFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/ezLogo_Cube_RGBA_NoMips_D.dds";
  }

  if (m_iFrame == 3)
  {
    textureFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/ezLogo_Cube_RGBA_Mips_D.dds";
  }

  if (m_iFrame == 4)
  {
    textureFormat = ezGALResourceFormat::BC1sRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/ezLogo_Cube_DXT1_NoMips_D.dds";
  }

  if (m_iFrame == 5)
  {
    textureFormat = ezGALResourceFormat::BC1sRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/ezLogo_Cube_DXT1_Mips_D.dds";
  }

  if (m_iFrame == 6)
  {
    textureFormat = ezGALResourceFormat::BC2sRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/ezLogo_Cube_DXT3_NoMips_D.dds";
  }

  if (m_iFrame == 7)
  {
    textureFormat = ezGALResourceFormat::BC2sRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/ezLogo_Cube_DXT3_Mips_D.dds";
  }

  if (m_iFrame == 8)
  {
    textureFormat = ezGALResourceFormat::BC3sRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/ezLogo_Cube_DXT5_NoMips_D.dds";
  }

  if (m_iFrame == 9)
  {
    textureFormat = ezGALResourceFormat::BC3sRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/ezLogo_Cube_DXT5_Mips_D.dds";
  }

  if (m_iFrame == 10)
  {
    textureFormat = ezGALResourceFormat::BGRAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/ezLogo_Cube_RGB_NoMips_D.dds";
  }

  if (m_iFrame == 11)
  {
    textureFormat = ezGALResourceFormat::BGRAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/ezLogo_Cube_RGB_Mips_D.dds";
  }

  const bool bSupported = m_pDevice->GetCapabilities().m_FormatSupport[textureFormat].AreAllSet(ezGALResourceFormatSupport::Texture);
  if (bSupported)
  {
    ezBindGroupBuilder& bindGroupTest = ezRenderContext::GetDefaultInstance()->GetBindGroup();
    m_hTextureCube = ezResourceManager::LoadResource<ezTextureCubeResource>(sTextureResourceId);
    bindGroupTest.BindTexture("DiffuseTexture", m_hTextureCube);
  }
  BeginRendering(ezColor::Black);

  if (bSupported)
  {
    RenderObjects(ezShaderBindFlags::Default);
  }

  EndRendering();

  if (bSupported)
  {
    TransitionTexture(GetBackbuffer(), ezGALResourceState::CopySource);
    EZ_TEST_IMAGE(m_iFrame, 200);
  }
  EndCommands();
  EndFrame();

  return m_iFrame < (iNumFrames - 1) ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}
