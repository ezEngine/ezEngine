#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetDocument, 6, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("ChannelMode", ezTextureChannelMode, m_ChannelMode),
    EZ_MEMBER_PROPERTY("TextureLod", m_iTextureLod),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureChannelMode, 1)
  EZ_ENUM_CONSTANT(ezTextureChannelMode::RGBA)->AddAttributes(new ezGroupAttribute("Multi", 0.0f)),
  EZ_ENUM_CONSTANT(ezTextureChannelMode::RGB)->AddAttributes(new ezGroupAttribute("Multi", 1.0f)),
  EZ_ENUM_CONSTANT(ezTextureChannelMode::Red)->AddAttributes(new ezGroupAttribute("Single", 0.0f)),
  EZ_ENUM_CONSTANT(ezTextureChannelMode::Green)->AddAttributes(new ezGroupAttribute("Single", 1.0f)),
  EZ_ENUM_CONSTANT(ezTextureChannelMode::Blue)->AddAttributes(new ezGroupAttribute("Single", 2.0f)),
  EZ_ENUM_CONSTANT(ezTextureChannelMode::Alpha)->AddAttributes(new ezGroupAttribute("Single", 3.0f)),
  EZ_ENUM_CONSTANT(ezTextureChannelMode::CoverageRed)->AddAttributes(new ezGroupAttribute("Coverage", 0.0f)),
  EZ_ENUM_CONSTANT(ezTextureChannelMode::CoverageAlpha)->AddAttributes(new ezGroupAttribute("Coverage", 1.0f)),
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezTextureAssetDocument::ezTextureAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezTextureAssetProperties>(sDocumentPath, ezAssetDocEngineConnection::Simple)
{
}

static const char* ToWrapMode(ezImageAddressMode::Enum mode)
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

