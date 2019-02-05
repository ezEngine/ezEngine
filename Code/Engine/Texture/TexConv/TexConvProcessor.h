#pragma once

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/Rect.h>
#include <Texture/TexConv/TexConvDesc.h>

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

private:

  //////////////////////////////////////////////////////////////////////////
  // Modifying the Descriptor

  ezResult LoadInputImages();
  ezResult ForceSRGBFormats();
  ezResult ConvertAndScaleInputImages(ezUInt32 uiResolutionX, ezUInt32 uiResolutionY);

  //////////////////////////////////////////////////////////////////////////
  // Reading from the descriptor

  ezResult ChooseOutputFormat(ezEnum<ezImageFormat>& out_Format, ezEnum<ezTexConvUsage> usage, ezUInt32 uiNumChannels) const;
  ezResult DetermineTargetResolution(const ezImage& image, ezEnum<ezImageFormat> OutputImageFormat, ezUInt32& out_uiTargetResolutionX, ezUInt32& out_uiTargetResolutionY) const;
  ezResult Assemble2DTexture(const ezImageHeader& refImg, ezImage& dst) const;
  ezResult AdjustHdrExposure(ezImage& img) const;
  ezResult PremultiplyAlpha(ezImage& image) const;
  ezResult Assemble2DSlice(const ezTexConvSliceChannelMapping& mapping, ezUInt32 uiResolutionX, ezUInt32 uiResolutionY, ezColor* pPixelOut) const;
  ezResult GenerateMipmaps(ezImage& img) const;

  //////////////////////////////////////////////////////////////////////////
  // Purely functional

  static ezResult DetectNumChannels(ezArrayPtr<const ezTexConvSliceChannelMapping> channelMapping, ezUInt32& uiNumChannels);
  static ezResult AdjustUsage(const char* szFilename, const ezImage& srcImg, ezEnum<ezTexConvUsage>& inout_Usage);
  static ezResult ConvertAndScaleImage(const char* szImageName, ezImage& inout_Image, ezUInt32 uiResolutionX, ezUInt32 uiResolutionY);

  //////////////////////////////////////////////////////////////////////////
  // Output Generation

  static ezResult GenerateOutput(ezImage&& src, ezImage& dst, ezEnum<ezImageFormat> format);
  static ezResult GenerateThumbnailOutput(const ezImage& srcImg, ezImage& dstImg, ezUInt32 uiTargetRes);
  static ezResult GenerateLowResOutput(const ezImage& srcImg, ezImage& dstImg, ezUInt32 uiLowResMip);

  //////////////////////////////////////////////////////////////////////////
  // Texture Atlas

  enum TextureAtlasLayer
  {
    BaseColor,
    Normal,
    ENUM_COUNT,
  };

  struct TextureAtlasItem
  {
    ezString m_sIdentifier;
    ezString m_sInputFile[TextureAtlasLayer::ENUM_COUNT];
    ezImage m_InputImage[TextureAtlasLayer::ENUM_COUNT];
    ezRectU32 m_AtlasRect[TextureAtlasLayer::ENUM_COUNT];
  };

  ezResult LoadAtlasInputs(const ezTextureGroupDesc& atlasDesc, ezDynamicArray<TextureAtlasItem>& items) const;
  ezResult CreateAtlasLayerTexture(ezDynamicArray<TextureAtlasItem>& atlasItems, ezInt32 layer, ezStreamWriter& stream);

  static ezResult WriteTextureAtlasInfo(const ezDynamicArray<TextureAtlasItem>& decals, ezStreamWriter& stream);
  static ezResult TrySortItemsIntoAtlas(ezDynamicArray<TextureAtlasItem>& items, ezUInt32 uiWidth, ezUInt32 uiHeight, ezInt32 layer);
  static ezResult SortItemsIntoAtlas(ezDynamicArray<TextureAtlasItem>& decals, ezUInt32& out_ResX, ezUInt32& out_ResY, ezInt32 layer);
  static ezResult CreateAtlasTexture(ezDynamicArray<TextureAtlasItem>& decals, ezUInt32 uiResX, ezUInt32 uiResY, ezImage& atlas, ezInt32 layer);

  //////////////////////////////////////////////////////////////////////////
  // Decal Atlas

  ezResult GenerateDecalAtlas(ezMemoryStreamWriter& stream);

};


