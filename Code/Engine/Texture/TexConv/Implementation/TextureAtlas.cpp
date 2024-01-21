#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
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
    ezLog::Error("Failed to load texture atlas description '{0}'", ezArgSensitive(m_Descriptor.m_sTextureAtlasDescFile, "File"));
    return EZ_FAILURE;
  }

  m_Descriptor.m_uiMinResolution = ezMath::Max(32u, m_Descriptor.m_uiMinResolution);

  EZ_SUCCEED_OR_RETURN(LoadAtlasInputs(atlasDesc, atlasItems));

  const ezUInt8 uiVersion = 3;
  stream << uiVersion;

  ezDdsFileFormat ddsWriter;
  ezImage atlasImg;

  for (ezUInt32 layerIdx = 0; layerIdx < atlasDesc.m_Layers.GetCount(); ++layerIdx)
  {
    EZ_SUCCEED_OR_RETURN(CreateAtlasLayerTexture(atlasDesc, atlasItems, layerIdx, atlasImg));

    if (ddsWriter.WriteImage(stream, atlasImg, "dds").Failed())
    {
      ezLog::Error("Failed to write DDS image to texture atlas file.");
      return EZ_FAILURE;
    }

    // debug: write out atlas slices as pure DDS
    if (false)
    {
      ezStringBuilder sOut;
      sOut.SetFormat("D:/atlas_{}.dds", layerIdx);

      ezFileWriter fOut;
      if (fOut.Open(sOut).Succeeded())
      {
        EZ_SUCCEED_OR_RETURN(ddsWriter.WriteImage(fOut, atlasImg, "dds"));
      }
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
    item.m_uiFlags = srcItem.m_uiFlags;

    for (ezUInt32 layer = 0; layer < atlasDesc.m_Layers.GetCount(); ++layer)
    {
      if (!srcItem.m_sLayerInput[layer].IsEmpty())
      {
        if (item.m_InputImage[layer].LoadFrom(srcItem.m_sLayerInput[layer]).Failed())
        {
          ezLog::Error("Failed to load texture atlas texture '{0}'", ezArgSensitive(srcItem.m_sLayerInput[layer], "File"));
          return EZ_FAILURE;
        }

        if (atlasDesc.m_Layers[layer].m_Usage == ezTexConvUsage::Color)
        {
          // enforce sRGB format for all color textures
          item.m_InputImage[layer].ReinterpretAs(ezImageFormat::AsSrgb(item.m_InputImage[layer].GetImageFormat()));
        }

        ezUInt32 uiResX = 0, uiResY = 0;
        EZ_SUCCEED_OR_RETURN(DetermineTargetResolution(item.m_InputImage[layer], ezImageFormat::UNKNOWN, uiResX, uiResY));

        EZ_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sLayerInput[layer], item.m_InputImage[layer], uiResX, uiResY, atlasDesc.m_Layers[layer].m_Usage));
      }
    }


    if (!srcItem.m_sAlphaInput.IsEmpty())
    {
      ezImage alphaImg;

      if (alphaImg.LoadFrom(srcItem.m_sAlphaInput).Failed())
      {
        ezLog::Error("Failed to load texture atlas alpha mask '{0}'", srcItem.m_sAlphaInput);
        return EZ_FAILURE;
      }

      ezUInt32 uiResX = 0, uiResY = 0;
      EZ_SUCCEED_OR_RETURN(DetermineTargetResolution(alphaImg, ezImageFormat::UNKNOWN, uiResX, uiResY));

      EZ_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sAlphaInput, alphaImg, uiResX, uiResY, ezTexConvUsage::Linear));


      // layer 0 must have the exact same size as the alpha texture
      EZ_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sLayerInput[0], item.m_InputImage[0], uiResX, uiResY, ezTexConvUsage::Linear));

      // copy alpha channel into layer 0
      EZ_SUCCEED_OR_RETURN(ezImageUtils::CopyChannel(item.m_InputImage[0], 3, alphaImg, 0));

      // rescale all layers to be no larger than the alpha mask texture
      for (ezUInt32 layer = 1; layer < atlasDesc.m_Layers.GetCount(); ++layer)
      {
        if (item.m_InputImage[layer].GetWidth() <= uiResX && item.m_InputImage[layer].GetHeight() <= uiResY)
          continue;

        EZ_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sLayerInput[layer], item.m_InputImage[layer], uiResX, uiResY, ezTexConvUsage::Linear));
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::WriteTextureAtlasInfo(const ezDynamicArray<TextureAtlasItem>& atlasItems, ezUInt32 uiNumLayers, ezStreamWriter& stream)
{
  ezTextureAtlasRuntimeDesc runtimeAtlas;
  runtimeAtlas.m_uiNumLayers = uiNumLayers;

  runtimeAtlas.m_Items.Reserve(atlasItems.GetCount());

  for (const auto& item : atlasItems)
  {
    auto& e = runtimeAtlas.m_Items[item.m_uiUniqueID];
    e.m_uiFlags = item.m_uiFlags;

    for (ezUInt32 l = 0; l < uiNumLayers; ++l)
    {
      e.m_LayerRects[l] = item.m_AtlasRect[l];
    }
  }

  return runtimeAtlas.Serialize(stream);
}

