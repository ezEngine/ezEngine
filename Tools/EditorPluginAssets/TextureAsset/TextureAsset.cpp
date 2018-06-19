#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Image/Formats/DdsFileFormat.h>
#include <Foundation/Image/ImageConversion.h>
#include <QStringList>
#include <QTextStream>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetDocument, 5, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureChannelMode, 1)
EZ_ENUM_CONSTANTS(ezTextureChannelMode::RGB, ezTextureChannelMode::Red, ezTextureChannelMode::Green, ezTextureChannelMode::Blue, ezTextureChannelMode::Alpha)
EZ_END_STATIC_REFLECTED_ENUM()

ezTextureAssetDocument::ezTextureAssetDocument(const char* szDocumentPath)
    : ezSimpleAssetDocument<ezTextureAssetProperties>(szDocumentPath, true)
{
  m_iTextureLod = -1;
}

ezStatus ezTextureAssetDocument::RunTexConv(const char* szTargetFile, const ezAssetFileHeader& AssetHeader, bool bUpdateThumbnail)
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

    arguments << "-thumbnail";
    arguments << QString::fromUtf8(sThumbnail.GetData());
  }

  // low resolution data
  {
    ezStringBuilder lowResPath = szTargetFile;
    ezStringBuilder name = lowResPath.GetFileName();
    name.Append("-lowres");
    lowResPath.ChangeFileName(name);

    arguments << "-outLowRes";
    arguments << QString::fromUtf8(lowResPath.GetData());
  }

  arguments << "-channels";
  arguments << ezConversionUtils::ToString(pProp->GetNumChannels(), temp).GetData();

  if (pProp->m_bMipmaps)
    arguments << "-mipmaps";

  if (pProp->m_bCompression)
    arguments << "-compress";

  if (pProp->IsSRGB())
    arguments << "-srgb";

  if (pProp->IsHDR())
    arguments << "-hdr";

  if (pProp->m_bPremultipliedAlpha)
    arguments << "-premulalpha";

  if (pProp->m_bFlipHorizontal)
    arguments << "-flip_horz";

  arguments << "-addressU" << QString::number(pProp->m_AddressModeU.GetValue());
  arguments << "-addressV" << QString::number(pProp->m_AddressModeV.GetValue());
  arguments << "-addressW" << QString::number(pProp->m_AddressModeW.GetValue());
  arguments << "-filter" << QString::number(pProp->m_TextureFilter.GetValue());

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
    arguments << "in1.y"; // always linear
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
    ezAssetFileHeader assetThumbnailHeader;
    assetThumbnailHeader.SetFileHashAndVersion(uiThumbnailHash, GetAssetTypeVersion());
    AppendThumbnailInfo(sThumbnail, assetThumbnailHeader);
    InvalidateAssetThumbnail();
  }

  return ezStatus(EZ_SUCCESS);
}


