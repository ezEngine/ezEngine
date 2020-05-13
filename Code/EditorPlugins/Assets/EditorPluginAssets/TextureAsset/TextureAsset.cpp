#include <EditorPluginAssetsPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <QStringList>
#include <QTextStream>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/ImageConversion.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetDocument, 6, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureChannelMode, 1)
  EZ_ENUM_CONSTANT(ezTextureChannelMode::RGBA)->AddAttributes(new ezGroupAttribute("Multi", 0.0f)),
  EZ_ENUM_CONSTANT(ezTextureChannelMode::RGB)->AddAttributes(new ezGroupAttribute("Multi", 1.0f)),
  EZ_ENUM_CONSTANT(ezTextureChannelMode::Red)->AddAttributes(new ezGroupAttribute("Single", 0.0f)),
  EZ_ENUM_CONSTANT(ezTextureChannelMode::Green)->AddAttributes(new ezGroupAttribute("Single", 1.0f)),
  EZ_ENUM_CONSTANT(ezTextureChannelMode::Blue)->AddAttributes(new ezGroupAttribute("Single", 2.0f)),
  EZ_ENUM_CONSTANT(ezTextureChannelMode::Alpha)->AddAttributes(new ezGroupAttribute("Single", 3.0f))
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezTextureAssetDocument::ezTextureAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezTextureAssetProperties>(szDocumentPath, ezAssetDocEngineConnection::Simple)
{
  m_iTextureLod = -1;
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
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return "";
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

ezStatus ezTextureAssetDocument::RunTexConv(
  const char* szTargetFile, const ezAssetFileHeader& AssetHeader, bool bUpdateThumbnail, const ezTextureAssetProfileConfig* pAssetConfig)
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

    temp.Format("{0}", ezArgU(uiHashLow32, 8, true, 16, true));
    arguments << "-assetHashLow";
    arguments << temp.GetData();

    temp.Format("{0}", ezArgU(uiHashHigh32, 8, true, 16, true));
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
    ezOSFile::CreateDirectoryStructure(sDir);

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
    //arguments << "8"; // default value
  }

  if (pProp->m_bFlipHorizontal)
    arguments << "-flip_horz";

  if (pProp->m_bPreserveAlphaCoverage)
  {
    arguments << "-mipsPerserveCoverage";
    arguments << "-mipsAlphaThreshold";
    temp.Format("{0}", ezArgF(pProp->m_fAlphaThreshold, 2));
    arguments << temp.GetData();
  }

  if (pProp->m_TextureUsage == ezTexConvUsage::Hdr)
  {
    arguments << "-hdrExposure";
    temp.Format("{0}", ezArgF(pProp->m_fHdrExposureBias, 2));
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
    temp.Format("-in{0}", i);

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

  ezStringBuilder cmd;
  for (ezInt32 i = 0; i < arguments.size(); ++i)
    cmd.Append(" ", arguments[i].toUtf8().data());

  ezLog::Debug("TexConv.exe{0}", cmd);
  EZ_SUCCEED_OR_RETURN(ezQtEditorApp::GetSingleton()->ExecuteTool("TexConv.exe", arguments, 60, ezLog::GetThreadLocalLogSystem()));

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

  for (ezUInt32 i = GetProperties()->GetNumInputFiles(); i < 4; ++i)
  {
    // remove unused dependencies
    pInfo->m_AssetTransformDependencies.Remove(GetProperties()->GetInputFile(i));
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
      GetObjectAccessor()->SetValue(GetPropertyObject(), "IsRenderTarget", true);
      GetCommandHistory()->FinishTransaction();
      GetCommandHistory()->ClearUndoHistory();
    }
  }
}

