#include "Main.h"
#include <Foundation/Image/ImageUtils.h>
#include <Utilities/Textures/TextureGroupDesc.h>
#include <Foundation/Math/Rect.h>
#include <Utilities/Textures/TexturePacker.h>

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

  //if (layer == DecalLayer::Diffuse)
  //{
  //  m_sThumbnailFile = "D:/test.jpg";
  //  EZ_SUCCEED_OR_RETURN(SaveThumbnail());
  //}

  m_bCompress = true;
  m_bHDROutput = false;
  m_uiOutputChannels = 4;
  m_bPremultiplyAlpha = false;
  EZ_SUCCEED_OR_RETURN(ConvertToOutputFormat());

  EZ_SUCCEED_OR_RETURN(SaveResultToDDS(stream));

  return EZ_SUCCESS;
}

ezResult ezTexConv::PrepareDecalOutputFiles(ezFileWriter* files)
{
  const ezStringBuilder sFileName = ezPathUtils::GetFileName(m_sOutputFile);

  ezStringBuilder sNewFileName;
  ezStringBuilder sFile;

  {
    sNewFileName.Set(sFileName, "_N");
    sFile = m_sOutputFile;
    sFile.ChangeFileName(sNewFileName);

    if (files[1].Open(sFile).Failed())
    {
      SetReturnCode(TexConvReturnCodes::FAILED_WRITE_OUTPUT);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv::CreateDecalAtlas()
{
  ezTextureGroupDesc decalAtlasDesc;

  ezFileWriter outputFiles[2];
  EZ_SUCCEED_OR_RETURN(PrepareDecalOutputFiles(outputFiles));

  if (decalAtlasDesc.Load(m_InputFileNames[0]).Failed())
  {
    ezLog::Error("Failed to load decal atlas description '{0}'", m_InputFileNames[0]);
    SetReturnCode(TexConvReturnCodes::FAILED_LOAD_INPUTS);
    return EZ_FAILURE;
  }

  ezDynamicArray<DecalDesc> decals;
  decals.Reserve(decalAtlasDesc.m_Groups.GetCount());

  EZ_SUCCEED_OR_RETURN(LoadDecalInputs(decalAtlasDesc, decals));

  EZ_SUCCEED_OR_RETURN(CreateDecalLayerTexture(decals, DecalLayer::Diffuse, m_FileOut)); // m_FileOut is the diffuse file
  EZ_SUCCEED_OR_RETURN(CreateDecalLayerTexture(decals, DecalLayer::Normal, outputFiles[DecalLayer::Normal]));

  SetReturnCode(TexConvReturnCodes::OK);
  return EZ_SUCCESS;
}

ezResult ezTexConv::LoadDecalInputs(ezTextureGroupDesc &decalAtlasDesc, ezDynamicArray<DecalDesc> &decals)
{
  for (const auto& group : decalAtlasDesc.m_Groups)
  {
    auto& decal = decals.ExpandAndGetRef();

    for (ezInt32 layer = 0; layer < DecalLayer::ENUM_COUNT; ++layer)
    {
      decal.m_sFile[layer] = group.m_sFilepaths[layer];

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

  for (const auto& decal : decals)
  {
    if (!decal.m_sFile[layer].IsEmpty())
    {
      ezImage img32;
      EZ_SUCCEED_OR_RETURN(ToFloatImage(decal.m_Image[layer], img32));

      ezImageUtils::Copy(atlas, decal.m_Rect[layer].x, decal.m_Rect[layer].y, img32);
    }
  }

  return EZ_SUCCESS;
}

