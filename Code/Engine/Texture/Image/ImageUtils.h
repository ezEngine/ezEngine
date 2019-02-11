#pragma once

#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/Image/ImageFilter.h>

class EZ_TEXTURE_DLL ezImageUtils
{
public:
  /// \brief Returns the image with the difference (absolute values) between ImageA and ImageB.
  static void ComputeImageDifferenceABS(const ezImageView& ImageA, const ezImageView& ImageB, ezImage& out_Difference);

  /// \brief Computes the mean square error for the block at (offsetx, offsety) to (offsetx + uiBlockSize, offsety + uiBlockSize).
  /// DifferenceImage is expected to be an image that represents the difference between two images.
  static ezUInt32 ComputeMeanSquareError(const ezImageView& DifferenceImage, ezUInt8 uiBlockSize, ezUInt32 offsetx, ezUInt32 offsety);

  /// \brief Computes the mean square error of DifferenceImage, by computing the MSE for blocks of uiBlockSize and returning the maximum MSE
  /// that was found.
  static ezUInt32 ComputeMeanSquareError(const ezImageView& DifferenceImage, ezUInt8 uiBlockSize);

  /// \brief Rescales pixel values to use the full value range by scaling from [min, max] to [0, 255].
  /// Computes combined min/max for RGB and separate min/max for alpha.
  static void Normalize(ezImage& image);
  static void Normalize(ezImage& image, ezUInt8& uiMinRgb, ezUInt8& uiMaxRgb, ezUInt8& uiMinAlpha, ezUInt8& uiMaxAlpha);

  /// \brief Extracts the alpha channel from 8bpp 4 channel images into a 8bpp single channel image.
  static void ExtractAlphaChannel(const ezImageView& inputImage, ezImage& outputImage);

  /// \brief Returns the sub-image of \a input that starts at \a offset and has the size \a newsize
  static void CropImage(const ezImageView& input, const ezVec2I32& offset, const ezSizeU32& newsize, ezImage& output);

  /// \brief rotates a sub image by 180 degrees in place. Only works with uncompressed images.
  static void RotateSubImage180(ezImage& image, ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0);

  /// \brief Copies the source image into the destination image at the specified location.
  ///
  /// The image must fit, no scaling or cropping is done. Image formats must be identical. Compressed formats are not supported.
  /// If the target location leaves not enough room for the source image to be copied, bad stuff will happen.
  static ezResult Copy(const ezImageView& srcImg, const ezRectU32& srcRect, ezImage& dstImg, const ezVec3U32& dstOffset,
    ezUInt32 uiDstMipLevel = 0, ezUInt32 uiDstFace = 0, ezUInt32 uiDstArrayIndex = 0);

  /// \brief Copies the lower uiNumMips data of a 2D image into another one.
  static ezResult ExtractLowerMipChain(const ezImageView& src, ezImage& dst, ezUInt32 uiNumMips);

  /// Mip map generation options
  struct MipMapOptions
  {
    /// The filter to use for mipmap generation. Defaults to bilinear filtering (Triangle filter) if none is given.
    const ezImageFilter* m_filter = nullptr;

    /// Rescale RGB components to unit length.
    bool m_renormalizeNormals = false;

    /// If true, the alpha values are scaled to preserve the average coverage when alpha testing is enabled,
    bool m_preserveCoverage = false;

    /// The alpha test threshold to use when m_preserveCoverage == true.
    float m_alphaThreshold = 0.5f;

    /// The address mode for samples when filtering outside of the image dimensions in the horizontal direction.
    ezImageAddressMode::Enum m_addressModeU = ezImageAddressMode::Clamp;

    /// The address mode for samples when filtering outside of the image dimensions in the vertical direction.
    ezImageAddressMode::Enum m_addressModeV = ezImageAddressMode::Clamp;

    /// The address mode for samples when filtering outside of the image dimensions in the depth direction.
    ezImageAddressMode::Enum m_addressModeW = ezImageAddressMode::Clamp;

    /// The border color if texture address mode equals BORDER.
    ezColor m_borderColor = ezColor::Black;

    /// How many mip maps should be generated. Pass 0 to generate all mip map levels.
    ezUInt32 m_numMipMaps = 0;
  };

  /// Scales the image.
  static ezResult Scale(const ezImageView& source, ezImage& target, ezUInt32 width, ezUInt32 height, const ezImageFilter* filter = nullptr,
    ezImageAddressMode::Enum addressModeU = ezImageAddressMode::Clamp, ezImageAddressMode::Enum addressModeV = ezImageAddressMode::Clamp,
    const ezColor& borderColor = ezColor::Black);

  /// Scales the image.
  static ezResult Scale3D(const ezImageView& source, ezImage& target, ezUInt32 width, ezUInt32 height, ezUInt32 depth,
    const ezImageFilter* filter = nullptr, ezImageAddressMode::Enum addressModeU = ezImageAddressMode::Clamp,
    ezImageAddressMode::Enum addressModeV = ezImageAddressMode::Clamp, ezImageAddressMode::Enum addressModeW = ezImageAddressMode::Clamp,
    const ezColor& borderColor = ezColor::Black);

  /// Genererates the mip maps for the image. The input texture must be in ezImageFormat::R32_G32_B32_A32_FLOAT
  static void GenerateMipMaps(const ezImageView& source, ezImage& target, const MipMapOptions& options);

  /// Assumes that the Red and Green components of an image contain XY of an unit length normal and reconstructs the Z component into B
  static void ReconstructNormalZ(ezImage& source);

  /// Renormalizes a normal map to unit length.
  static void RenormalizeNormalMap(ezImage& image);

  /// Adjust the roughness in lower mip levels so it maintains the same look from all distances.
  static void AdjustRoughness(ezImage& roughnessMap, const ezImageView& normalMap);

  /// \brief Changes the exposure of an HDR image by 2^bias
  static void ChangeExposure(ezImage& image, float bias);

  /// \brief Creates a cubemap from srcImg and stores it in dstImg.
  ///
  /// If srcImg is already a cubemap, the data will be copied 1:1 to dstImg.
  /// If it is a 2D texture, it is analyzed and sub-images are copied to the proper faces of the output cubemap.
  ///
  /// Supported input layouts are:
  ///  * Vertical Cross
  ///  * Horizontal Cross
  ///  * Spherical mapping
  static ezResult CreateCubemapFromSingleFile(ezImage& dstImg, const ezImageView& srcImg);

  /// \brief Copies the 6 given source images to the faces of dstImg.
  ///
  /// All input images must have the same square, power-of-two dimensions and mustn't be compressed.
  static ezResult CreateCubemapFrom6Files(ezImage& dstImg, const ezImageView* pSourceImages);

  static ezResult CreateVolumeTextureFromSingleFile(ezImage& dstImg, const ezImageView& srcImg);

  static ezUInt32 GetSampleIndex(ezUInt32 numTexels, ezInt32 index, ezImageAddressMode::Enum addressMode, bool& outUseBorderColor);
};
