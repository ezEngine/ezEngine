#include <TexturePCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Image/Formats/DdsFileFormat.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>
#include <Texture/Utils/TextureAtlasDesc.h>
#include <Texture/Utils/TexturePacker.h>

ezResult ezTexConvProcessor::GenerateTextureAtlas(ezMemoryStreamWriter& stream)
{
  if (m_Descriptor.m_OutputType != ezTexConvOutputType::Atlas)
    return EZ_SUCCESS;


  if (m_Descriptor.m_sTextureAtlasDescFile.IsEmpty())
  {
    ezLog::Error("Texture atlas description file is not specified.");
    return EZ_FAILURE;
  }

  ezTextureAtlasCreationDesc atlasDesc;
  ezDynamicArray<TextureAtlasItem> atlasItems;

  if (atlasDesc.Load(m_Descriptor.m_sTextureAtlasDescFile).Failed())
  {
    ezLog::Error("Failed to load texture atlas description '{0}'", m_Descriptor.m_sTextureAtlasDescFile);
    return EZ_FAILURE;
  }

  EZ_SUCCEED_OR_RETURN(LoadAtlasInputs(atlasDesc, atlasItems));

  const ezUInt8 uiVersion = 2;
  stream << uiVersion;

  ezDdsFileFormat ddsWriter;
  ezImage atlasImg;

  bool bDebugOutput = true;

  for (ezUInt32 layerIdx = 0; layerIdx < atlasDesc.m_Layers.GetCount(); ++layerIdx)
  {
    EZ_SUCCEED_OR_RETURN(CreateAtlasLayerTexture(atlasDesc, atlasItems, layerIdx, atlasImg, 4));

    if (ddsWriter.WriteImage(stream, atlasImg, ezLog::GetThreadLocalLogSystem(), "dds").Failed())
    {
      ezLog::Error("Failed to write DDS image to texture atlas file.");
      return EZ_FAILURE;
    }
  }

  EZ_SUCCEED_OR_RETURN(WriteTextureAtlasInfo(atlasItems, atlasDesc.m_Layers.GetCount(), stream));

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::LoadAtlasInputs(const ezTextureAtlasCreationDesc& atlasDesc, ezDynamicArray<TextureAtlasItem>& items) const
{
  items.Clear();

  for (const auto& srcItem : atlasDesc.m_Items)
  {
    auto& item = items.ExpandAndGetRef();
    item.m_uiUniqueID = srcItem.m_uiUniqueID;

    for (ezUInt32 layer = 0; layer < atlasDesc.m_Layers.GetCount(); ++layer)
    {
      if (!srcItem.m_sLayerInput[layer].IsEmpty())
      {
        if (item.m_InputImage[layer].LoadFrom(srcItem.m_sLayerInput[layer]).Failed())
        {
          ezLog::Error("Failed to load texture atlas texture '{0}'", srcItem.m_sLayerInput[layer]);
          return EZ_FAILURE;
        }

        ezUInt32 uiResX = 0, uiResY = 0;
        EZ_SUCCEED_OR_RETURN(DetermineTargetResolution(item.m_InputImage[layer], ezImageFormat::UNKNOWN, uiResX, uiResY));

        EZ_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sLayerInput[layer], item.m_InputImage[layer], uiResX, uiResY));
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::WriteTextureAtlasInfo(
  const ezDynamicArray<TextureAtlasItem>& atlasItems, ezUInt32 uiNumLayers, ezStreamWriter& stream)
{
  ezTextureAtlasRuntimeDesc runtimeAtlas;
  runtimeAtlas.m_uiNumLayers = uiNumLayers;

  runtimeAtlas.m_Items.Reserve(atlasItems.GetCount());

  for (const auto& item : atlasItems)
  {
    auto& e = runtimeAtlas.m_Items[item.m_uiUniqueID];

    for (ezUInt32 l = 0; l < uiNumLayers; ++l)
    {
      e.m_LayerRects[l] = item.m_AtlasRect[l];
    }
  }

  return runtimeAtlas.Serialize(stream);
}

ezResult ezTexConvProcessor::TrySortItemsIntoAtlas(
  ezDynamicArray<TextureAtlasItem>& items, ezUInt32 uiWidth, ezUInt32 uiHeight, ezInt32 layer, ezUInt32 uiPixelAlign)
{
  ezTexturePacker packer;

  // TODO: review, currently the texture packer only works on 128 sized cells
  uiPixelAlign = 128;

  packer.SetTextureSize(uiWidth, uiHeight, items.GetCount() * 2);

  for (const auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      packer.AddTexture((item.m_InputImage[layer].GetWidth() + (uiPixelAlign - 1)) / uiPixelAlign,
        (item.m_InputImage[layer].GetHeight() + (uiPixelAlign - 1)) / uiPixelAlign);
    }
  }

  EZ_SUCCEED_OR_RETURN(packer.PackTextures());

  ezUInt32 uiTexIdx = 0;
  for (auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      const auto& tex = packer.GetTextures()[uiTexIdx++];

      item.m_AtlasRect[layer].x = tex.m_Position.x * 128;
      item.m_AtlasRect[layer].y = tex.m_Position.y * 128;
      item.m_AtlasRect[layer].width = tex.m_Size.x * 128;
      item.m_AtlasRect[layer].height = tex.m_Size.y * 128;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::SortItemsIntoAtlas(
  ezDynamicArray<TextureAtlasItem>& items, ezUInt32& out_ResX, ezUInt32& out_ResY, ezInt32 layer, ezUInt32 uiPixelAlign)
{
  for (ezUInt32 power = 8; power < 12; ++power)
  {
    const ezUInt32 halfRes = 1 << (power - 1);
    const ezUInt32 resolution = 1 << power;
    const ezUInt32 resDiv128 = resolution / 128;
    const ezUInt32 halfResDiv128 = halfRes / 128;

    if (TrySortItemsIntoAtlas(items, resDiv128, halfResDiv128, layer, uiPixelAlign).Succeeded())
    {
      out_ResX = resolution;
      out_ResY = halfRes;
      return EZ_SUCCESS;
    }

    if (TrySortItemsIntoAtlas(items, halfResDiv128, resDiv128, layer, uiPixelAlign).Succeeded())
    {
      out_ResX = halfRes;
      out_ResY = resolution;
      return EZ_SUCCESS;
    }

    if (TrySortItemsIntoAtlas(items, resDiv128, resDiv128, layer, uiPixelAlign).Succeeded())
    {
      out_ResX = resolution;
      out_ResY = resolution;
      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

ezResult ezTexConvProcessor::CreateAtlasTexture(
  ezDynamicArray<TextureAtlasItem>& items, ezUInt32 uiResX, ezUInt32 uiResY, ezImage& atlas, ezInt32 layer)
{
  ezImageHeader imgHeader;
  imgHeader.SetWidth(uiResX);
  imgHeader.SetHeight(uiResY);
  imgHeader.SetImageFormat(ezImageFormat::R32G32B32A32_FLOAT);
  atlas.ResetAndAlloc(imgHeader);

  const ezColor fill(0, 0, 0, 0);

  // make sure the target texture is filled with all black
  {
    auto pixelData = atlas.GetBlobPtr<ezUInt8>();
    ezMemoryUtils::ZeroFill(pixelData.GetPtr(), pixelData.GetCount());
  }

  for (auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      ezImage& itemImage = item.m_InputImage[layer];

      ezUInt32 uiSourceWidth = itemImage.GetWidth();
      ezUInt32 uiSourceHeight = itemImage.GetHeight();

      // fill the border of the source image with black
      {
        for (ezUInt32 y = 0; y < uiSourceHeight; ++y)
        {
          ezColor* pColor1 = itemImage.GetPixelPointer<ezColor>(0, 0, 0, 0, y);
          ezColor* pColor2 = itemImage.GetPixelPointer<ezColor>(0, 0, 0, uiSourceWidth - 1, y);

          *pColor1 = fill;
          *pColor2 = fill;
        }

        for (ezUInt32 x = 0; x < uiSourceWidth; ++x)
        {
          ezColor* pColor1 = itemImage.GetPixelPointer<ezColor>(0, 0, 0, x, 0);
          ezColor* pColor2 = itemImage.GetPixelPointer<ezColor>(0, 0, 0, x, uiSourceHeight - 1);

          *pColor1 = fill;
          *pColor2 = fill;
        }
      }

      ezRectU32 r;
      r.x = 0;
      r.y = 0;
      r.width = itemImage.GetWidth();
      r.height = itemImage.GetHeight();

      ezImageUtils::Copy(itemImage, r, atlas, ezVec3U32(item.m_AtlasRect[layer].x, item.m_AtlasRect[layer].y, 0));
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::CreateAtlasLayerTexture(const ezTextureAtlasCreationDesc& atlasDesc,
  ezDynamicArray<TextureAtlasItem>& atlasItems, ezInt32 layer, ezImage& dstImg, ezUInt32 uiNumMipmaps)
{
  EZ_ASSERT_DEV(uiNumMipmaps > 0, "Number of mipmaps to generate must be at least 1");

  const ezUInt32 uiPixelAlign = ezMath::Pow2((int)uiNumMipmaps);

  ezUInt32 uiTexWidth, uiTexHeight;
  EZ_SUCCEED_OR_RETURN(SortItemsIntoAtlas(atlasItems, uiTexWidth, uiTexHeight, layer, uiPixelAlign));

  ezLog::Success("Required Resolution for Texture Atlas: {0} x {1}", uiTexWidth, uiTexHeight);

  ezImage atlasImg;
  EZ_SUCCEED_OR_RETURN(CreateAtlasTexture(atlasItems, uiTexWidth, uiTexHeight, atlasImg, layer));

  EZ_SUCCEED_OR_RETURN(GenerateMipmaps(atlasImg, uiNumMipmaps));

  ezEnum<ezImageFormat> OutputImageFormat;

  EZ_SUCCEED_OR_RETURN(ChooseOutputFormat(OutputImageFormat, atlasDesc.m_Layers[layer].m_Usage, atlasDesc.m_Layers[layer].m_uiNumChannels));

  EZ_SUCCEED_OR_RETURN(GenerateOutput(std::move(atlasImg), dstImg, OutputImageFormat));

  return EZ_SUCCESS;
}