ezStatus ezTextureAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  // EZ_ASSERT_DEV(ezStringUtils::IsEqual(szPlatform, "PC"), "Platform '{0}' is not supported", szPlatform);

  const auto* pAssetConfig = pAssetProfile->GetTypeConfig<ezTextureAssetProfileConfig>();

  const auto props = GetProperties();

  if (m_bIsRenderTarget)
  {
    ezDeferredFileWriter file;
    file.SetOutput(szTargetFile);

    AssetHeader.Write(file);

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
      return ezStatus(ezFmt("Writing to target file failed: '{0}'", szTargetFile));

    return ezStatus(EZ_SUCCESS);
  }
  else
  {
    const bool bUpdateThumbnail = pAssetProfile == ezAssetCurator::GetSingleton()->GetDevelopmentAssetProfile();

    ezStatus result = RunTexConv(szTargetFile, AssetHeader, bUpdateThumbnail, pAssetConfig);

    ezFileStats stat;
    if (ezOSFile::GetFileStats(szTargetFile, stat).Succeeded() && stat.m_uiFileSize == 0)
    {
      // if the file was touched, but nothing written to it, delete the file
      // might happen if TexConv crashed or had an error
      ezOSFile::DeleteFile(szTargetFile);
      result.m_Result = EZ_FAILURE;
    }

    return result;
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezTextureAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

enum class TextureType
{
  Unknown,
  Diffuse,
  Normal,
  Roughness,
  AO,
  Metalness,
  Height,
  HDR,
  Linear,
};

ezTextureAssetDocumentGenerator::ezTextureAssetDocumentGenerator()
{
  AddSupportedFileType("tga");
  AddSupportedFileType("dds");
  AddSupportedFileType("jpg");
  AddSupportedFileType("jpeg");
  AddSupportedFileType("hdr");
  AddSupportedFileType("png");
}

ezTextureAssetDocumentGenerator::~ezTextureAssetDocumentGenerator() = default;

void ezTextureAssetDocumentGenerator::GetImportModes(
  const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  ezStringBuilder baseOutputFile = szParentDirRelativePath;

  const ezStringBuilder baseFilename = baseOutputFile.GetFileName();

  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  TextureType tt = TextureType::Unknown;

  if (ezPathUtils::HasExtension(szParentDirRelativePath, "hdr"))
  {
    tt = TextureType::HDR;
  }
  else if (baseFilename.EndsWith_NoCase("_d") || baseFilename.EndsWith_NoCase("diffuse") || baseFilename.EndsWith_NoCase("diff") ||
           baseFilename.EndsWith_NoCase("col") || baseFilename.EndsWith_NoCase("color"))
  {
    tt = TextureType::Diffuse;
  }
  else if (baseFilename.EndsWith_NoCase("_n") || baseFilename.EndsWith_NoCase("normal") || baseFilename.EndsWith_NoCase("normals") ||
           baseFilename.EndsWith_NoCase("nrm") || baseFilename.EndsWith_NoCase("norm"))
  {
    tt = TextureType::Normal;
  }
  else if (baseFilename.EndsWith_NoCase("_rough") || baseFilename.EndsWith_NoCase("roughness") || baseFilename.EndsWith_NoCase("_rgh"))
  {
    tt = TextureType::Roughness;
  }
  else if (baseFilename.EndsWith_NoCase("_ao"))
  {
    tt = TextureType::AO;
  }
  else if (baseFilename.EndsWith_NoCase("_height") || baseFilename.EndsWith_NoCase("_disp"))
  {
    tt = TextureType::Height;
  }
  else if (baseFilename.EndsWith_NoCase("_metal") || baseFilename.EndsWith_NoCase("_met") || baseFilename.EndsWith_NoCase("metallic") ||
           baseFilename.EndsWith_NoCase("metalness"))
  {
    tt = TextureType::Metalness;
  }
  else if (baseFilename.EndsWith_NoCase("_alpha"))
  {
    tt = TextureType::Linear;
  }

  ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
  info.m_Priority = ezAssetDocGeneratorPriority::DefaultPriority;
  info.m_sOutputFileParentRelative = baseOutputFile;

  switch (tt)
  {
    case TextureType::Diffuse:
    {
      info.m_sName = "TextureImport.Diffuse";
      info.m_sIcon = ":/AssetIcons/Texture_2D.png";
      break;
    }

    case TextureType::Normal:
    {
      info.m_sName = "TextureImport.Normal";
      info.m_sIcon = ":/AssetIcons/Texture_Normals.png";
      break;
    }

    case TextureType::Roughness:
    {
      info.m_sName = "TextureImport.Roughness";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.png";
      break;
    }

    case TextureType::AO:
    {
      info.m_sName = "TextureImport.AO";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.png";
      break;
    }

    case TextureType::Metalness:
    {
      info.m_sName = "TextureImport.Metalness";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.png";
      break;
    }

    case TextureType::Height:
    {
      info.m_sName = "TextureImport.Height";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.png";
      break;
    }

    case TextureType::HDR:
    {
      info.m_sName = "TextureImport.HDR";
      info.m_sIcon = ":/AssetIcons/Texture_2D.png";
      break;
    }

    case TextureType::Linear:
    {
      info.m_sName = "TextureImport.Linear";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.png";
      break;
    }
  }

  if (tt != TextureType::Diffuse)
  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sName = "TextureImport.Diffuse";
    info.m_sIcon = ":/AssetIcons/Texture_2D.png";
  }

  if (tt != TextureType::Linear)
  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sName = "TextureImport.Linear";
    info.m_sIcon = ":/AssetIcons/Texture_Linear.png";
  }


  if (tt != TextureType::Normal)
  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sName = "TextureImport.Normal";
    info.m_sIcon = ":/AssetIcons/Texture_Normals.png";
  }

  if (tt != TextureType::Metalness)
  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sName = "TextureImport.Metalness";
    info.m_sIcon = ":/AssetIcons/Texture_Linear.png";
  }

  if (tt != TextureType::Roughness)
  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sName = "TextureImport.Roughness";
    info.m_sIcon = ":/AssetIcons/Texture_Linear.png";
  }

  if (tt != TextureType::AO)
  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sName = "TextureImport.AO";
    info.m_sIcon = ":/AssetIcons/Texture_Linear.png";
  }

  if (tt != TextureType::Height)
  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sName = "TextureImport.Height";
    info.m_sIcon = ":/AssetIcons/Texture_Linear.png";
  }
}