void ezTextureAssetDocument::InitializeAfterLoading()
{
  SUPER::InitializeAfterLoading();

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

ezStatus ezTextureAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  EZ_ASSERT_DEV(ezStringUtils::IsEqual(szPlatform, "PC"), "Platform '{0}' is not supported", szPlatform);

  const auto props = GetProperties();

  if (m_bIsRenderTarget)
  {
    ezDeferredFileWriter file;
    file.SetOutput(szTargetFile);

    AssetHeader.Write(file);

    // TODO: move this into a shared location, reuse in ezTexConv::WriteTexHeader
    const ezUInt8 uiTexFileFormatVersion = 5;
    file << uiTexFileFormatVersion;

    file << props->IsSRGB();
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

    ezGALResourceFormat::Enum format = ezGALResourceFormat::Invalid;

    switch (props->m_RtFormat)
    {
    case ezRenderTargetFormat::RGBA8:
      format = ezGALResourceFormat::RGBAUByteNormalized;
      break;

    case ezRenderTargetFormat::RGBA8sRgb:
      format = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
      break;

    case ezRenderTargetFormat::RGB10:
      format = ezGALResourceFormat::RG11B10Float;
      break;

    case ezRenderTargetFormat::RGBA16:
      format = ezGALResourceFormat::RGBAHalf;
      break;
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
    const bool bUpdateThumbnail = ezStringUtils::IsEqual(szPlatform, "PC");

    ezStatus result = RunTexConv(szTargetFile, AssetHeader, bUpdateThumbnail);

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

const char* ezTextureAssetDocument::QueryAssetType() const
{
  if (m_bIsRenderTarget)
  {
    return "Render Target";
  }

  return "Texture 2D";
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezTextureAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE

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

void ezTextureAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  ezStringBuilder baseOutputFile = szParentDirRelativePath;

  const ezStringBuilder baseFilename = baseOutputFile.GetFileName();
  const bool isHDR = ezPathUtils::HasExtension(szParentDirRelativePath, "hdr");

  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  /// \todo make this configurable
  const bool isNormalMap = !isHDR &&
                           (baseFilename.EndsWith_NoCase("_n") ||
                            baseFilename.EndsWith_NoCase("normal") ||
                            baseFilename.EndsWith_NoCase("normals") ||
                            baseFilename.EndsWith_NoCase("nrm") ||
                            baseFilename.EndsWith_NoCase("norm"));

  const bool isLinear = !isHDR && !isNormalMap &&
                        (baseFilename.EndsWith_NoCase("_ao") ||
                         baseFilename.EndsWith_NoCase("_rough") ||
                         baseFilename.EndsWith_NoCase("roughness") ||
                         baseFilename.EndsWith_NoCase("_rgh") ||
                         baseFilename.EndsWith_NoCase("_height") ||
                         baseFilename.EndsWith_NoCase("_alpha") ||
                         baseFilename.EndsWith_NoCase("_metal") ||
                         baseFilename.EndsWith_NoCase("_met") ||
                         baseFilename.EndsWith_NoCase("_disp") ||
                         baseFilename.EndsWith_NoCase("metallic"));

  if (isHDR)
  {
    {
      ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
      info.m_Priority = ezAssetDocGeneratorPriority::DefaultPriority;
      info.m_sName = "TextureImport.HDR";
      info.m_sOutputFileParentRelative = baseOutputFile;
      info.m_sIcon = ":/AssetIcons/Texture_2D.png";
    }
  }
  else
  {
    {
      ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
      info.m_Priority = ezAssetDocGeneratorPriority::DefaultPriority;
      info.m_sName = "TextureImport.Diffuse";
      info.m_sOutputFileParentRelative = baseOutputFile;
      info.m_sIcon = ":/AssetIcons/Texture_2D.png";
    }

    {
      ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
      info.m_Priority = isNormalMap ? ezAssetDocGeneratorPriority::HighPriority : ezAssetDocGeneratorPriority::LowPriority;
      info.m_sName = "TextureImport.Normal";
      info.m_sOutputFileParentRelative = baseOutputFile;
      info.m_sIcon = ":/AssetIcons/Texture_Normals.png";
    }

    {
      ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
      info.m_Priority = isLinear ? ezAssetDocGeneratorPriority::HighPriority : ezAssetDocGeneratorPriority::LowPriority;
      info.m_sName = "TextureImport.Linear";
      info.m_sOutputFileParentRelative = baseOutputFile;
      info.m_sIcon = ":/AssetIcons/Texture_Linear.png";
    }
  }
}

ezStatus ezTextureAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateOrOpenDocument(true, info.m_sOutputFileAbsolute, false, false);
  if (out_pGeneratedDocument == nullptr)
    return ezStatus("Could not create target document");

  ezTextureAssetDocument* pAssetDoc = ezDynamicCast<ezTextureAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezTextureAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input1", szDataDirRelativePath);
  accessor.SetValue("ChannelMapping", (int)ezTexture2DChannelMappingEnum::RGB1);

  if (info.m_sName == "TextureImport.Diffuse")
  {
    accessor.SetValue("Usage", (int)ezTexture2DUsageEnum::Diffuse);
  }
  else if (info.m_sName == "TextureImport.Normal")
  {
    accessor.SetValue("Usage", (int)ezTexture2DUsageEnum::NormalMap);
  }
  else if (info.m_sName == "TextureImport.HDR")
  {
    accessor.SetValue("Usage", (int)ezTexture2DUsageEnum::HDR);
  }
  else if (info.m_sName == "TextureImport.Linear")
  {
    accessor.SetValue("Usage", (int)ezTexture2DUsageEnum::Other_Linear);
  }

  return ezStatus(EZ_SUCCESS);
}