constexpr ezUInt32 uiAtlasCellSize = 32;

ezResult ezTexConvProcessor::TrySortItemsIntoAtlas(ezDynamicArray<TextureAtlasItem>& items, ezUInt32 uiWidth, ezUInt32 uiHeight, ezInt32 layer)
{
  ezTexturePacker packer;

  // TODO: review, currently the texture packer only works on 32 sized cells
  ezUInt32 uiPixelAlign = uiAtlasCellSize;

  packer.SetTextureSize(uiWidth, uiHeight, items.GetCount() * 2);

  for (const auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      packer.AddTexture((item.m_InputImage[layer].GetWidth() + (uiPixelAlign - 1)) / uiPixelAlign, (item.m_InputImage[layer].GetHeight() + (uiPixelAlign - 1)) / uiPixelAlign);
    }
  }

  EZ_SUCCEED_OR_RETURN(packer.PackTextures());

  ezUInt32 uiTexIdx = 0;
  for (auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      const auto& tex = packer.GetTextures()[uiTexIdx++];

      item.m_AtlasRect[layer].x = tex.m_Position.x * uiAtlasCellSize;
      item.m_AtlasRect[layer].y = tex.m_Position.y * uiAtlasCellSize;
      item.m_AtlasRect[layer].width = tex.m_Size.x * uiAtlasCellSize;
      item.m_AtlasRect[layer].height = tex.m_Size.y * uiAtlasCellSize;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::SortItemsIntoAtlas(ezDynamicArray<TextureAtlasItem>& items, ezUInt32& out_ResX, ezUInt32& out_ResY, ezInt32 layer)
{
  for (ezUInt32 power = 8; power < 14; ++power)
  {
    const ezUInt32 halfRes = 1 << (power - 1);
    const ezUInt32 resolution = 1 << power;
    const ezUInt32 resDivCellSize = resolution / uiAtlasCellSize;
    const ezUInt32 halfResDivCellSize = halfRes / uiAtlasCellSize;

    if (TrySortItemsIntoAtlas(items, resDivCellSize, halfResDivCellSize, layer).Succeeded())
    {
      out_ResX = resolution;
      out_ResY = halfRes;
      return EZ_SUCCESS;
    }

    if (TrySortItemsIntoAtlas(items, halfResDivCellSize, resDivCellSize, layer).Succeeded())
    {
      out_ResX = halfRes;
      out_ResY = resolution;
      return EZ_SUCCESS;
    }

    if (TrySortItemsIntoAtlas(items, resDivCellSize, resDivCellSize, layer).Succeeded())
    {
      out_ResX = resolution;
      out_ResY = resolution;
      return EZ_SUCCESS;
    }
  }

  ezLog::Error("Could not sort items into texture atlas. Too many too large textures.");
  return EZ_FAILURE;
}

ezResult ezTexConvProcessor::CreateAtlasTexture(ezDynamicArray<TextureAtlasItem>& items, ezUInt32 uiResX, ezUInt32 uiResY, ezImage& atlas, ezInt32 layer)
{
  ezImageHeader imgHeader;
  imgHeader.SetWidth(uiResX);
  imgHeader.SetHeight(uiResY);
  imgHeader.SetImageFormat(ezImageFormat::R32G32B32A32_FLOAT);
  atlas.ResetAndAlloc(imgHeader);

  // make sure the target texture is filled with all black
  {
    auto pixelData = atlas.GetBlobPtr<ezUInt8>();
    ezMemoryUtils::ZeroFill(pixelData.GetPtr(), static_cast<size_t>(pixelData.GetCount()));
  }

  for (auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      ezImage& itemImage = item.m_InputImage[layer];

      ezRectU32 r;
      r.x = 0;
      r.y = 0;
      r.width = itemImage.GetWidth();
      r.height = itemImage.GetHeight();

      EZ_SUCCEED_OR_RETURN(ezImageUtils::Copy(itemImage, r, atlas, ezVec3U32(item.m_AtlasRect[layer].x, item.m_AtlasRect[layer].y, 0)));
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::FillAtlasBorders(ezDynamicArray<TextureAtlasItem>& items, ezImage& atlas, ezInt32 layer)
{
  const ezUInt32 uiBorderPixels = 2;

  const ezUInt32 uiNumMipmaps = atlas.GetHeader().GetNumMipLevels();
  for (ezUInt32 uiMipLevel = 0; uiMipLevel < uiNumMipmaps; ++uiMipLevel)
  {
    for (auto& item : items)
    {
      if (!item.m_InputImage[layer].IsValid())
        continue;

      ezRectU32& itemRect = item.m_AtlasRect[layer];
      const ezUInt32 uiRectX = itemRect.x >> uiMipLevel;
      const ezUInt32 uiRectY = itemRect.y >> uiMipLevel;
      const ezUInt32 uiWidth = ezMath::Max(1u, itemRect.width >> uiMipLevel);
      const ezUInt32 uiHeight = ezMath::Max(1u, itemRect.height >> uiMipLevel);

      // fill the border of the item rect with alpha 0 to prevent bleeding into other decals in the atlas
      if (uiWidth <= 2 * uiBorderPixels || uiHeight <= 2 * uiBorderPixels)
      {
        for (ezUInt32 y = 0; y < uiHeight; ++y)
        {
          for (ezUInt32 x = 0; x < uiWidth; ++x)
          {
            const ezUInt32 xClamped = ezMath::Min(uiRectX + x, atlas.GetWidth(uiMipLevel));
            const ezUInt32 yClamped = ezMath::Min(uiRectY + y, atlas.GetHeight(uiMipLevel));
            atlas.GetPixelPointer<ezColor>(uiMipLevel, 0, 0, xClamped, yClamped)->a = 0.0f;
          }
        }
      }
      else
      {
        for (ezUInt32 i = 0; i < uiBorderPixels; ++i)
        {
          for (ezUInt32 y = 0; y < uiHeight; ++y)
          {
            atlas.GetPixelPointer<ezColor>(uiMipLevel, 0, 0, uiRectX + i, uiRectY + y)->a = 0.0f;
            atlas.GetPixelPointer<ezColor>(uiMipLevel, 0, 0, uiRectX + uiWidth - 1 - i, uiRectY + y)->a = 0.0f;
          }

          for (ezUInt32 x = 0; x < uiWidth; ++x)
          {
            atlas.GetPixelPointer<ezColor>(uiMipLevel, 0, 0, uiRectX + x, uiRectY + i)->a = 0.0f;
            atlas.GetPixelPointer<ezColor>(uiMipLevel, 0, 0, uiRectX + x, uiRectY + uiHeight - 1 - i)->a = 0.0f;
          }
        }
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::CreateAtlasLayerTexture(const ezTextureAtlasCreationDesc& atlasDesc, ezDynamicArray<TextureAtlasItem>& atlasItems, ezInt32 layer, ezImage& dstImg)
{
  ezUInt32 uiTexWidth, uiTexHeight;
  EZ_SUCCEED_OR_RETURN(SortItemsIntoAtlas(atlasItems, uiTexWidth, uiTexHeight, layer));

  ezLog::Success("Required Resolution for Texture Atlas: {0} x {1}", uiTexWidth, uiTexHeight);

  ezImage atlasImg;
  EZ_SUCCEED_OR_RETURN(CreateAtlasTexture(atlasItems, uiTexWidth, uiTexHeight, atlasImg, layer));

  ezUInt32 uiNumMipmaps = atlasImg.GetHeader().ComputeNumberOfMipMaps();
  EZ_SUCCEED_OR_RETURN(GenerateMipmaps(atlasImg, uiNumMipmaps));

  if (atlasDesc.m_Layers[layer].m_uiNumChannels == 4)
  {
    EZ_SUCCEED_OR_RETURN(FillAtlasBorders(atlasItems, atlasImg, layer));
  }

  ezEnum<ezImageFormat> OutputImageFormat;

  EZ_SUCCEED_OR_RETURN(ChooseOutputFormat(OutputImageFormat, atlasDesc.m_Layers[layer].m_Usage, atlasDesc.m_Layers[layer].m_uiNumChannels));

  EZ_SUCCEED_OR_RETURN(GenerateOutput(std::move(atlasImg), dstImg, OutputImageFormat));

  return EZ_SUCCESS;
}


