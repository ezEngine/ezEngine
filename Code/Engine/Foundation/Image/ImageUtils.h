#pragma once

#include <Foundation/Image/Image.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Image/ImageFilter.h>

class EZ_FOUNDATION_DLL ezImageUtils
{
public:
  /// \brief Returns the image with the difference (absolute values) between ImageA and ImageB.
  static void ComputeImageDifferenceABS(const ezImageView& ImageA, const ezImageView& ImageB, ezImage& out_Difference);

  /// \brief Computes the mean square error for the block at (offsetx, offsety) to (offsetx + uiBlockSize, offsety + uiBlockSize). DifferenceImage is expected to be an image that represents the difference between two images.
  static ezUInt32 ComputeMeanSquareError(const ezImageView& DifferenceImage, ezUInt8 uiBlockSize, ezUInt32 offsetx, ezUInt32 offsety);

  /// \brief Computes the mean square error of DifferenceImage, by computing the mse for blocks of uiBlockSize and returning the maximum mse that was found.
  static ezUInt32 ComputeMeanSquareError(const ezImageView& DifferenceImage, ezUInt8 uiBlockSize);

  /// \brief Returns the subimage of \a input that starts at \a offset and has the size \a newsize
  static void CropImage(const ezImageView& input, const ezVec2I32& offset, const ezSizeU32& newsize, ezImage& output);

  /// \brief rotates a sub image by 180 degrees in place. Only works with uncompressed images.
  static void RotateSubImage180(ezImage& image, ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0);

  /// \brief Copies the source image into the destination image at the specified location.
  ///
  /// The image must fit, no scaling or cropping is done. Image formats must be identical. Compressed formats are not supported.
  /// If the target location leaves not enough room for the source image to be copied, bad stuff will happen.
  static void Copy(ezImage& dst, ezUInt32 uiPosX, ezUInt32 uiPosY, const ezImageView& src, ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0,
                   ezUInt32 uiArrayIndex = 0);

  /// \brief Copies the lower uiNumMips data of a 2D image into another one.
  ///
  /// Does not support 3D, cubemap or array textures.
  static ezResult ExtractLowerMipChain(const ezImageView& src, ezImage& dst, ezUInt8 uiNumMips);

    /// Mip map generation options
  struct MipMapOptions
  {
    MipMapOptions()
    {
      m_filter = nullptr;
      m_renormalizeNormals = false;
      m_preserveCoverage = false;
      m_alphaThreshold = 0.5f;

      m_addressModeU = ezImageAddressMode::CLAMP;
      m_addressModeV = ezImageAddressMode::CLAMP;
      m_addressModeW = ezImageAddressMode::CLAMP;

      m_borderColor = ezColor::Black;
      m_numMipMaps = 0;
    }

    /// The filter to use for mipmap generation. Defaults to bilinear filtering (Triangle filter) if none is given.
    const ezImageFilter* m_filter;

    /// Rescale RGB components to unit length.
    bool m_renormalizeNormals;

    /// If true, the alpha values are scaled to preserve the average coverage when alpha testing is enabled,
    bool m_preserveCoverage;

    /// The alpha test threshold to use when m_preserveCoverage == true.
    float m_alphaThreshold;

    /// The address mode for samples when filtering outside of the image dimensions in the horizontal direction.
    ezImageAddressMode::Enum m_addressModeU;

    /// The address mode for samples when filtering outside of the image dimensions in the vertical direction.
    ezImageAddressMode::Enum m_addressModeV;

    /// The address mode for samples when filtering outside of the image dimensions in the depth direction.
    ezImageAddressMode::Enum m_addressModeW;

    /// The border color if texture address mode equals BORDER.
    ezColor m_borderColor;

    /// How many mip maps should be generated. Pass 0 to generate all mip map levels.
    ezUInt32 m_numMipMaps;
  };

  /// Scales the image.
  static ezResult Scale(const ezImageView& source, ezImage& target, ezUInt32 width, ezUInt32 height,
                        const ezImageFilter* filter = nullptr, ezImageAddressMode::Enum addressModeU = ezImageAddressMode::CLAMP,
                        ezImageAddressMode::Enum addressModeV = ezImageAddressMode::CLAMP, const ezColor& borderColor = ezColor::Black);

  /// Scales the image.
  static ezResult Scale3D(const ezImageView& source, ezImage& target, ezUInt32 width, ezUInt32 height, ezUInt32 depth,
                          const ezImageFilter* filter = nullptr, ezImageAddressMode::Enum addressModeU = ezImageAddressMode::CLAMP,
                          ezImageAddressMode::Enum addressModeV = ezImageAddressMode::CLAMP,
                          ezImageAddressMode::Enum addressModeW = ezImageAddressMode::CLAMP, const ezColor& borderColor = ezColor::Black);

  /// Genererates the mip maps for the image. The input texture must be in ezImageFormat::R32_G32_B32_A32_FLOAT
  static void GenerateMipMaps(const ezImageView& source, ezImage& target, const MipMapOptions& options);

  /// Assumes that the Red and Green components of an image contain XY of an unit length normal and reconstructs the Z component into B
  static void ReconstructNormalZ(ezImage& source);

  /// Renormalizes a normal map to unit length.
  static void RenormalizeNormalMap(ezImage& image);

  /// Adjust the roughness in lower mip levels so it maintains the same look from all distances.
  static void AdjustRoughness(ezImage& roughnessMap, const ezImageView& normalMap);

  /// Converts the image to cubemap.
  /// The input image must be in ezImageFormat::R32_G32_B32_A32_FLOAT.
  ///
  /// Supported input image layouts:
  ///   horizontal cross
  ///   vertical cross
  ///   horizontal strip
  ///   vertical strip
  ///   spherical map (a.k.a. latitude-longitude or cylindrical)
  static void ConvertToCubemap(const ezImageView& src, ezImage& dst);
};
