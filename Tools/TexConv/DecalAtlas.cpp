#include "Main.h"
#include <Foundation/Image/ImageUtils.h>
#include <Utilities/Textures/TextureGroupDesc.h>
#include <Foundation/Math/Rect.h>
#include <Utilities/Textures/TexturePacker.h>

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

  ezUInt32 uiTexWidth, uiTexHeight;
  EZ_SUCCEED_OR_RETURN(SortDecalsIntoAtlas(decals, uiTexWidth, uiTexHeight));

  ezLog::Success("Required Resolution for Decal Atlas: {0} x {1}", uiTexWidth, uiTexHeight);

  ezImage atlas;
  EZ_SUCCEED_OR_RETURN(CreateDecalAtlasTexture(decals, uiTexWidth, uiTexHeight, atlas));

  // stores result in m_pCurrentImage 
  EZ_SUCCEED_OR_RETURN(CreateTexture2D(&atlas, false));

  m_bGeneratedMipmaps = true;
  EZ_SUCCEED_OR_RETURN(GenerateMipmaps());

  m_sThumbnailFile = "D:/test.jpg";
  //EZ_SUCCEED_OR_RETURN(SaveThumbnail());

  m_bCompress = true;
  m_bHDROutput = false;
  m_uiOutputChannels = 4;
  m_bPremultiplyAlpha = false;
  EZ_SUCCEED_OR_RETURN(ConvertToOutputFormat());

  EZ_SUCCEED_OR_RETURN(SaveResultToDDS());

  SetReturnCode(TexConvReturnCodes::OK);
  return EZ_SUCCESS;
}

ezResult ezTexConv::LoadDecalInputs(ezTextureGroupDesc &decalAtlasDesc, ezDynamicArray<DecalDesc> &decals)
{
  for (const auto& group : decalAtlasDesc.m_Groups)
  {
    auto& decal = decals.ExpandAndGetRef();

    decal.m_sDiffuse = group.m_sFilepaths[0];
    decal.m_sNormal = group.m_sFilepaths[1];

    if (!decal.m_sDiffuse.IsEmpty())
    {
      if (decal.m_DiffuseImg.LoadFrom(decal.m_sDiffuse).Failed())
      {
        ezLog::Error("Failed to load decal texture '{0}'", decal.m_sDiffuse);
        SetReturnCode(TexConvReturnCodes::FAILED_LOAD_INPUTS);
        return EZ_FAILURE;
      }
    }

    if (!decal.m_sNormal.IsEmpty())
    {
      if (decal.m_NormalImg.LoadFrom(decal.m_sNormal).Failed())
      {
        ezLog::Error("Failed to load decal texture '{0}'", decal.m_sNormal);
        SetReturnCode(TexConvReturnCodes::FAILED_LOAD_INPUTS);
        return EZ_FAILURE;
      }
    }
  }

  return EZ_SUCCESS;
}


ezResult ezTexConv::TrySortDecalsIntoAtlas(ezDynamicArray<DecalDesc> &decals, ezUInt32 uiWidth, ezUInt32 uiHeight)
{
  ezTexturePacker packer;

  packer.SetTextureSize(uiWidth, uiHeight, decals.GetCount() * 2);

  for (const auto& decal : decals)
  {
    if (!decal.m_sDiffuse.IsEmpty())
    {
      packer.AddTexture((decal.m_DiffuseImg.GetWidth() + 127) / 128, (decal.m_DiffuseImg.GetHeight() + 127) / 128);
    }

    if (!decal.m_sNormal.IsEmpty())
    {
      packer.AddTexture((decal.m_NormalImg.GetWidth() + 127) / 128, (decal.m_NormalImg.GetHeight() + 127) / 128);
    }
  }

  EZ_SUCCEED_OR_RETURN(packer.PackTextures());

  ezUInt32 uiTexIdx = 0;
  for (auto& decal : decals)
  {
    if (!decal.m_sDiffuse.IsEmpty())
    {
      const auto& tex = packer.GetTextures()[uiTexIdx++];

      decal.m_DiffuseRect.x = tex.m_Position.x * 128;
      decal.m_DiffuseRect.y = tex.m_Position.y * 128;
      decal.m_DiffuseRect.width = tex.m_Size.x * 128;
      decal.m_DiffuseRect.height = tex.m_Size.y * 128;
    }

    if (!decal.m_sNormal.IsEmpty())
    {
      const auto& tex = packer.GetTextures()[uiTexIdx++];

      decal.m_NormalRect.x = tex.m_Position.x * 128;
      decal.m_NormalRect.y = tex.m_Position.y * 128;
      decal.m_NormalRect.width = tex.m_Size.x * 128;
      decal.m_NormalRect.height = tex.m_Size.y * 128;
    }
  }

  return EZ_SUCCESS;
}


ezResult ezTexConv::SortDecalsIntoAtlas(ezDynamicArray<DecalDesc> &decals, ezUInt32& out_ResX, ezUInt32& out_ResY)
{
  for (ezUInt32 power = 8; power < 12; ++power)
  {
    const ezUInt32 halfRes = 1 << (power - 1);
    const ezUInt32 resolution = 1 << power;
    const ezUInt32 resDiv128 = resolution / 128;
    const ezUInt32 halfResDiv128 = halfRes / 128;

    if (TrySortDecalsIntoAtlas(decals, resDiv128, halfResDiv128).Succeeded())
    {
      out_ResX = resolution;
      out_ResY = halfRes;
      return EZ_SUCCESS;
    }

    if (TrySortDecalsIntoAtlas(decals, halfResDiv128, resDiv128).Succeeded())
    {
      out_ResX = halfRes;
      out_ResY = resolution;
      return EZ_SUCCESS;
    }

    if (TrySortDecalsIntoAtlas(decals, resDiv128, resDiv128).Succeeded())
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

ezResult ezTexConv::CreateDecalAtlasTexture(const ezDynamicArray<DecalDesc>& decals, ezUInt32 uiResX, ezUInt32 uiResY, ezImage& atlas)
{
  atlas.SetWidth(uiResX);
  atlas.SetHeight(uiResY);
  atlas.SetImageFormat(ezImageFormat::R32G32B32A32_FLOAT);
  atlas.AllocateImageData();

  for (const auto& decal : decals)
  {
    if (!decal.m_sDiffuse.IsEmpty())
    {
      ezImage img32;
      EZ_SUCCEED_OR_RETURN(ToFloatImage(decal.m_DiffuseImg, img32));

      ezImageUtils::Copy(atlas, decal.m_DiffuseRect.x, decal.m_DiffuseRect.y, img32);
    }

    if (!decal.m_sNormal.IsEmpty())
    {
      ezImage img32;
      EZ_SUCCEED_OR_RETURN(ToFloatImage(decal.m_NormalImg, img32));

      ezImageUtils::Copy(atlas, decal.m_NormalRect.x, decal.m_NormalRect.y, img32);
    }
  }

  return EZ_SUCCESS;
}

