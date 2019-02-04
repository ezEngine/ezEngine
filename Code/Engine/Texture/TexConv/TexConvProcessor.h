#pragma once

#include <Foundation/Math/Rect.h>
#include <Texture/TexConv/TexConvDesc.h>
#include <Foundation/IO/MemoryStream.h>

class ezTextureGroupDesc;

class EZ_TEXTURE_DLL ezTexConvProcessor
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezTexConvProcessor);

public:
  ezTexConvProcessor();

  ezTexConvDesc m_Descriptor;

  ezResult Process();

  ezImage m_OutputImage;
  ezImage m_LowResOutputImage;
  ezImage m_ThumbnailOutputImage;
  ezMemoryStreamStorage m_DecalAtlas;

  ezResult WriteTexFile(ezStreamWriter& stream, const ezImage& image);

private:
  ezEnum<ezImageFormat> m_OutputImageFormat;

  ezImage* m_pCurrentScratchImage = nullptr;
  ezImage* m_pOtherScratchImage = nullptr;

  ezImage m_ScratchImage1;
  ezImage m_ScratchImage2;

  ezUInt32 m_uiTargetResolutionX = 0;
  ezUInt32 m_uiTargetResolutionY = 0;

  ezUInt32 m_uiNumChannels = 4;

  enum DecalLayer
  {
    BaseColor,
    Normal,
    ENUM_COUNT,
  };

  struct DecalDesc
  {
    ezString m_sIdentifier;
    ezString m_sFile[DecalLayer::ENUM_COUNT];
    ezImage m_Image[DecalLayer::ENUM_COUNT];
    ezRectU32 m_Rect[DecalLayer::ENUM_COUNT];
  };

  ezDynamicArray<DecalDesc> m_Decals;

  ezResult DetectNumChannels();
  ezResult LoadInputImages();
  ezResult AdjustTargetFormat();
  ezResult ForceSRGBFormats();
  ezResult ChooseOutputFormat();
  ezResult DetermineTargetResolution();
  ezResult ConvertInputImagesToFloat32();
  ezResult ResizeInputImagesToSameDimensions();
  ezResult Assemble2DTexture();
  ezResult AdjustHdrExposure();
  ezResult PremultiplyAlpha();
  ezResult Assemble2DSlice(const ezTexConvSliceChannelMapping& mapping, ezColor* pPixelOut);
  ezResult GenerateMipmaps();
  ezResult GenerateOutput();

  ezResult GenerateDecalAtlas();
  ezResult LoadDecalInputs(ezTextureGroupDesc& decalAtlasDesc);
  static ezResult WriteDecalAtlasInfo(ezDynamicArray<DecalDesc>& decals, ezStreamWriter& stream);
  ezResult CreateDecalLayerTexture(ezInt32 layer, ezStreamWriter& stream);
  static ezResult TrySortDecalsIntoAtlas(ezDynamicArray<DecalDesc>& decals, ezUInt32 uiWidth, ezUInt32 uiHeight, ezInt32 layer);
  static ezResult SortDecalsIntoAtlas(ezDynamicArray<DecalDesc>& decals, ezUInt32& out_ResX, ezUInt32& out_ResY, ezInt32 layer);
  static ezResult CreateDecalAtlasTexture(ezDynamicArray<DecalDesc>& decals, ezUInt32 uiResX, ezUInt32 uiResY, ezImage& atlas, ezInt32 layer);

  ezResult GenerateThumbnailOutput();
  ezResult GenerateLowResOutput();
};


