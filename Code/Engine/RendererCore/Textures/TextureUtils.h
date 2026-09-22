#pragma once

#include <Foundation/Configuration/CVar.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>
#include <Texture/Image/Image.h>

/// How many of the highest mipmaps to skip when a texture is uploaded.
///
/// This reduces every texture by the same factor, independent of how large it is, which is what makes a
/// quality setting have an effect in a project whose textures are all fairly small already.
/// It is applied before the resolution limits below, which then put absolute bounds on the result.
EZ_RENDERERCORE_DLL extern ezCVarInt cvar_RenderingTexturesDropMips;

/// The resolution that textures are not reduced below.
///
/// Textures that are already smaller than this are never reduced at all. This keeps DropMips from
/// destroying small textures such as UI icons and lookup textures.
EZ_RENDERERCORE_DLL extern ezCVarInt cvar_RenderingTexturesMinResolution;

/// The maximum resolution that textures are uploaded at.
///
/// Textures that are larger lose their highest mipmaps, which reduces GPU memory usage at the cost of
/// detail. The default (16384) is large enough not to affect anything.
/// If this is smaller than the minimum resolution, this one wins, it is a hard cap.
EZ_RENDERERCORE_DLL extern ezCVarInt cvar_RenderingTexturesMaxResolution;

// All three CVars above are read while a texture is uploaded. Changing any of them reloads every texture
// resource so that it applies immediately, which reads them all from disk again and is therefore only
// suitable for a deliberate change such as an options menu.
// Note also that they cap the size a texture is uploaded at, they do not prevent the full file from being
// read from disk.

/// Utility functions for texture format conversion and manipulation.
struct EZ_RENDERERCORE_DLL ezTextureUtils
{
  /// Converts an image format to the corresponding GPU texture format.
  ///
  /// The bSRGB parameter determines whether to use sRGB or linear variants.
  static ezGALResourceFormat::Enum ImageFormatToGalFormat(ezImageFormat::Enum format, bool bSRGB);

  /// Converts a GPU texture format to the corresponding image format.
  ///
  /// If bRemoveSRGB is true, sRGB formats are converted to their linear equivalents.
  static ezImageFormat::Enum GalFormatToImageFormat(ezGALResourceFormat::Enum format, bool bRemoveSRGB);

  /// Converts a GPU texture format to the corresponding image format.
  static ezImageFormat::Enum GalFormatToImageFormat(ezGALResourceFormat::Enum format);

  /// Configures a sampler state based on a texture filter setting.
  ///
  /// Sets up filtering mode, addressing, and anisotropy based on the filter enum.
  static void ConfigureSampler(ezTextureFilterSetting::Enum filter, ezGALSamplerStateCreationDescription& out_sampler);

  /// Copies the given texture subresource from `memory` into `out_image` according to the texture description.
  static void CopySubResourceToImage(const ezGALTextureCreationDescription& desc, const ezGALTextureSubresource& subResource, const ezGALSystemMemoryDescription& memory, ezImage& out_image, bool bRemoveSRGB);
  /// Returns an image view of the texture subresource in `memory`. If the format allows for it, the memory will be aliased, removing the need to copy the data but the view becomes invalid once the memory does. If this is not possible, the function reverts to calling CopySubResourceToImage with `ref_tempImage` used as the data storage. You can check if `ref_tempImage` is valid to figure out which code path was taken.
  static ezImageView MakeImageViewFromSubResource(const ezGALTextureCreationDescription& desc, const ezGALTextureSubresource& subResource, const ezGALSystemMemoryDescription& memory, ezImage& ref_tempImage, bool bRemoveSRGB);
  /// Copies a texture subresource memory to a new location with a different row pitch.
  static void CopySubResourceToMemory(const ezGALTextureCreationDescription& desc, const ezGALTextureSubresource& subResource, const ezGALSystemMemoryDescription& sourceMemory, ezArrayPtr<ezUInt8> targetData, ezUInt32 uiTargetRowPitch);

  /// Returns how many mip levels of the given image may be uploaded, according to the texture quality CVars.
  ///
  /// Mipmaps are uploaded starting at the smallest one, so the number of levels decides which mip ends up
  /// being the largest. The result is
  ///   clamp(mipCount - DropMips, levelsFor(MinResolution), levelsFor(MaxResolution))
  /// and is always at least 1, so that no texture disappears entirely.
  ///
  /// Call this once per texture and use the result for every upload stage of that texture. Deriving the
  /// 'is there more to load' state from anything else will not terminate, because a limited texture never
  /// reaches its own mip 0.
  static ezUInt32 GetMaxMipLevelsToUpload(const ezImage* pImage);

  /// If enabled, textures are always loaded to full quality immediately. Mostly necessary for image comparison unit tests.
  static bool s_bForceFullQualityAlways;
};
