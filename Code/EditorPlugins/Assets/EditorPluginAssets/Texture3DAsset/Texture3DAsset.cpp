#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/Texture3DAsset/Texture3DAsset.h>
#include <EditorPluginAssets/Texture3DAsset/Texture3DAssetManager.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetUtils.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Texture/Image/ImageHeader.h>
#include <Texture/Image/Formats/ImageFileFormat.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTexture3DPreviewMode, 1)
  EZ_ENUM_CONSTANTS(ezTexture3DPreviewMode::Slices, ezTexture3DPreviewMode::RayMarch)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTexture3DAssetDocument, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("PreviewMode", ezTexture3DPreviewMode, m_PreviewMode),
    EZ_MEMBER_PROPERTY("SliceCoordinate", m_fSliceCoordinate),
    EZ_MEMBER_PROPERTY("OpacityMultiplier", m_fOpacityMultiplier),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTexture3DAssetDocument::ezTexture3DAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezTexture3DAssetProperties>(sDocumentPath, ezAssetDocEngineConnection::Simple)
{
}

ezStatus ezTexture3DAssetDocument::RunTexConv(const char* szTargetFile, const ezAssetFileHeader& AssetHeader, bool bUpdateThumbnail, const ezTextureAssetProfileConfig* pAssetConfig)
{
  const ezTexture3DAssetProperties* pProp = GetProperties();

  if (pProp->m_TextureUsage == ezTexConvUsage::Auto)
  {
    return ezStatus("'Auto' usage detection is not supported for volume textures. Please pick an explicit Usage (e.g. Linear or Color).");
  }

  QStringList arguments;
  ezStringBuilder temp;

  const ezStringBuilder sThumbnail = GetThumbnailFilePath();

  ezTexConvCommonSettings commonSettings;
  commonSettings.m_MipmapMode = pProp->m_MipmapMode;
  commonSettings.m_CompressionMode = pProp->m_CompressionMode;
  commonSettings.m_TextureUsage = pProp->m_TextureUsage;
  commonSettings.m_fHdrExposureBias = pProp->m_fHdrExposureBias;
  commonSettings.m_uiMaxResolution = pAssetConfig->m_uiMaxResolution;
  commonSettings.m_AddressModeU = pProp->m_AddressModeU;
  commonSettings.m_AddressModeV = pProp->m_AddressModeV;
  commonSettings.m_AddressModeW = pProp->m_AddressModeW;
  commonSettings.m_TextureFilter = pProp->m_TextureFilter;

  AppendCommonTexConvArguments(arguments, temp, AssetHeader, szTargetFile, sThumbnail, bUpdateThumbnail, commonSettings);

  arguments << "-type";
  arguments << "Volume";

  arguments << "-in0";
  arguments << QString(pProp->GetAbsoluteInputFilePath().GetData());

  // Only request as many channels from TexConv as the source file actually has. Forcing every
  // volume to full RGBA (regardless of the source being e.g. single-channel grayscale) quadruples
  // the resulting texture's GPU memory footprint for no reason, and can push an otherwise fine
  // volume texture over GPU upload limits.
  ezImageHeader inputHeader;
  ezUInt32 uiNumChannels = 4;
  if (ezImageFileFormat::ReadImageHeader(pProp->GetAbsoluteInputFilePath(), inputHeader).Succeeded())
  {
    uiNumChannels = ezImageFormat::GetNumChannels(inputHeader.GetImageFormat());
  }

  switch (uiNumChannels)
  {
    case 1:
      arguments << "-r" << "in0.r";
      break;
    case 2:
      arguments << "-rg" << "in0.rg";
      break;
    case 3:
      arguments << "-rgb" << "in0.rgb";
      break;
    default:
      arguments << "-rgba" << "in0.rgba";
      break;
  }

  EZ_SUCCEED_OR_RETURN(ezQtEditorApp::GetSingleton()->ExecuteTool("ezTexConv", arguments, 180, ezLog::GetThreadLocalLogSystem()));

  if (bUpdateThumbnail)
  {
    ezUInt64 uiThumbnailHash = ezAssetCurator::GetSingleton()->GetAssetThumbnailHash(GetGuid());
    EZ_ASSERT_DEV(uiThumbnailHash != 0, "Thumbnail hash should never be zero when reaching this point!");

    ThumbnailInfo thumbnailInfo;
    thumbnailInfo.SetFileHashAndVersion(uiThumbnailHash, GetAssetTypeVersion());
    AppendThumbnailInfo(sThumbnail, thumbnailInfo);
    InvalidateAssetThumbnail();
  }

  return ezStatus(EZ_SUCCESS);
}

ezTransformStatus ezTexture3DAssetDocument::InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const auto* pAssetConfig = pAssetProfile->GetTypeConfig<ezTextureAssetProfileConfig>();

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

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTexture3DAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezTexture3DAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezTexture3DAssetDocumentGenerator::ezTexture3DAssetDocumentGenerator()
{
  AddSupportedFileType("dds");
}

ezTexture3DAssetDocumentGenerator::~ezTexture3DAssetDocumentGenerator() = default;

void ezTexture3DAssetDocumentGenerator::GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const
{
  ezAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
  info.m_Priority = ezAssetDocGeneratorPriority::DefaultPriority;
  info.m_sName = "Texture3DImport.Volume";
  info.m_sIcon = ":/AssetIcons/Texture_Cube.svg";
}

ezStatus ezTexture3DAssetDocumentGenerator::Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments)
{
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

  ezTexture3DAssetDocument* pAssetDoc = ezDynamicCast<ezTexture3DAssetDocument*>(pDoc);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezTexture3DAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input", sInputFileRel.GetView());

  return ezStatus(EZ_SUCCESS);
}
