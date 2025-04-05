#pragma once

#include "../TestClass/TestClass.h"
#include <RendererTest/../../../Data/UnitTests/RendererTest/Shaders/TestInstancing.h>
EZ_DEFINE_AS_POD_TYPE(ezTestShaderData);

class ezRendererTestUtils
{
public:
  struct ImgColor
  {
    EZ_DECLARE_POD_TYPE();
    ezUInt8 b;
    ezUInt8 g;
    ezUInt8 r;
    ezUInt8 a;
  };

  static ezTransform CreateTransform(const ezUInt32 uiColumns, const ezUInt32 uiRows, ezUInt32 x, ezUInt32 y);

  static void FillStructuredBuffer(ezHybridArray<ezTestShaderData, 16>& ref_instanceData, ezUInt32 uiColorOffset = 0, ezUInt32 uiSlotOffset = 0);

  static void CreateImage(ezImage& ref_image, ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiMipLevelCount, bool bMipLevelIsBlue, ezUInt8 uiFixedBlue = 0);
};
