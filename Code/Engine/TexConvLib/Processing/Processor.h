#pragma once

#include <TexConvLib/Basics.h>
#include <TexConvLib/Configuration/TexConvDesc.h>

class EZ_TEXCONV_DLL ezTexConvProcessor
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezTexConvProcessor);
public:
  ezTexConvProcessor();

  ezTexConvDesc m_Descriptor;

  ezResult Process();

  ezImage m_OutputImage;
  ezImage m_LowResOutputImage;
  ezImage m_ThumbnailOutputImage;

private:
  ezEnum<ezImageFormat> m_OutputImageFormat;

  ezImage* m_pCurrentScratchImage = nullptr;
  ezImage* m_pOtherScratchImage = nullptr;

  ezImage m_ScratchImage1;
  ezImage m_ScratchImage2;

  ezUInt32 m_uiTargetResolutionX = 0;
  ezUInt32 m_uiTargetResolutionY = 0;

  ezResult LoadInputImages();
  ezResult AdjustTargetFormat();
  ezResult ChooseOutputFormat();
  ezResult DetermineTargetResolution();
  ezResult ConvertInputImagesToFloat32();
  ezResult ResizeInputImagesToSameDimensions();
  ezResult Assemble2DTexture();
  ezResult Assemble2DSlice(const ezTexConvSliceChannelMapping& mapping, ezColor* pPixelOut);
  ezResult GenerateMipmaps();
  ezResult GenerateOutput();

  ezResult GenerateThumbnailOutput();


};

