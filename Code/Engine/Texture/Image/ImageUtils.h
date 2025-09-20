#pragma once

#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/Image/ImageFilter.h>

/// \brief Collection of utility functions for image processing, analysis, and manipulation.
class EZ_TEXTURE_DLL ezImageUtils
{
public:
  /// \brief Computes the absolute difference between two images for comparison analysis.
  ///
  /// Both images must have the same dimensions and format. The output difference image
  /// contains the absolute difference for each channel. Useful for regression testing
  /// and quality analysis.
  static void ComputeImageDifferenceABS(const ezImageView& imageA, const ezImageView& imageB, ezImage& out_difference);

  /// \brief Computes relaxed difference allowing for 1-pixel shifts between images.
  ///
  /// For each pixel in imageA, searches for the minimum difference within a 1-pixel radius
  /// in imageB. This accounts for minor alignment differences or sub-pixel shifts that
  /// shouldn't be considered significant differences.
  static void ComputeImageDifferenceABSRelaxed(const ezImageView& imageA, const ezImageView& imageB, ezImage& out_difference);

  /// \brief Computes the mean square error for the block at (offsetx, offsety) to (offsetx + uiBlockSize, offsety + uiBlockSize).
  /// DifferenceImage is expected to be an image that represents the difference between two images.
  static ezUInt32 ComputeMeanSquareError(const ezImageView& differenceImage, ezUInt8 uiBlockSize, ezUInt32 uiOffsetx, ezUInt32 uiOffsety);

  /// \brief Computes the mean square error of DifferenceImage, by computing the MSE for blocks of uiBlockSize and returning the maximum MSE
  /// that was found.
  static ezUInt32 ComputeMeanSquareError(const ezImageView& differenceImage, ezUInt8 uiBlockSize);

  /// \brief Rescales pixel values to use the full value range by scaling from [min, max] to [0, 255].
  /// Computes combined min/max for RGB and separate min/max for alpha.
  static void Normalize(ezImage& ref_image);
  static void Normalize(ezImage& ref_image, ezUInt8& ref_uiMinRgb, ezUInt8& ref_uiMaxRgb, ezUInt8& ref_uiMinAlpha, ezUInt8& ref_uiMaxAlpha);

  /// \brief Extracts the alpha channel from 8bpp 4 channel images into a 8bpp single channel image.
  static void ExtractAlphaChannel(const ezImageView& inputImage, ezImage& ref_outputImage);

  /// \brief Returns the sub-image of \a input that starts at \a offset and has the size \a newsize
  static void CropImage(const ezImageView& input, const ezVec2I32& vOffset, const ezSizeU32& newsize, ezImage& ref_output);

  /// \brief rotates a sub image by 180 degrees in place. Only works with uncompressed images.
  static void RotateSubImage180(ezImage& ref_image, ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0);

  /// \brief Copies the source image into the destination image at the specified location.
  ///
  /// The image must fit, no scaling or cropping is done. Image formats must be identical. Compressed formats are not supported.
  /// If the target location leaves not enough room for the source image to be copied, bad stuff will happen.
  static ezResult Copy(const ezImageView& srcImg, const ezRectU32& srcRect, ezImage& ref_dstImg, const ezVec3U32& vDstOffset, ezUInt32 uiDstMipLevel = 0,
    ezUInt32 uiDstFace = 0, ezUInt32 uiDstArrayIndex = 0);

  /// \brief Copies the lower uiNumMips data of a 2D image into another one.
  static ezResult ExtractLowerMipChain(const ezImageView& src, ezImage& ref_dst, ezUInt32 uiNumMips);

  /// Mip map generation options
  struct MipMapOptions
  {
    /// The filter to use for mipmap generation. Defaults to bilinear filtering (Triangle filter) if none is given.
    const ezImageFilter* m_filter = nullptr;

    /// Rescale RGB components to unit length for normal maps.
    ///
    /// Enable this when generating mipmaps for normal maps to maintain
    /// proper normal vector length after filtering.
    bool m_renormalizeNormals = false;

    /// Preserve alpha coverage for alpha testing.
    ///
    /// When enabled, alpha values are scaled to maintain the same coverage
    /// percentage as the original image when using alpha testing. Essential
    /// for proper rendering of vegetation and other alpha-tested geometry.
    bool m_preserveCoverage = false;

    /// Alpha test threshold for coverage preservation.
    ///
    /// Only used when m_preserveCoverage is true. Pixels with alpha >= threshold
    /// are considered opaque for coverage calculations.
    float m_alphaThreshold = 0.5f;

    /// The address mode for samples when filtering outside of the image dimensions in the horizontal direction.
    ezImageAddressMode::Enum m_addressModeU = ezImageAddressMode::Clamp;

    /// The address mode for samples when filtering outside of the image dimensions in the vertical direction.
    ezImageAddressMode::Enum m_addressModeV = ezImageAddressMode::Clamp;

    /// The address mode for samples when filtering outside of the image dimensions in the depth direction.
    ezImageAddressMode::Enum m_addressModeW = ezImageAddressMode::Clamp;

    /// Border color for ClampBorder address mode.
    ezColor m_borderColor = ezColor::Black;

    /// Number of mip levels to generate.
    ///
    /// Set to 0 to generate all possible mip levels down to 1x1.
    /// Set to a specific value to limit the number of generated levels.
    ezUInt32 m_numMipMaps = 0;
  };

  /// Scales the image.
  static ezResult Scale(const ezImageView& source, ezImage& ref_target, ezUInt32 uiWidth, ezUInt32 uiHeight, const ezImageFilter* pFilter = nullptr,
    ezImageAddressMode::Enum addressModeU = ezImageAddressMode::Clamp, ezImageAddressMode::Enum addressModeV = ezImageAddressMode::Clamp,
    const ezColor& borderColor = ezColor::Black);

