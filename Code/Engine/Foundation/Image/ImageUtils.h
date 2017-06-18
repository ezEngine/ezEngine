#pragma once

#include <Foundation/Image/Image.h>
#include <Foundation/Math/Size.h>

class EZ_FOUNDATION_DLL ezImageUtils
{
public:

  /// \brief Returns the image with the difference (absolute values) between ImageA and ImageB.
  static void ComputeImageDifferenceABS(const ezImage& ImageA, const ezImage& ImageB, ezImage& out_Difference);

  /// \brief Computes the mean square error for the block at (offsetx, offsety) to (offsetx + uiBlockSize, offsety + uiBlockSize). DifferenceImage is expected to be an image that represents the difference between two images.
  static ezUInt32 ComputeMeanSquareError(const ezImage& DifferenceImage, ezUInt8 uiBlockSize, ezUInt32 offsetx, ezUInt32 offsety);

  /// \brief Computes the mean square error of DifferenceImage, by computing the mse for blocks of uiBlockSize and returning the maximum mse that was found.
  static ezUInt32 ComputeMeanSquareError(const ezImage& DifferenceImage, ezUInt8 uiBlockSize);

  /// \brief Returns the subimage of \a input that starts at \a offset and has the size \a newsize
  static void CropImage(const ezImage& input, const ezVec2I32& offset, const ezSizeU32& newsize, ezImage& output);

  /// \brief The resulting image is the input image scaled down by half along each axis. Only RGB8 and RGBA8 formats are supported.
  static void ScaleDownHalf(const ezImage& Image, ezImage& out_Result);

  /// \brief rotates a sub image by 180 degrees in place. Only works with uncompressed images.
  static void RotateSubImage180(ezImage& image, ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0);

  /// \brief Copies the source image into the destination image at the specified location.
  /// 
  /// The image must fit, no scaling or cropping is done. Image formats must be identical. Compressed formats are not supported.
  /// If the target location leaves not enough room for the source image to be copied, bad stuff will happen.
  static void Copy(ezImage& dst, ezUInt32 uiPosX, ezUInt32 uiPosY, const ezImage& src, ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0);


  static void ScaleDownArbitrary(const ezImage& src, ezUInt32 uiNewWidth, ezUInt32 uiNewHeight, ezImage& out_Result);
};

