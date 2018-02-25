#include "Main.h"
#include <Foundation/Image/ImageUtils.h>
#include <Utilities/Textures/TextureGroupDesc.h>
#include <Foundation/Math/Rect.h>
#include <Utilities/Textures/TexturePacker.h>
#include <Foundation/Strings/HashedString.h>

ezResult ezTexConv::CreateDecalLayerTexture(ezDynamicArray<DecalDesc>& decals, ezInt32 layer, ezStreamWriter& stream)
{
  ezUInt32 uiTexWidth, uiTexHeight;
  EZ_SUCCEED_OR_RETURN(SortDecalsIntoAtlas(decals, uiTexWidth, uiTexHeight, layer));

  ezLog::Success("Required Resolution for Decal Atlas: {0} x {1}", uiTexWidth, uiTexHeight);

  ezImage atlas;
  EZ_SUCCEED_OR_RETURN(CreateDecalAtlasTexture(decals, uiTexWidth, uiTexHeight, atlas, layer));

  // stores result in m_pCurrentImage
  EZ_SUCCEED_OR_RETURN(CreateTexture2D(&atlas, false));

  m_bGeneratedMipmaps = true;
  EZ_SUCCEED_OR_RETURN(GenerateMipmaps());

  m_bCompress = true;
  m_bHDROutput = false;
  m_uiOutputChannels = 4;
  m_bPremultiplyAlpha = false;
  m_bFlipHorizontal = false;
  EZ_SUCCEED_OR_RETURN(ConvertToOutputFormat());

  EZ_SUCCEED_OR_RETURN(SaveResultToDDS(stream));

  return EZ_SUCCESS;
}

ezResult ezTexConv::CreateDecalAtlas()
{
  ezTextureGroupDesc decalAtlasDesc;

  if (decalAtlasDesc.Load(m_InputFileNames[0]).Failed())
  {
    ezLog::Error("Failed to load decal atlas description '{0}'", m_InputFileNames[0]);
    SetReturnCode(TexConvReturnCodes::FAILED_LOAD_INPUTS);
    return EZ_FAILURE;
  }

  ezDynamicArray<DecalDesc> decals;
  decals.Reserve(decalAtlasDesc.m_Groups.GetCount());

  EZ_SUCCEED_OR_RETURN(LoadDecalInputs(decalAtlasDesc, decals));

  {
    ezAssetFileHeader header;
    header.SetFileHashAndVersion(m_uiAssetHash, m_uiAssetVersion);
    header.Write(m_FileOut);
  }

  const ezUInt8 uiVersion = 1;
  m_FileOut << uiVersion;

  EZ_SUCCEED_OR_RETURN(CreateDecalLayerTexture(decals, DecalLayer::BaseColor, m_FileOut));
  EZ_SUCCEED_OR_RETURN(CreateDecalLayerTexture(decals, DecalLayer::Normal, m_FileOut));

  WriteDecalAtlasInfo(decals);

  SetReturnCode(TexConvReturnCodes::OK);
  return EZ_SUCCESS;
}

ezResult ezTexConv::LoadDecalInputs(ezTextureGroupDesc &decalAtlasDesc, ezDynamicArray<DecalDesc> &decals)
{
  for (const auto& group : decalAtlasDesc.m_Groups)
  {
    auto& decal = decals.ExpandAndGetRef();
    decal.m_sIdentifier = group.m_sFilepaths[0];

    for (ezInt32 layer = 0; layer < DecalLayer::ENUM_COUNT; ++layer)
    {
      decal.m_sFile[layer] = group.m_sFilepaths[1 + layer];

      if (!decal.m_sFile[layer].IsEmpty())
      {
        if (decal.m_Image[layer].LoadFrom(decal.m_sFile[layer]).Failed())
        {
          ezLog::Error("Failed to load decal texture '{0}'", decal.m_sFile[layer]);
          SetReturnCode(TexConvReturnCodes::FAILED_LOAD_INPUTS);
          return EZ_FAILURE;
        }
      }
    }
  }

  return EZ_SUCCESS;
}


ezResult ezTexConv::TrySortDecalsIntoAtlas(ezDynamicArray<DecalDesc> &decals, ezUInt32 uiWidth, ezUInt32 uiHeight, ezInt32 layer)
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


ezResult ezTexConv::SortDecalsIntoAtlas(ezDynamicArray<DecalDesc> &decals, ezUInt32& out_ResX, ezUInt32& out_ResY, ezInt32 layer)
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


ezResult ezTexConv::ToFloatImage(const ezImage& src, ezImage& dst)
{
  if (ezImageConversion::Convert(src, dst, ezImageFormat::R32G32B32A32_FLOAT).Failed())
  {
    SetReturnCode(TexConvReturnCodes::FAILED_CONVERT_INPUT_TO_RGBA);
    ezLog::Error("Failed to convert image from format {0} to R32G32B32A32_FLOAT. Format is not supported.", ezImageFormat::GetName(src.GetImageFormat()));
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv::CreateDecalAtlasTexture(const ezDynamicArray<DecalDesc>& decals, ezUInt32 uiResX, ezUInt32 uiResY, ezImage& atlas, ezInt32 layer)
{
  atlas.SetWidth(uiResX);
  atlas.SetHeight(uiResY);
  atlas.SetImageFormat(ezImageFormat::R32G32B32A32_FLOAT);
  atlas.AllocateImageData();

  const ezColor fill(0, 0, 0, 0);

  // make sure the target texture is filled with all black
  for (ezUInt32 y = 0; y < uiResY; ++y)
  {
    for (ezUInt32 x = 0; x < uiResX; ++x)
    {
      ezColor* pColor = atlas.GetPixelPointer<ezColor>(0, 0, 0, x, y);
      *pColor = fill;
    }
  }


  for (const auto& decal : decals)
  {
    if (!decal.m_sFile[layer].IsEmpty())
    {
      ezImage decalImage;
      EZ_SUCCEED_OR_RETURN(ToFloatImage(decal.m_Image[layer], decalImage));

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


void ezTexConv::WriteDecalAtlasInfo(ezDynamicArray<DecalDesc> decals)
{
  m_FileOut << decals.GetCount();

  for (const auto& decal : decals)
  {
    const ezUInt32 uiHash = ezTempHashedString::ComputeHash(decal.m_sIdentifier.GetData());

    m_FileOut << decal.m_sIdentifier;
    m_FileOut << uiHash;

    for (ezInt32 layer = 0; layer < DecalLayer::ENUM_COUNT; ++layer)
    {
      m_FileOut << decal.m_Rect[layer].x;
      m_FileOut << decal.m_Rect[layer].y;
      m_FileOut << decal.m_Rect[layer].width;
      m_FileOut << decal.m_Rect[layer].height;
    }
  }
}