ezStatus ezTextureAssetDocument::RunTexConv(const char* szTargetFile, const ezAssetFileHeader& AssetHeader, bool bUpdateThumbnail, const ezTextureAssetProfileConfig* pAssetConfig)
{
  const ezTextureAssetProperties* pProp = GetProperties();

  QStringList arguments;
  ezStringBuilder temp;

  // Asset Version
  {
    arguments << "-assetVersion";
    arguments << ezConversionUtils::ToString(AssetHeader.GetFileVersion(), temp).GetData();
  }

  // Asset Hash
  {
    const ezUInt64 uiHash64 = AssetHeader.GetFileHash();
    const ezUInt32 uiHashLow32 = uiHash64 & 0xFFFFFFFF;
    const ezUInt32 uiHashHigh32 = (uiHash64 >> 32) & 0xFFFFFFFF;

    temp.SetFormat("{0}", ezArgU(uiHashLow32, 8, true, 16, true));
    arguments << "-assetHashLow";
    arguments << temp.GetData();

    temp.SetFormat("{0}", ezArgU(uiHashHigh32, 8, true, 16, true));
    arguments << "-assetHashHigh";
    arguments << temp.GetData();
  }


  arguments << "-out";
  arguments << szTargetFile;

  const ezStringBuilder sThumbnail = GetThumbnailFilePath();
  if (bUpdateThumbnail)
  {
    // Thumbnail
    const ezStringBuilder sDir = sThumbnail.GetFileDirectory();
    ezOSFile::CreateDirectoryStructure(sDir).IgnoreResult();

    arguments << "-thumbnailRes";
    arguments << "256";
    arguments << "-thumbnailOut";

    arguments << QString::fromUtf8(sThumbnail.GetData());
  }

  // low resolution data
  {
    ezStringBuilder lowResPath = szTargetFile;
    ezStringBuilder name = lowResPath.GetFileName();
    name.Append("-lowres");
    lowResPath.ChangeFileName(name);

    arguments << "-lowMips";
    arguments << "6";
    arguments << "-lowOut";

    arguments << QString::fromUtf8(lowResPath.GetData());
  }

  arguments << "-mipmaps";
  arguments << ToMipmapMode(pProp->m_MipmapMode);

  arguments << "-compression";
  arguments << ToCompressionMode(pProp->m_CompressionMode);

  arguments << "-usage";
  arguments << ToUsageMode(pProp->m_TextureUsage);

  if (pProp->m_bPremultipliedAlpha)
    arguments << "-premulalpha";

  if (pProp->m_bDilateColor)
  {
    arguments << "-dilate";
    // arguments << "8"; // default value
  }

  if (pProp->m_bFlipHorizontal)
    arguments << "-flip_horz";

  if (pProp->m_bPreserveAlphaCoverage)
  {
    arguments << "-mipsPreserveCoverage";
    arguments << "-mipsAlphaThreshold";
    temp.SetFormat("{0}", ezArgF(pProp->m_fAlphaThreshold, 2));
    arguments << temp.GetData();
  }

  if (pProp->m_TextureUsage == ezTexConvUsage::Hdr)
  {
    arguments << "-hdrExposure";
    temp.SetFormat("{0}", ezArgF(pProp->m_fHdrExposureBias, 2));
    arguments << temp.GetData();
  }

  arguments << "-maxRes" << QString::number(pAssetConfig->m_uiMaxResolution);

  arguments << "-addressU" << ToWrapMode(pProp->m_AddressModeU);
  arguments << "-addressV" << ToWrapMode(pProp->m_AddressModeV);
  arguments << "-addressW" << ToWrapMode(pProp->m_AddressModeW);
  arguments << "-filter" << ToFilterMode(pProp->m_TextureFilter);

  const ezInt32 iNumInputFiles = pProp->GetNumInputFiles();
  for (ezInt32 i = 0; i < iNumInputFiles; ++i)
  {
    temp.SetFormat("-in{0}", i);

    if (ezStringUtils::IsNullOrEmpty(pProp->GetInputFile(i)))
      break;

    arguments << temp.GetData();
    arguments << QString(pProp->GetAbsoluteInputFilePath(i).GetData());
  }

  switch (pProp->GetChannelMapping())
  {
    case ezTexture2DChannelMappingEnum::R1:
    {
      arguments << "-r";
      arguments << "in0.r"; // always linear
    }
    break;

    case ezTexture2DChannelMappingEnum::RG1:
    {
      arguments << "-rg";
      arguments << "in0.rg"; // always linear
    }
    break;

    case ezTexture2DChannelMappingEnum::R1_G2:
    {
      arguments << "-r";
      arguments << "in0.r";
      arguments << "-g";
      arguments << "in1.g"; // always linear
    }
    break;

    case ezTexture2DChannelMappingEnum::RGB1:
    {
      arguments << "-rgb";
      arguments << "in0.rgb";
    }
    break;

    case ezTexture2DChannelMappingEnum::RGB1_ABLACK:
    {
      arguments << "-rgb";
      arguments << "in0.rgb";
      arguments << "-a";
      arguments << "black";
    }
    break;

    case ezTexture2DChannelMappingEnum::R1_G2_B3:
    {
      arguments << "-r";
      arguments << "in0.r";
      arguments << "-g";
      arguments << "in1.r";
      arguments << "-b";
      arguments << "in2.r";
    }
    break;

    case ezTexture2DChannelMappingEnum::RGBA1:
    {
      arguments << "-rgba";
      arguments << "in0.rgba";
    }
    break;

    case ezTexture2DChannelMappingEnum::RGB1_A2:
    {
      arguments << "-rgb";
      arguments << "in0.rgb";
      arguments << "-a";
      arguments << "in1.r";
    }
    break;

    case ezTexture2DChannelMappingEnum::R1_G2_B3_A4:
    {
      arguments << "-r";
      arguments << "in0.r";
      arguments << "-g";
      arguments << "in1.r";
      arguments << "-b";
      arguments << "in2.r";
      arguments << "-a";
      arguments << "in3.r";
    }
    break;
  }

  EZ_SUCCEED_OR_RETURN(ezQtEditorApp::GetSingleton()->ExecuteTool("ezTexConv", arguments, 180, ezLog::GetThreadLocalLogSystem()));

  if (bUpdateThumbnail)
  {
    ezUInt64 uiThumbnailHash = ezAssetCurator::GetSingleton()->GetAssetReferenceHash(GetGuid());
    EZ_ASSERT_DEV(uiThumbnailHash != 0, "Thumbnail hash should never be zero when reaching this point!");

    ThumbnailInfo thumbnailInfo;
    thumbnailInfo.SetFileHashAndVersion(uiThumbnailHash, GetAssetTypeVersion());
    AppendThumbnailInfo(sThumbnail, thumbnailInfo);
    InvalidateAssetThumbnail();
  }

  return ezStatus(EZ_SUCCESS);
}


void ezTextureAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (!m_bIsRenderTarget)
  {
    // every 2D texture also generates a "-lowres" output, which is used to be embedded into materials for quick streaming
    pInfo->m_Outputs.Insert("LOWRES");
  }

  for (ezUInt32 i = GetProperties()->GetNumInputFiles(); i < 4; ++i)
  {
    // remove unused dependencies
    pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile(i));
  }
}

void ezTextureAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  if (m_bIsRenderTarget)
  {
    if (GetProperties()->m_bIsRenderTarget == false)
    {
      GetCommandHistory()->StartTransaction("MakeRenderTarget");
      GetObjectAccessor()->SetValueByName(GetPropertyObject(), "IsRenderTarget", true).AssertSuccess();
      GetCommandHistory()->FinishTransaction();
      GetCommandHistory()->ClearUndoHistory();
    }
  }
}

ezTransformStatus ezTextureAssetDocument::InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  if (sOutputTag.IsEqual("LOWRES"))
  {
    // no need to generate this file, it will be generated together with the main output
    return ezTransformStatus();
  }

  const auto* pAssetConfig = pAssetProfile->GetTypeConfig<ezTextureAssetProfileConfig>();

  const auto props = GetProperties();

  if (m_bIsRenderTarget)
  {
    ezDeferredFileWriter file;
    file.SetOutput(szTargetFile);

    EZ_SUCCEED_OR_RETURN(AssetHeader.Write(file));

    // TODO: move this into a shared location, reuse in ezTexConv::WriteTexHeader
    const ezUInt8 uiTexFileFormatVersion = 5;
    file << uiTexFileFormatVersion;

    ezGALResourceFormat::Enum format = ezGALResourceFormat::Invalid;
    bool bIsSRGB = false;

    switch (props->m_RtFormat)
    {
      case ezRenderTargetFormat::RGBA8:
        format = ezGALResourceFormat::RGBAUByteNormalized;
        break;

      case ezRenderTargetFormat::RGBA8sRgb:
        format = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
        bIsSRGB = true;
        break;

      case ezRenderTargetFormat::RGB10:
        format = ezGALResourceFormat::RG11B10Float;
        break;

      case ezRenderTargetFormat::RGBA16:
        format = ezGALResourceFormat::RGBAHalf;
        break;
    }

    file << bIsSRGB;
    file << (ezUInt8)props->m_AddressModeU;
    file << (ezUInt8)props->m_AddressModeV;
    file << (ezUInt8)props->m_AddressModeW;
    file << (ezUInt8)props->m_TextureFilter;

    ezInt16 resX = 0, resY = 0;

    switch (props->m_Resolution)
    {
      case ezTexture2DResolution::Fixed64x64:
        resX = 64;
        resY = 64;
        break;
      case ezTexture2DResolution::Fixed128x128:
        resX = 128;
        resY = 128;
        break;
      case ezTexture2DResolution::Fixed256x256:
        resX = 256;
        resY = 256;
        break;
      case ezTexture2DResolution::Fixed512x512:
        resX = 512;
        resY = 512;
        break;
      case ezTexture2DResolution::Fixed1024x1024:
        resX = 1024;
        resY = 1024;
        break;
      case ezTexture2DResolution::Fixed2048x2048:
        resX = 2048;
        resY = 2048;
        break;
      case ezTexture2DResolution::CVarRtResolution1:
        resX = -1;
        resY = 1;
        break;
      case ezTexture2DResolution::CVarRtResolution2:
        resX = -1;
        resY = 2;
        break;
      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
    }

    file << resX;
    file << resY;
    file << props->m_fCVarResolutionScale;
    file << (int)format;


    if (file.Close().Failed())
      return ezTransformStatus(ezFmt("Writing to target file failed: '{0}'", szTargetFile));

    return ezTransformStatus();
  }
  else
  {
    const bool bUpdateThumbnail = pAssetProfile == ezAssetCurator::GetSingleton()->GetDevelopmentAssetProfile();

    ezTransformStatus result = RunTexConv(szTargetFile, AssetHeader, bUpdateThumbnail, pAssetConfig);

    ezFileStats stat;
    if (ezOSFile::GetFileStats(szTargetFile, stat).Succeeded() && stat.m_uiFileSize == 0)
    {
      // if the file was touched, but nothing written to it, delete the file
      // might happen if TexConv crashed or had an error
      ezOSFile::DeleteFile(szTargetFile).IgnoreResult();

      if (result.Succeeded())
        result = ezTransformStatus("TexConv did not write an output file");
    }

    return result;
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezTextureAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezTextureAssetDocumentGenerator::ezTextureAssetDocumentGenerator()
{
  AddSupportedFileType("tga");
  AddSupportedFileType("dds");
  AddSupportedFileType("jpg");
  AddSupportedFileType("jpeg");
  AddSupportedFileType("png");
  AddSupportedFileType("hdr");
  AddSupportedFileType("exr");
}

ezTextureAssetDocumentGenerator::~ezTextureAssetDocumentGenerator() = default;

ezTextureAssetDocumentGenerator::TextureType ezTextureAssetDocumentGenerator::DetermineTextureType(ezStringView sFile)
{
  ezStringBuilder baseFilename = sFile.GetFileName();

  while (baseFilename.TrimWordEnd("_") ||
         baseFilename.TrimWordEnd("K") ||
         baseFilename.TrimWordEnd("-") ||
         baseFilename.TrimWordEnd("1") ||
         baseFilename.TrimWordEnd("2") ||
         baseFilename.TrimWordEnd("3") ||
         baseFilename.TrimWordEnd("4") ||
         baseFilename.TrimWordEnd("5") ||
         baseFilename.TrimWordEnd("6") ||
         baseFilename.TrimWordEnd("7") ||
         baseFilename.TrimWordEnd("8") ||
         baseFilename.TrimWordEnd("9") ||
         baseFilename.TrimWordEnd("0") ||
         baseFilename.TrimWordEnd("gl"))
  {
  }

  if (sFile.HasExtension("hdr"))
  {
    return TextureType::HDR;
  }
  else if (sFile.HasExtension("exr"))
  {
    return TextureType::HDR;
  }
  else if (baseFilename.EndsWith_NoCase("_d") || baseFilename.EndsWith_NoCase("diffuse") || baseFilename.EndsWith_NoCase("diff") || baseFilename.EndsWith_NoCase("col") || baseFilename.EndsWith_NoCase("color"))
  {
    return TextureType::Diffuse;
  }
  else if (baseFilename.EndsWith_NoCase("_n") || baseFilename.EndsWith_NoCase("normal") || baseFilename.EndsWith_NoCase("normals") || baseFilename.EndsWith_NoCase("nrm") || baseFilename.EndsWith_NoCase("norm") || baseFilename.EndsWith_NoCase("_nor"))
  {
    return TextureType::Normal;
  }
  else if (baseFilename.EndsWith_NoCase("_arm") || baseFilename.EndsWith_NoCase("_orm"))
  {
    return TextureType::ORM;
  }
  else if (baseFilename.EndsWith_NoCase("_rough") || baseFilename.EndsWith_NoCase("roughness") || baseFilename.EndsWith_NoCase("_rgh"))
  {
    return TextureType::Roughness;
  }
  else if (baseFilename.EndsWith_NoCase("_ao"))
  {
    return TextureType::Occlusion;
  }
  else if (baseFilename.EndsWith_NoCase("_height") || baseFilename.EndsWith_NoCase("_disp"))
  {
    return TextureType::Height;
  }
  else if (baseFilename.EndsWith_NoCase("_metal") || baseFilename.EndsWith_NoCase("_met") || baseFilename.EndsWith_NoCase("metallic") || baseFilename.EndsWith_NoCase("metalness"))
  {
    return TextureType::Metalness;
  }
  else if (baseFilename.EndsWith_NoCase("_alpha"))
  {
    return TextureType::Linear;
  }

  return TextureType::Diffuse;
}

void ezTextureAssetDocumentGenerator::GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const
{
  if (sAbsInputFile.IsEmpty())
  {
    {
      ezAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
      info2.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
      info2.m_sName = "TextureImport.Auto";
      info2.m_sIcon = ":/AssetIcons/Texture_2D.svg";
    }

    //{
    //  ezAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    //  info2.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    //  info2.m_sName = "TextureImport.Diffuse";
    //  info2.m_sIcon = ":/AssetIcons/Texture_2D.svg";
    //}

    //{
    //  ezAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    //  info2.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    //  info2.m_sName = "TextureImport.Linear";
    //  info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
    //}

    //{
    //  ezAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    //  info2.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    //  info2.m_sName = "TextureImport.Normal";
    //  info2.m_sIcon = ":/AssetIcons/Texture_Normals.svg";
    //}
    return;
  }

  const TextureType tt = DetermineTextureType(sAbsInputFile);

  ezAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
  info.m_Priority = ezAssetDocGeneratorPriority::DefaultPriority;

  // first add the default option
  switch (tt)
  {
    case TextureType::Diffuse:
    {
      info.m_sName = "TextureImport.Diffuse";
      info.m_sIcon = ":/AssetIcons/Texture_2D.svg";
      break;
    }

    case TextureType::Normal:
    {
      info.m_sName = "TextureImport.Normal";
      info.m_sIcon = ":/AssetIcons/Texture_Normals.svg";
      break;
    }

    case TextureType::Roughness:
    {
      info.m_sName = "TextureImport.Roughness";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }

    case TextureType::Occlusion:
    {
      info.m_sName = "TextureImport.Occlusion";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }

    case TextureType::Metalness:
    {
      info.m_sName = "TextureImport.Metalness";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }

    case TextureType::ORM:
    {
      info.m_sName = "TextureImport.ORM";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }

    case TextureType::Height:
    {
      info.m_sName = "TextureImport.Height";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }

    case TextureType::HDR:
    {
      info.m_sName = "TextureImport.HDR";
      info.m_sIcon = ":/AssetIcons/Texture_2D.svg";
      break;
    }

    case TextureType::Linear:
    {
      info.m_sName = "TextureImport.Linear";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }
  }

  // now add all the other options

  if (tt != TextureType::Diffuse)
  {
    ezAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    info2.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info2.m_sName = "TextureImport.Diffuse";
    info2.m_sIcon = ":/AssetIcons/Texture_2D.svg";
  }

  if (tt != TextureType::Linear)
  {
    ezAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    info2.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info2.m_sName = "TextureImport.Linear";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }

  if (tt != TextureType::Normal)
  {
    ezAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    info2.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info2.m_sName = "TextureImport.Normal";
    info2.m_sIcon = ":/AssetIcons/Texture_Normals.svg";
  }

  if (tt != TextureType::Metalness)
  {
    ezAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    info2.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info2.m_sName = "TextureImport.Metalness";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }

  if (tt != TextureType::Roughness)
  {
    ezAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    info2.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info2.m_sName = "TextureImport.Roughness";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }

  if (tt != TextureType::Occlusion)
  {
    ezAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    info2.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info2.m_sName = "TextureImport.Occlusion";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }

  if (tt != TextureType::ORM)
  {
    ezAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    info2.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info2.m_sName = "TextureImport.ORM";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }

  if (tt != TextureType::Height)
  {
    ezAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    info2.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info2.m_sName = "TextureImport.Height";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }
}

ezStatus ezTextureAssetDocumentGenerator::Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments)
{
  if (sMode == "TextureImport.Auto")
  {
    const TextureType tt = DetermineTextureType(sInputFileAbs);

    switch (tt)
    {
      case TextureType::Diffuse:
        sMode = "TextureImport.Diffuse";
        break;
      case TextureType::Normal:
        sMode = "TextureImport.Normal";
        break;
      case TextureType::Occlusion:
        sMode = "TextureImport.Occlusion";
        break;
      case TextureType::Roughness:
        sMode = "TextureImport.Roughness";
        break;
      case TextureType::Metalness:
        sMode = "TextureImport.Metalness";
        break;
      case TextureType::ORM:
        sMode = "TextureImport.ORM";
        break;
      case TextureType::Height:
        sMode = "TextureImport.Height";
        break;
      case TextureType::HDR:
        sMode = "TextureImport.HDR";
        break;
      case TextureType::Linear:
        sMode = "TextureImport.Linear";
        break;
    }
  }

  ezStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());
  ezOSFile::FindFreeFilename(sOutFile);

  auto pApp = ezQtEditorApp::GetSingleton();

  ezStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  ezDocument* pDoc = pApp->CreateDocument(sOutFile, ezDocumentFlags::None);
  if (pDoc == nullptr)
    return ezStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  ezTextureAssetDocument* pAssetDoc = ezDynamicCast<ezTextureAssetDocument*>(pDoc);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezTextureAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input1", sInputFileRel.GetView());
  accessor.SetValue("ChannelMapping", (int)ezTexture2DChannelMappingEnum::RGB1);
  accessor.SetValue("Usage", (int)ezTexConvUsage::Linear);

  if (sMode == "TextureImport.Diffuse")
  {
    accessor.SetValue("Usage", (int)ezTexConvUsage::Color);
  }
  else if (sMode == "TextureImport.Normal")
  {
    accessor.SetValue("Usage", (int)ezTexConvUsage::NormalMap);
  }
  else if (sMode == "TextureImport.HDR")
  {
    accessor.SetValue("Usage", (int)ezTexConvUsage::Hdr);
  }
  else if (sMode == "TextureImport.Linear")
  {
  }
  else if (sMode == "TextureImport.Occlusion")
  {
    accessor.SetValue("ChannelMapping", (int)ezTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)ezTextureFilterSetting::LowestQuality);
  }
  else if (sMode == "TextureImport.Height")
  {
    accessor.SetValue("ChannelMapping", (int)ezTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)ezTextureFilterSetting::LowQuality);
  }
  else if (sMode == "TextureImport.Roughness")
  {
    accessor.SetValue("ChannelMapping", (int)ezTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)ezTextureFilterSetting::LowQuality);
  }
  else if (sMode == "TextureImport.Metalness")
  {
    accessor.SetValue("ChannelMapping", (int)ezTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)ezTextureFilterSetting::LowQuality);
  }
  else if (sMode == "TextureImport.ORM")
  {
    accessor.SetValue("ChannelMapping", (int)ezTexture2DChannelMappingEnum::RGB1);
    accessor.SetValue("TextureFilter", (int)ezTextureFilterSetting::LowQuality);
  }

  return ezStatus(EZ_SUCCESS);
}
