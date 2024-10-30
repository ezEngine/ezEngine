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

  /// \brief Copies the given texture subresource from `memory` into `out_image` according to the texture description.
  static void CopySubResourceToImage(const ezGALTextureCreationDescription& desc, const ezGALTextureSubresource& subResource, const ezGALSystemMemoryDescription& memory, ezImage& out_image, bool bRemoveSRGB);
  /// \brief Returns an image view of the texture subresource in `memory`. If the format allows for it, the memory will be aliased, removing the need to copy the data but the view becomes invalid once the memory does. If this is not possible, the function reverts to calling CopySubResourceToImage with `ref_tempImage` used as the data storage. You can check if `ref_tempImage` is valid to figure out which code path was taken.
  static ezImageView MakeImageViewFromSubResource(const ezGALTextureCreationDescription& desc, const ezGALTextureSubresource& subResource, const ezGALSystemMemoryDescription& memory, ezImage& ref_tempImage, bool bRemoveSRGB);
  /// \brief Copies a texture subresource memory to a new location with a different row pitch.
  static void CopySubResourceToMemory(const ezGALTextureCreationDescription& desc, const ezGALTextureSubresource& subResource, const ezGALSystemMemoryDescription& sourceMemory, ezArrayPtr<ezUInt8> targetData, ezUInt32 uiTargetRowPitch);

  /// \brief If enabled, textures are always loaded to full quality immediately. Mostly necessary for image comparison unit tests.
  static bool s_bForceFullQualityAlways;
};
