#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/Declarations.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetUtils.h>
#include <Foundation/Utilities/AssetInfoFile.h>

const char* ToWrapMode(ezImageAddressMode::Enum mode)
{
  switch (mode)
  {
    case ezImageAddressMode::Repeat:
      return "Repeat";
    case ezImageAddressMode::Clamp:
      return "Clamp";
    case ezImageAddressMode::ClampBorder:
      return "ClampBorder";
    case ezImageAddressMode::Mirror:
      return "Mirror";
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return "";
  }
}

const char* ToFilterMode(ezTextureFilterSetting::Enum mode)
{
  switch (mode)
  {
    case ezTextureFilterSetting::FixedNearest:
      return "Nearest";
    case ezTextureFilterSetting::FixedBilinear:
      return "Bilinear";
    case ezTextureFilterSetting::FixedTrilinear:
      return "Trilinear";
    case ezTextureFilterSetting::FixedAnisotropic2x:
      return "Aniso2x";
    case ezTextureFilterSetting::FixedAnisotropic4x:
      return "Aniso4x";
    case ezTextureFilterSetting::FixedAnisotropic8x:
      return "Aniso8x";
    case ezTextureFilterSetting::FixedAnisotropic16x:
      return "Aniso16x";
    case ezTextureFilterSetting::LowestQuality:
      return "Lowest";
    case ezTextureFilterSetting::LowQuality:
      return "Low";
    case ezTextureFilterSetting::DefaultQuality:
      return "Default";
    case ezTextureFilterSetting::HighQuality:
      return "High";
    case ezTextureFilterSetting::HighestQuality:
      return "Highest";
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return "";
}

const char* ToUsageMode(ezTexConvUsage::Enum mode)
{
  switch (mode)
  {
    case ezTexConvUsage::Auto:
      return "Auto";
    case ezTexConvUsage::Color:
      return "Color";
    case ezTexConvUsage::Linear:
      return "Linear";
    case ezTexConvUsage::Hdr:
      return "Hdr";
    case ezTexConvUsage::NormalMap:
      return "NormalMap";
    case ezTexConvUsage::NormalMap_Inverted:
      return "NormalMap_Inverted";
    case ezTexConvUsage::BumpMap:
      return "BumpMap";
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return "";
}

const char* ToMipmapMode(ezTexConvMipmapMode::Enum mode)
{
  switch (mode)
  {
    case ezTexConvMipmapMode::None:
      return "None";
    case ezTexConvMipmapMode::Linear:
      return "Linear";
    case ezTexConvMipmapMode::Kaiser:
      return "Kaiser";
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return "";
}

const char* ToCompressionMode(ezTexConvCompressionMode::Enum mode)
{
  switch (mode)
  {
    case ezTexConvCompressionMode::None:
      return "None";
    case ezTexConvCompressionMode::Medium:
      return "Medium";
    case ezTexConvCompressionMode::High:
      return "High";
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return "";
}

void AppendCommonTexConvArguments(QStringList& inout_arguments, ezStringBuilder& ref_temp, const ezAssetFileHeader& assetHeader, const char* szTargetFile, ezStringView sThumbnailFile, bool bUpdateThumbnail, const ezTexConvCommonSettings& settings)
{
  // Asset Version
  {
    inout_arguments << "-assetVersion";
    inout_arguments << ezConversionUtils::ToString(assetHeader.GetFileVersion(), ref_temp).GetData();
  }

  // Asset Hash
  {
    const ezUInt64 uiHash64 = assetHeader.GetFileHash();
    const ezUInt32 uiHashLow32 = uiHash64 & 0xFFFFFFFF;
    const ezUInt32 uiHashHigh32 = (uiHash64 >> 32) & 0xFFFFFFFF;

    ref_temp.SetFormat("{0}", ezArgU(uiHashLow32, 8, true, 16, true));
    inout_arguments << "-assetHashLow";
    inout_arguments << ref_temp.GetData();

    ref_temp.SetFormat("{0}", ezArgU(uiHashHigh32, 8, true, 16, true));
    inout_arguments << "-assetHashHigh";
    inout_arguments << ref_temp.GetData();
  }

  inout_arguments << "-out";
  inout_arguments << szTargetFile;

  // TexConv writes this itself, because only it knows the resolution and format it chose.
  {
    const ezStringBuilder sInfoFile = ezAssetInfoFile::GetInfoFilePathForOutput(szTargetFile);
    inout_arguments << "-assetInfoOut";
    inout_arguments << sInfoFile.GetData();
  }

  if (bUpdateThumbnail)
  {
    const ezStringBuilder sThumbnail = sThumbnailFile;
    const ezStringBuilder sDir = sThumbnail.GetFileDirectory();
    ezOSFile::CreateDirectoryStructure(sDir).IgnoreResult();

    inout_arguments << "-thumbnailRes";
    inout_arguments << "256";
    inout_arguments << "-thumbnailOut";
    inout_arguments << QString::fromUtf8(sThumbnail.GetData());
  }

  inout_arguments << "-mipmaps";
  inout_arguments << ToMipmapMode(settings.m_MipmapMode);

  inout_arguments << "-compression";
  inout_arguments << ToCompressionMode(settings.m_CompressionMode);

  inout_arguments << "-usage";
  inout_arguments << ToUsageMode(settings.m_TextureUsage);

  if (settings.m_TextureUsage == ezTexConvUsage::Hdr)
  {
    inout_arguments << "-hdrExposure";
    ref_temp.SetFormat("{0}", ezArgF(settings.m_fHdrExposureBias, 2));
    inout_arguments << ref_temp.GetData();
  }

  inout_arguments << "-maxRes" << QString::number(settings.m_uiMaxResolution);

  inout_arguments << "-addressU" << ToWrapMode(settings.m_AddressModeU);
  inout_arguments << "-addressV" << ToWrapMode(settings.m_AddressModeV);
  inout_arguments << "-addressW" << ToWrapMode(settings.m_AddressModeW);
  inout_arguments << "-filter" << ToFilterMode(settings.m_TextureFilter);
}
