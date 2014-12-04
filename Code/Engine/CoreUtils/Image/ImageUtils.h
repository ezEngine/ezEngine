#pragma once

#include <CoreUtils/Image/Image.h>
#include <Foundation/Math/Size.h>

class EZ_COREUTILS_DLL ezImageUtils
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

  static void ScaleDownHalf(const ezImage& Image, ezImage& out_Result);
};