  /// Scales the image.
  static ezResult Scale3D(const ezImageView& source, ezImage& ref_target, ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiDepth,
    const ezImageFilter* pFilter = nullptr, ezImageAddressMode::Enum addressModeU = ezImageAddressMode::Clamp,
    ezImageAddressMode::Enum addressModeV = ezImageAddressMode::Clamp, ezImageAddressMode::Enum addressModeW = ezImageAddressMode::Clamp,
    const ezColor& borderColor = ezColor::Black);

  /// Genererates the mip maps for the image. The input texture must be in ezImageFormat::R32_G32_B32_A32_FLOAT
  static void GenerateMipMaps(const ezImageView& source, ezImage& ref_target, const MipMapOptions& options);

  /// Assumes that the Red and Green components of an image contain XY of an unit length normal and reconstructs the Z component into B
  static void ReconstructNormalZ(ezImage& ref_source);

  /// Renormalizes a normal map to unit length.
  static void RenormalizeNormalMap(ezImage& ref_image);

  /// Adjust the roughness in lower mip levels so it maintains the same look from all distances.
  static void AdjustRoughness(ezImage& ref_roughnessMap, const ezImageView& normalMap);

  /// \brief Changes the exposure of an HDR image by 2^bias
  static void ChangeExposure(ezImage& ref_image, float fBias);

  /// \brief Creates a cubemap from srcImg and stores it in dstImg.
  ///
  /// If srcImg is already a cubemap, the data will be copied 1:1 to dstImg.
  /// If it is a 2D texture, it is analyzed and sub-images are copied to the proper faces of the output cubemap.
  ///
  /// Supported input layouts are:
  ///  * Vertical Cross
  ///  * Horizontal Cross
  ///  * Spherical mapping
  static ezResult CreateCubemapFromSingleFile(ezImage& ref_dstImg, const ezImageView& srcImg);

  /// \brief Copies the 6 given source images to the faces of dstImg.
  ///
  /// All input images must have the same square, power-of-two dimensions and mustn't be compressed.
  static ezResult CreateCubemapFrom6Files(ezImage& ref_dstImg, const ezImageView* pSourceImages);

  static ezResult CreateVolumeTextureFromSingleFile(ezImage& ref_dstImg, const ezImageView& srcImg);

  static ezUInt32 GetSampleIndex(ezUInt32 uiNumTexels, ezInt32 iIndex, ezImageAddressMode::Enum addressMode, bool& out_bUseBorderColor);

  /// \brief Samples the image at the given UV coordinates with nearest filtering.
  ///
  /// This function has to validate that the image is of the right format, and has to query the pixel pointer, which is slow.
  /// If you need to sample the image very often, use the overload that takes a pixel pointer instead of an image.
  static ezColor NearestSample(const ezImageView& image, ezImageAddressMode::Enum addressMode, ezVec2 vUv);

  /// \brief Samples the image at the given UV coordinates with nearest filtering.
  ///
  /// Prefer this function over the one that takes an ezImageView when you need to sample the image very often,
  /// as it does away with internal validation that would be redundant. Also, the pixel pointer given to this function
  /// should be retrieved only once from the source image, as ezImage::GetPixelPointer() is rather slow due to validation overhead.
  static ezColor NearestSample(const ezColor* pPixelPointer, ezUInt32 uiWidth, ezUInt32 uiHeight, ezImageAddressMode::Enum addressMode, ezVec2 vUv);

  /// \brief Samples the image at the given UV coordinates with bilinear filtering.
  ///
  /// This function has to validate that the image is of the right format, and has to query the pixel pointer, which is slow.
  /// If you need to sample the image very often, use the overload that takes a pixel pointer instead of an image.
  static ezColor BilinearSample(const ezImageView& image, ezImageAddressMode::Enum addressMode, ezVec2 vUv);

  /// \brief Samples the image at the given UV coordinates with bilinear filtering.
  ///
  /// Prefer this function over the one that takes an ezImageView when you need to sample the image very often,
  /// as it does away with internal validation that would be redundant. Also, the pixel pointer given to this function
  /// should be retrieved only once from the source image, as ezImage::GetPixelPointer() is rather slow due to validation overhead.
  static ezColor BilinearSample(const ezColor* pPixelPointer, ezUInt32 uiWidth, ezUInt32 uiHeight, ezImageAddressMode::Enum addressMode, ezVec2 vUv);

  /// \brief Copies channel 0, 1, 2 or 3 from srcImg into dstImg.
  ///
  /// Currently only supports images of format R32G32B32A32_FLOAT and with identical resolution.
  /// Returns failure if any of those requirements are not met.
  static ezResult CopyChannel(ezImage& ref_dstImg, ezUInt8 uiDstChannelIdx, const ezImage& srcImg, ezUInt8 uiSrcChannelIdx);

  /// \brief Embeds the image as Base64 encoded text into an HTML file.
  static void EmbedImageData(ezStringBuilder& out_sHtml, const ezImage& image);

  /// \brief Generates an HTML file containing the given images with mouse-over functionality to compare them.
  static void CreateImageDiffHtml(ezStringBuilder& out_sHtml, ezStringView sTitle, const ezImage& referenceImgRgb, const ezImage& referenceImgAlpha, const ezImage& capturedImgRgb, const ezImage& capturedImgAlpha, const ezImage& diffImgRgb, const ezImage& diffImgAlpha, ezUInt32 uiError, ezUInt32 uiThreshold, ezUInt8 uiMinDiffRgb, ezUInt8 uiMaxDiffRgb, ezUInt8 uiMinDiffAlpha, ezUInt8 uiMaxDiffAlpha);
};
