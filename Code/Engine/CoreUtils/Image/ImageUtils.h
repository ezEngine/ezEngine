#pragma once

#include <CoreUtils/Image/Image.h>

class EZ_COREUTILS_DLL ezImageUtils
{
public:

  static void ComputeImageDifference(const ezImage& ImageA, const ezImage& ImageB, ezImage& out_Difference);

};

