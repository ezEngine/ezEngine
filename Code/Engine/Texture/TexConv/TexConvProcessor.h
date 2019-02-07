#pragma once

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/Rect.h>
#include <Texture/TexConv/TexConvDesc.h>

struct ezTextureAtlasCreationDesc;

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
  ezMemoryStreamStorage m_TextureAtlas;

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
  ezResult AssembleCubemap(ezImage& dst) const;
  ezResult AdjustHdrExposure(ezImage& img) const;
  ezResult PremultiplyAlpha(ezImage& image) const;
  ezResult Assemble2DSlice(const ezTexConvSliceChannelMapping& mapping, ezUInt32 uiResolutionX, ezUInt32 uiResolutionY, ezColor* pPixelOut) const;
  ezResult GenerateMipmaps(ezImage& img, ezUInt32 uiNumMips = 0) const;

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

  struct TextureAtlasItem
  {
    ezUInt32 m_uiUniqueID = 0;
    ezImage m_InputImage[4];
    ezRectU32 m_AtlasRect[4];
  };

  ezResult LoadAtlasInputs(const ezTextureAtlasCreationDesc& atlasDesc, ezDynamicArray<TextureAtlasItem>& items) const;
  ezResult CreateAtlasLayerTexture(const ezTextureAtlasCreationDesc& atlasDesc, ezDynamicArray<TextureAtlasItem>& atlasItems, ezInt32 layer, ezImage& dstImg, ezUInt32 uiNumMipmaps);

  static ezResult WriteTextureAtlasInfo(const ezDynamicArray<TextureAtlasItem>& atlasItems, ezUInt32 uiNumLayers, ezStreamWriter& stream);
  static ezResult TrySortItemsIntoAtlas(ezDynamicArray<TextureAtlasItem>& items, ezUInt32 uiWidth, ezUInt32 uiHeight, ezInt32 layer, ezUInt32 uiPixelAlign);
  static ezResult SortItemsIntoAtlas(ezDynamicArray<TextureAtlasItem>& items, ezUInt32& out_ResX, ezUInt32& out_ResY, ezInt32 layer, ezUInt32 uiPixelAlign);
  static ezResult CreateAtlasTexture(ezDynamicArray<TextureAtlasItem>& items, ezUInt32 uiResX, ezUInt32 uiResY, ezImage& atlas, ezInt32 layer);

  //////////////////////////////////////////////////////////////////////////
  // Texture Atlas

  ezResult GenerateTextureAtlas(ezMemoryStreamWriter& stream);

};