ezStatus ezTextureAssetDocumentGenerator::Generate(
  const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, ezDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return ezStatus("Could not create target document");

  ezTextureAssetDocument* pAssetDoc = ezDynamicCast<ezTextureAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezTextureAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input1", szDataDirRelativePath);
  accessor.SetValue("ChannelMapping", (int)ezTexture2DChannelMappingEnum::RGB1);
  accessor.SetValue("Usage", (int)ezTexConvUsage::Linear);

  if (info.m_sName == "TextureImport.Diffuse")
  {
    accessor.SetValue("Usage", (int)ezTexConvUsage::Color);
  }
  else if (info.m_sName == "TextureImport.Normal")
  {
    accessor.SetValue("Usage", (int)ezTexConvUsage::NormalMap);
  }
  else if (info.m_sName == "TextureImport.HDR")
  {
    accessor.SetValue("Usage", (int)ezTexConvUsage::Hdr);
  }
  else if (info.m_sName == "TextureImport.Linear")
  {
  }
  else if (info.m_sName == "TextureImport.AO")
  {
    accessor.SetValue("ChannelMapping", (int)ezTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)ezTextureFilterSetting::LowestQuality);
  }
  else if (info.m_sName == "TextureImport.Height")
  {
    accessor.SetValue("ChannelMapping", (int)ezTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)ezTextureFilterSetting::LowQuality);
  }
  else if (info.m_sName == "TextureImport.Roughness")
  {
    accessor.SetValue("ChannelMapping", (int)ezTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)ezTextureFilterSetting::LowQuality);
  }
  else if (info.m_sName == "TextureImport.Metalness")
  {
    accessor.SetValue("ChannelMapping", (int)ezTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)ezTextureFilterSetting::LowQuality);
  }

  return ezStatus(EZ_SUCCESS);
}
