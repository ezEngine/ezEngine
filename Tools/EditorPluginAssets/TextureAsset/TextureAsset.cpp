#include <PCH.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <CoreUtils/Image/Formats/DdsFileFormat.h>
#include <CoreUtils/Image/ImageConversion.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
#include <QProcess>
#include <QStringList>
#include <Foundation/IO/OSFile.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezTextureAssetDocument::ezTextureAssetDocument(const char* szDocumentPath) : ezSimpleAssetDocument<ezTextureAssetProperties>(szDocumentPath)
{
}

void ezTextureAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo)
{
  const ezTextureAssetProperties* pProp = GetProperties();

  const ezInt32 iNumInputFiles = pProp->GetNumInputFiles();

  for (ezInt32 i = 0; i < iNumInputFiles; ++i)
  {
    ezStringBuilder sTemp = pProp->GetInputFile(i);
    sTemp.MakeCleanPath();
    pInfo->m_FileDependencies.Insert(sTemp);
  }
}

ezString ezTextureAssetDocument::FindTexConvTool() const
{
  ezStringBuilder sTool = ezQtEditorApp::GetSingleton()->GetExternalToolsFolder();
  sTool.AppendPath("TexConv.exe");

  if (ezFileSystem::ExistsFile(sTool))
    return sTool;

  // just try the one in the same folder as the editor
  return "TexConv.exe";
}

ezResult ezTextureAssetDocument::RunTexConv(const char* szTargetFile, const ezAssetFileHeader& AssetHeader, bool bUpdateThumbnail)
{
  const ezTextureAssetProperties* pProp = GetProperties();

  QStringList arguments;
  ezStringBuilder temp;

  // Asset Version
  {
    arguments << "-assetVersion";
    arguments << ezConversionUtils::ToString(AssetHeader.GetFileVersion()).GetData();
  }

  // Asset Hash
  {
    const ezUInt64 uiHash64 = AssetHeader.GetFileHash();
    const ezUInt32 uiHashLow32 = uiHash64 & 0xFFFFFFFF;
    const ezUInt32 uiHashHigh32 = (uiHash64 >> 32) & 0xFFFFFFFF;

    temp.Format("%08X", uiHashLow32);
    arguments << "-assetHashLow";
    arguments << temp.GetData();

    temp.Format("%08X", uiHashHigh32);
    arguments << "-assetHashHigh";
    arguments << temp.GetData();
  }


  arguments << "-out";
  arguments << szTargetFile;

  if (bUpdateThumbnail)
  {
    // Thumbnail
    const ezStringBuilder sThumbnail = GetThumbnailFilePath();
    const ezStringBuilder sDir = sThumbnail.GetFileDirectory();
    ezOSFile::CreateDirectoryStructure(sDir);

    arguments << "-thumbnail";
    arguments << QString::fromUtf8(sThumbnail.GetData());
  }

  arguments << "-channels";
  arguments << ezConversionUtils::ToString(pProp->GetNumChannels()).GetData();

  if (pProp->m_bMipmaps)
    arguments << "-mipmaps";

  if (pProp->m_bCompression)
    arguments << "-compress";

  if (pProp->IsSRGB())
    arguments << "-srgb";

  if (pProp->m_bPremultipliedAlpha)
    arguments << "-premulalpha";

  if (pProp->IsTextureCube())
    arguments << "-cubemap";

  const ezInt32 iNumInputFiles = pProp->GetNumInputFiles();
  for (ezInt32 i = 0; i < iNumInputFiles; ++i)
  {
    temp.Format("-in%i", i);

    if (ezStringUtils::IsNullOrEmpty(pProp->GetInputFile(i)))
      break;

    arguments << temp.GetData();
    arguments << QString(pProp->GetAbsoluteInputFilePath(i).GetData());
  }

  switch (pProp->GetChannelMapping())
  {
  case ezChannelMappingEnum::R1_2D:
    {
      arguments << "-r";
      arguments << "in0.x"; // always linear
    }
    break;
  case ezChannelMappingEnum::RG1_2D:
    {
      arguments << "-rg";
      arguments << "in0.xy"; // always linear
    }
    break;
  case ezChannelMappingEnum::R1_G2_2D:
    {
      arguments << "-r";
      arguments << "in0.x";
      arguments << "-g";
      arguments << "in1.y"; // always linear
    }
    break;
  case ezChannelMappingEnum::RGB1_2D:
    {
      arguments << "-rgb";
      arguments << "in0.rgb";
    }
    break;
  case ezChannelMappingEnum::R1_G2_B3_2D:
    {
      arguments << "-r";
      arguments << "in0.r";
      arguments << "-g";
      arguments << "in1.r";
      arguments << "-b";
      arguments << "in2.r";
    }
    break;
  case ezChannelMappingEnum::RGBA1_2D:
    {
      arguments << "-rgba";
      arguments << "in0.rgba";
    }
    break;
  case ezChannelMappingEnum::RGB1_A2_2D:
    {
      arguments << "-rgb";
      arguments << "in0.rgb";
      arguments << "-a";
      arguments << "in1.x";    }
    break;
  case ezChannelMappingEnum::R1_G2_B3_A4_2D:
    {
      arguments << "-r";
      arguments << "in0.r";
      arguments << "-g";
      arguments << "in1.r";
      arguments << "-b";
      arguments << "in2.r";
      arguments << "-a";
      arguments << "in3.x";
    }
    break;

  case ezChannelMappingEnum::RGB1_CUBE:
  case ezChannelMappingEnum::RGBA1_CUBE:
  case ezChannelMappingEnum::RGB1TO6_CUBE:
  case ezChannelMappingEnum::RGBA1TO6_CUBE:
    break;
  }

  ezStringBuilder cmd;
  for (ezInt32 i = 0; i < arguments.size(); ++i)
    cmd.Append(" ", arguments[i].toUtf8().data());

  ezLog::Debug("TexConv.exe%s", cmd.GetData());

  QProcess proc;
  proc.start(QString::fromUtf8(FindTexConvTool().GetData()), arguments);
  if (!proc.waitForFinished(60000))
  {
    ezLog::Error("TexConv.exe timed out");
    return EZ_FAILURE;
  }

  if (proc.exitCode() != 0)
  {
    ezLog::Error("TexConv.exe returned error code %i", proc.exitCode());
    return EZ_FAILURE;
  }

  if (bUpdateThumbnail)
  {
    InvalidateAssetThumbnail();
  }

  return EZ_SUCCESS;
}

#define USE_TEXCONV

ezStatus ezTextureAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szPlatform, const ezAssetFileHeader& AssetHeader)
{
  EZ_ASSERT_DEV(ezStringUtils::IsEqual(szPlatform, "PC"), "Platform '%s' is not supported", szPlatform);
  const bool bUpdateThumbnail = ezStringUtils::IsEqual(szPlatform, "PC");

  RunTexConv(szTargetFile, AssetHeader, bUpdateThumbnail);

  return ezStatus(EZ_SUCCESS);
}

const char* ezTextureAssetDocument::QueryAssetType() const
{
  if (GetProperties()->IsTexture2D())
    return "Texture 2D";

  if (GetProperties()->IsTextureCube())
    return "Texture Cube";

  return "Unknown";
}