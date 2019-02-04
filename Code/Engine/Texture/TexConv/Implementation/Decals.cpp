#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>
#include <Texture/Utils/TextureGroupDesc.h>
#include <Texture/Utils/TexturePacker.h>

ezResult ezTexConvProcessor::GenerateDecalAtlas()
{
  if (m_Descriptor.m_OutputType != ezTexConvOutputType::DecalAtlas)
    return EZ_SUCCESS;

  if (m_Descriptor.m_sDecalAtlasDescFile.IsEmpty())
  {
    ezLog::Error("Decal atlas description file is not specified.");
    return EZ_FAILURE;
  }

  ezTextureGroupDesc decalAtlasDesc;

  if (decalAtlasDesc.Load(m_Descriptor.m_sDecalAtlasDescFile).Failed())
  {
    ezLog::Error("Failed to load decal atlas description '{0}'", m_Descriptor.m_sDecalAtlasDescFile);
    return EZ_FAILURE;
  }

  m_Decals.Reserve(decalAtlasDesc.m_Groups.GetCount());

  EZ_SUCCEED_OR_RETURN(LoadDecalInputs(decalAtlasDesc));

  ezMemoryStreamWriter DecalOutput(&m_DecalAtlas);

  {
    ezAssetFileHeader header;
    header.SetFileHashAndVersion(m_Descriptor.m_uiAssetHash, m_Descriptor.m_uiAssetVersion);
    header.Write(DecalOutput);
  }

  const ezUInt8 uiVersion = 1;
  DecalOutput << uiVersion;

  EZ_SUCCEED_OR_RETURN(CreateDecalLayerTexture(DecalLayer::BaseColor, DecalOutput));
  EZ_SUCCEED_OR_RETURN(CreateDecalLayerTexture(DecalLayer::Normal, DecalOutput));

  EZ_SUCCEED_OR_RETURN(WriteDecalAtlasInfo(m_Decals, DecalOutput));

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::LoadDecalInputs(ezTextureGroupDesc& decalAtlasDesc)
{
  for (const auto& group : decalAtlasDesc.m_Groups)
  {
    auto& decal = m_Decals.ExpandAndGetRef();
    decal.m_sIdentifier = group.m_sFilepaths[0];

    for (ezInt32 layer = 0; layer < DecalLayer::ENUM_COUNT; ++layer)
    {
      decal.m_sFile[layer] = group.m_sFilepaths[1 + layer];

      if (!decal.m_sFile[layer].IsEmpty())
      {
        if (decal.m_Image[layer].LoadFrom(decal.m_sFile[layer]).Failed())
        {
          ezLog::Error("Failed to load decal texture '{0}'", decal.m_sFile[layer]);
          return EZ_FAILURE;
        }

        if (decal.m_Image[layer].Convert(ezImageFormat::R32G32B32A32_FLOAT).Failed())
        {
          ezLog::Error("Failed to convert decal texture '{0}' to RGBA 32 Float", decal.m_sFile[layer]);
          return EZ_FAILURE;
        }
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::WriteDecalAtlasInfo(ezDynamicArray<DecalDesc>& decals, ezStreamWriter& stream)
{
  stream << decals.GetCount();

  for (const auto& decal : decals)
  {
    const ezUInt32 uiHash = ezHashingUtils::xxHash32(decal.m_sIdentifier.GetData(), decal.m_sIdentifier.GetElementCount());

    stream << decal.m_sIdentifier;
    stream << uiHash;

    for (ezInt32 layer = 0; layer < DecalLayer::ENUM_COUNT; ++layer)
    {
      stream << decal.m_Rect[layer].x;
      stream << decal.m_Rect[layer].y;
      stream << decal.m_Rect[layer].width;
      stream << decal.m_Rect[layer].height;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::TrySortDecalsIntoAtlas(ezDynamicArray<DecalDesc>& decals, ezUInt32 uiWidth, ezUInt32 uiHeight, ezInt32 layer)
{
  ezTexturePacker packer;

  packer.SetTextureSize(uiWidth, uiHeight, decals.GetCount() * 2);

  for (const auto& decal : decals)
  {
    if (!decal.m_sFile[layer].IsEmpty())
    {
      packer.AddTexture((decal.m_Image[layer].GetWidth() + 127) / 128, (decal.m_Image[layer].GetHeight() + 127) / 128);
    }
  }

  EZ_SUCCEED_OR_RETURN(packer.PackTextures());

  ezUInt32 uiTexIdx = 0;
  for (auto& decal : decals)
  {
    if (!decal.m_sFile[layer].IsEmpty())
    {
      const auto& tex = packer.GetTextures()[uiTexIdx++];

      decal.m_Rect[layer].x = tex.m_Position.x * 128;
      decal.m_Rect[layer].y = tex.m_Position.y * 128;
      decal.m_Rect[layer].width = tex.m_Size.x * 128;
      decal.m_Rect[layer].height = tex.m_Size.y * 128;
    }
  }

  return EZ_SUCCESS;
}


ezResult ezTexConvProcessor::SortDecalsIntoAtlas(ezDynamicArray<DecalDesc>& decals, ezUInt32& out_ResX, ezUInt32& out_ResY, ezInt32 layer)
{
  for (ezUInt32 power = 8; power < 12; ++power)
  {
    const ezUInt32 halfRes = 1 << (power - 1);
    const ezUInt32 resolution = 1 << power;
    const ezUInt32 resDiv128 = resolution / 128;
    const ezUInt32 halfResDiv128 = halfRes / 128;

    if (TrySortDecalsIntoAtlas(decals, resDiv128, halfResDiv128, layer).Succeeded())
    {
      out_ResX = resolution;
      out_ResY = halfRes;
      return EZ_SUCCESS;
    }

    if (TrySortDecalsIntoAtlas(decals, halfResDiv128, resDiv128, layer).Succeeded())
    {
      out_ResX = halfRes;
      out_ResY = resolution;
      return EZ_SUCCESS;
    }

    if (TrySortDecalsIntoAtlas(decals, resDiv128, resDiv128, layer).Succeeded())
    {
      out_ResX = resolution;
      out_ResY = resolution;
      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

ezResult ezTexConvProcessor::CreateDecalAtlasTexture(
  ezDynamicArray<DecalDesc>& decals, ezUInt32 uiResX, ezUInt32 uiResY, ezImage& atlas, ezInt32 layer)
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

  for (auto& decal : decals)
  {
    if (!decal.m_sFile[layer].IsEmpty())
    {
      ezImage& decalImage = decal.m_Image[layer];

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

      ezImageUtils::Copy(atlas, decal.m_Rect[layer].x, decal.m_Rect[layer].y, decalImage);
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::CreateDecalLayerTexture(ezInt32 layer, ezStreamWriter& stream)
{
  ezUInt32 uiTexWidth, uiTexHeight;
  EZ_SUCCEED_OR_RETURN(SortDecalsIntoAtlas(m_Decals, uiTexWidth, uiTexHeight, layer));

  ezLog::Success("Required Resolution for Decal Atlas: {0} x {1}", uiTexWidth, uiTexHeight);

  EZ_SUCCEED_OR_RETURN(CreateDecalAtlasTexture(m_Decals, uiTexWidth, uiTexHeight, *m_pCurrentScratchImage, layer));

  EZ_SUCCEED_OR_RETURN(GenerateMipmaps());

  switch (layer)
  {
    case DecalLayer::BaseColor:
      m_Descriptor.m_Usage = ezTexConvUsage::Color;
      m_uiNumChannels = 4;
      break;

    case DecalLayer::Normal:
      m_Descriptor.m_Usage = ezTexConvUsage::NormalMap;
      m_uiNumChannels = 2;
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return EZ_FAILURE;
  }

  m_OutputImageFormat = ezImageFormat::UNKNOWN;
  EZ_SUCCEED_OR_RETURN(ChooseOutputFormat());

  EZ_SUCCEED_OR_RETURN(GenerateOutput());

  ezDdsFileFormat ddsWriter;
  if (ddsWriter.WriteImage(stream, m_OutputImage, ezLog::GetThreadLocalLogSystem(), "dds").Failed())
  {
    ezLog::Error("Failed to write DDS image to decal atlas file.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}
