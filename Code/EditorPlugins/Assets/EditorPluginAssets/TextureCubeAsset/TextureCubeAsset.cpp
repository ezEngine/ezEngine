#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAsset.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureCubeAssetDocument, 3, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("ChannelMode", ezTextureChannelMode, m_ChannelMode),
    EZ_MEMBER_PROPERTY("TextureLod", m_iTextureLod),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const char* ToFilterMode(ezTextureFilterSetting::Enum mode);
const char* ToUsageMode(ezTexConvUsage::Enum mode);
const char* ToCompressionMode(ezTexConvCompressionMode::Enum mode);
const char* ToMipmapMode(ezTexConvMipmapMode::Enum mode);

ezTextureCubeAssetDocument::ezTextureCubeAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezTextureCubeAssetProperties>(sDocumentPath, ezAssetDocEngineConnection::Simple)
{
}

ezStatus ezTextureCubeAssetDocument::RunTexConv(const char* szTargetFile, const ezAssetFileHeader& AssetHeader, bool bUpdateThumbnail)
{
  const ezTextureCubeAssetProperties* pProp = GetProperties();

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

  if (pProp->m_TextureUsage == ezTexConvUsage::Hdr)
  {
    arguments << "-hdrExposure";
    temp.SetFormat("{0}", ezArgF(pProp->m_fHdrExposureBias, 2));
    arguments << temp.GetData();
  }

  // TODO: downscale steps and min/max resolution

  arguments << "-mipmaps";
  arguments << ToMipmapMode(pProp->m_MipmapMode);

  arguments << "-compression";
  arguments << ToCompressionMode(pProp->m_CompressionMode);

  arguments << "-usage";
  arguments << ToUsageMode(pProp->m_TextureUsage);

  arguments << "-filter" << ToFilterMode(pProp->m_TextureFilter);

  arguments << "-type";
  arguments << "Cubemap";

  switch (pProp->m_ChannelMapping)
  {
    case ezTextureCubeChannelMappingEnum::RGB1:
      arguments << "-rgb"
                << "in0";
      break;

    case ezTextureCubeChannelMappingEnum::RGB1TO6:
      arguments << "-rgb0"
                << "in0";
      arguments << "-rgb1"
                << "in1";
      arguments << "-rgb2"
                << "in2";
      arguments << "-rgb3"
                << "in3";
      arguments << "-rgb4"
                << "in4";
      arguments << "-rgb5"
                << "in5";
      break;


    case ezTextureCubeChannelMappingEnum::RGBA1:
      arguments << "-rgba"
                << "in0";
      break;

    case ezTextureCubeChannelMappingEnum::RGBA1TO6:
      arguments << "-rgba0"
                << "in0";
      arguments << "-rgba1"
                << "in1";
      arguments << "-rgba2"
                << "in2";
      arguments << "-rgba3"
                << "in3";
      arguments << "-rgba4"
                << "in4";
      arguments << "-rgba5"
                << "in5";
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  const ezInt32 iNumInputFiles = pProp->GetNumInputFiles();
  for (ezInt32 i = 0; i < iNumInputFiles; ++i)
  {
    if (ezStringUtils::IsNullOrEmpty(pProp->GetInputFile(i)))
      break;

    temp.SetFormat("-in{0}", i);
    arguments << temp.GetData();
    arguments << QString(pProp->GetAbsoluteInputFilePath(i).GetData());
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

void ezTextureCubeAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  switch (GetProperties()->m_ChannelMapping)
  {
    case ezTextureCubeChannelMappingEnum::RGB1:
    case ezTextureCubeChannelMappingEnum::RGBA1:
    {
      // remove file dependencies, that aren't used
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile1());
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile2());
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile3());
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile4());
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile5());
      break;
    }

    case ezTextureCubeChannelMappingEnum::RGB1TO6:
    case ezTextureCubeChannelMappingEnum::RGBA1TO6:
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
}

ezTransformStatus ezTextureCubeAssetDocument::InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const bool bUpdateThumbnail = pAssetProfile == ezAssetCurator::GetSingleton()->GetDevelopmentAssetProfile();

  ezTransformStatus result = RunTexConv(szTargetFile, AssetHeader, bUpdateThumbnail);

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

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureCubeAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezTextureCubeAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezTextureCubeAssetDocumentGenerator::ezTextureCubeAssetDocumentGenerator()
{
  AddSupportedFileType("dds");
  AddSupportedFileType("hdr");
  AddSupportedFileType("exr");

  // these formats would need to use 6 files for the faces
  // more elaborate detection and mapping would need to be implemented
  // AddSupportedFileType("tga");
  // AddSupportedFileType("jpg");
  // AddSupportedFileType("jpeg");
  // AddSupportedFileType("png");
}

ezTextureCubeAssetDocumentGenerator::~ezTextureCubeAssetDocumentGenerator() = default;

void ezTextureCubeAssetDocumentGenerator::GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const
{
  const ezStringBuilder baseFilename = sAbsInputFile.GetFileName();
  const bool isHDR = sAbsInputFile.HasExtension("hdr") || sAbsInputFile.HasExtension("exr");

  const bool isCubemap = ((baseFilename.FindSubString_NoCase("cubemap") != nullptr) || (baseFilename.FindSubString_NoCase("skybox") != nullptr));

  // TODO: if (sAbsInputFile.IsEmpty()) -> CubemapImport.SkyboxAuto

  if (isHDR)
  {
    {
      ezAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
      info.m_Priority = isCubemap ? ezAssetDocGeneratorPriority::HighPriority : ezAssetDocGeneratorPriority::Undecided;
      info.m_sName = "CubemapImport.SkyboxHDR";
      info.m_sIcon = ":/AssetIcons/Texture_Cube.svg";
    }
  }
  else
  {
    {
      ezAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
      info.m_Priority = isCubemap ? ezAssetDocGeneratorPriority::HighPriority : ezAssetDocGeneratorPriority::Undecided;
      info.m_sName = "CubemapImport.Skybox";
      info.m_sIcon = ":/AssetIcons/Texture_Cube.svg";
    }
  }
}

ezStatus ezTextureCubeAssetDocumentGenerator::Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments)
{
  ezStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());

  if (ezOSFile::ExistsFile(sOutFile))
  {
    ezLog::Info("Skipping cubemap import, file has been imported before: '{}'", sOutFile);
    return ezStatus(EZ_SUCCESS);
  }

  auto pApp = ezQtEditorApp::GetSingleton();

  ezStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  ezDocument* pDoc = pApp->CreateDocument(sOutFile, ezDocumentFlags::None);
  if (pDoc == nullptr)
    return ezStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  ezTextureCubeAssetDocument* pAssetDoc = ezDynamicCast<ezTextureCubeAssetDocument*>(pDoc);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input1", sInputFileRel.GetView());
  accessor.SetValue("ChannelMapping", (int)ezTextureCubeChannelMappingEnum::RGB1);

  if (sMode == "CubemapImport.SkyboxHDR")
  {
    accessor.SetValue("Usage", (int)ezTexConvUsage::Hdr);
  }
  else if (sMode == "CubemapImport.Skybox")
  {
    accessor.SetValue("Usage", (int)ezTexConvUsage::Color);
  }

  ezLog::Success("Imported cubemap: '{}'", sOutFile);

  return ezStatus(EZ_SUCCESS);
}
