#include <PCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>
#include <Texture/Utils/TextureGroupDesc.h>
#include <Texture/Utils/TexturePacker.h>

ezResult ezTexConvProcessor::LoadAtlasInputs(const ezTextureGroupDesc& atlasDesc, ezDynamicArray<TextureAtlasItem>& items) const
{
  for (const auto& group : atlasDesc.m_Groups)
  {
    auto& item = items.ExpandAndGetRef();
    item.m_sIdentifier = group.m_sFilepaths[0];

    for (ezInt32 layer = 0; layer < TextureAtlasLayer::ENUM_COUNT; ++layer)
    {
      item.m_sInputFile[layer] = group.m_sFilepaths[1 + layer];

      if (!item.m_sInputFile[layer].IsEmpty())
      {
        if (item.m_InputImage[layer].LoadFrom(item.m_sInputFile[layer]).Failed())
        {
          ezLog::Error("Failed to load texture atlas texture '{0}'", item.m_sInputFile[layer]);
          return EZ_FAILURE;
        }

        ezUInt32 uiResX = 0, uiResY = 0;
        EZ_SUCCEED_OR_RETURN(DetermineTargetResolution(item.m_InputImage[layer], ezImageFormat::UNKNOWN, uiResX, uiResY));

        EZ_SUCCEED_OR_RETURN(ConvertAndScaleImage(item.m_sInputFile[layer], item.m_InputImage[layer], uiResX, uiResY));
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::WriteTextureAtlasInfo(const ezDynamicArray<TextureAtlasItem>& items, ezStreamWriter& stream)
{
  stream << items.GetCount();

  for (const auto& item : items)
  {
    const ezUInt32 uiHash = ezHashingUtils::xxHash32(item.m_sIdentifier.GetData(), item.m_sIdentifier.GetElementCount());

    stream << item.m_sIdentifier;
    stream << uiHash;

    for (ezInt32 layer = 0; layer < TextureAtlasLayer::ENUM_COUNT; ++layer)
    {
      stream << item.m_AtlasRect[layer].x;
      stream << item.m_AtlasRect[layer].y;
      stream << item.m_AtlasRect[layer].width;
      stream << item.m_AtlasRect[layer].height;
    }
  }

  return EZ_SUCCESS;
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
    if (!item.m_sInputFile[layer].IsEmpty())
    {
      packer.AddTexture((item.m_InputImage[layer].GetWidth() + (uiPixelAlign - 1)) / uiPixelAlign,
        (item.m_InputImage[layer].GetHeight() + (uiPixelAlign - 1)) / uiPixelAlign);
    }
  }

  EZ_SUCCEED_OR_RETURN(packer.PackTextures());

  ezUInt32 uiTexIdx = 0;
  for (auto& item : items)
  {
    if (!item.m_sInputFile[layer].IsEmpty())
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
  ezDynamicArray<TextureAtlasItem>& decals, ezUInt32& out_ResX, ezUInt32& out_ResY, ezInt32 layer, ezUInt32 uiPixelAlign)
{
  for (ezUInt32 power = 8; power < 12; ++power)
  {
    const ezUInt32 halfRes = 1 << (power - 1);
    const ezUInt32 resolution = 1 << power;
    const ezUInt32 resDiv128 = resolution / 128;
    const ezUInt32 halfResDiv128 = halfRes / 128;

    if (TrySortItemsIntoAtlas(decals, resDiv128, halfResDiv128, layer, uiPixelAlign).Succeeded())
    {
      out_ResX = resolution;
      out_ResY = halfRes;
      return EZ_SUCCESS;
    }

    if (TrySortItemsIntoAtlas(decals, halfResDiv128, resDiv128, layer, uiPixelAlign).Succeeded())
    {
      out_ResX = halfRes;
      out_ResY = resolution;
      return EZ_SUCCESS;
    }

    if (TrySortItemsIntoAtlas(decals, resDiv128, resDiv128, layer, uiPixelAlign).Succeeded())
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
    auto pixelData = atlas.GetArrayPtr<ezUInt8>();
    ezMemoryUtils::ZeroFill(pixelData.GetPtr(), pixelData.GetCount());
  }

  for (auto& item : items)
  {
    if (!item.m_sInputFile[layer].IsEmpty())
    {
      ezImage& decalImage = item.m_InputImage[layer];

      ezUInt32 uiSourceWidth = decalImage.GetWidth();
      ezUInt32 uiSourceHeight = decalImage.GetHeight();

      // fill the border of the source image with black
      {
        for (ezUInt32 y = 0; y < uiSourceHeight; ++y)
        {
          ezColor* pColor1 = decalImage.GetPixelPointer<ezColor>(0, 0, 0, 0, y);
          ezColor* pColor2 = decalImage.GetPixelPointer<ezColor>(0, 0, 0, uiSourceWidth - 1, y);

          *pColor1 = fill;
          *pColor2 = fill;
        }

        for (ezUInt32 x = 0; x < uiSourceWidth; ++x)
        {
          ezColor* pColor1 = decalImage.GetPixelPointer<ezColor>(0, 0, 0, x, 0);
          ezColor* pColor2 = decalImage.GetPixelPointer<ezColor>(0, 0, 0, x, uiSourceHeight - 1);

          *pColor1 = fill;
          *pColor2 = fill;
        }
      }

      ezImageUtils::Copy(atlas, item.m_AtlasRect[layer].x, item.m_AtlasRect[layer].y, decalImage);
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::CreateAtlasLayerTexture(
  ezDynamicArray<TextureAtlasItem>& atlasItems, ezInt32 layer, ezImage& dstImg, ezUInt32 uiNumMipmaps)
{
  EZ_ASSERT_DEV(uiNumMipmaps > 0, "Number of mipmaps to generate must be at least 1");

  const ezUInt32 uiPixelAlign = ezMath::Pow2((int)uiNumMipmaps);

  ezUInt32 uiTexWidth, uiTexHeight;
  EZ_SUCCEED_OR_RETURN(SortItemsIntoAtlas(atlasItems, uiTexWidth, uiTexHeight, layer, uiPixelAlign));

  ezLog::Success("Required Resolution for Decal Atlas: {0} x {1}", uiTexWidth, uiTexHeight);

  ezImage atlasImg;
  EZ_SUCCEED_OR_RETURN(CreateAtlasTexture(atlasItems, uiTexWidth, uiTexHeight, atlasImg, layer));

  EZ_SUCCEED_OR_RETURN(GenerateMipmaps(atlasImg, uiNumMipmaps));

  ezEnum<ezImageFormat> OutputImageFormat;

  switch (layer)
  {
    case TextureAtlasLayer::BaseColor:
      EZ_SUCCEED_OR_RETURN(ChooseOutputFormat(OutputImageFormat, ezTexConvUsage::Color, 4));
      break;

    case TextureAtlasLayer::Normal:
      EZ_SUCCEED_OR_RETURN(ChooseOutputFormat(OutputImageFormat, ezTexConvUsage::NormalMap, 2));
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return EZ_FAILURE;
  }

  EZ_SUCCEED_OR_RETURN(GenerateOutput(std::move(atlasImg), dstImg, OutputImageFormat));

  return EZ_SUCCESS;
}
