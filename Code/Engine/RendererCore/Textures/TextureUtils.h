#pragma once

#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>
#include <Texture/Image/Image.h>

struct EZ_RENDERERCORE_DLL ezTextureUtils
{
  static ezGALResourceFormat::Enum ImageFormatToGalFormat(ezImageFormat::Enum format, bool bSRGB);
  static ezImageFormat::Enum GalFormatToImageFormat(ezGALResourceFormat::Enum format, bool bRemoveSRGB);
  static ezImageFormat::Enum GalFormatToImageFormat(ezGALResourceFormat::Enum format);


  static void ConfigureSampler(ezTextureFilterSetting::Enum filter, ezGALSamplerStateCreationDescription& out_sampler);

  static void CreateSubResourceImage(const ezGALTextureCreationDescription& desc, const ezGALTextureSubresource& subResource, ezGALSystemMemoryDescription& memory, ezImage& out_Image);
  static ezImageView CreateSubResourceView(const ezGALTextureCreationDescription& desc, const ezGALTextureSubresource& subResource, ezGALSystemMemoryDescription& memory, ezImage& ref_Temp);

  /// \brief If enabled, textures are always loaded to full quality immediately. Mostly necessary for image comparison unit tests.
  static bool s_bForceFullQualityAlways;
};
