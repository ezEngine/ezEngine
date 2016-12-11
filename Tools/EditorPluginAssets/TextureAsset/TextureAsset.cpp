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
#include <QTextStream>
#include <Foundation/IO/OSFile.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetDocument, 3, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureChannelMode, 1)
EZ_ENUM_CONSTANTS(ezTextureChannelMode::RGB, ezTextureChannelMode::Red, ezTextureChannelMode::Green, ezTextureChannelMode::Blue, ezTextureChannelMode::Alpha)
EZ_END_STATIC_REFLECTED_ENUM()

ezTextureAssetDocument::ezTextureAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezTextureAssetProperties>(szDocumentPath, true)
{
  m_iTextureLod = -1;
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

ezStatus ezTextureAssetDocument::RunTexConv(const char* szTargetFile, const ezAssetFileHeader& AssetHeader, bool bUpdateThumbnail)
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
  case ezChannelMappingEnum::R1_2D:
    {
      arguments << "-r";
      arguments << "in0.r"; // always linear
    }
    break;
  case ezChannelMappingEnum::RG1_2D:
    {
      arguments << "-rg";
      arguments << "in0.rg"; // always linear
    }
    break;
  case ezChannelMappingEnum::R1_G2_2D:
    {
      arguments << "-r";
      arguments << "in0.r";
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
  case ezChannelMappingEnum::RGB1_ABLACK_2D:
    {
      arguments << "-rgb";
      arguments << "in0.rgb";
      arguments << "-a";
      arguments << "black";
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
      arguments << "in1.r";    }
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
      arguments << "in3.r";
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

  ezLog::DebugPrintf("TexConv.exe%s", cmd.GetData());

  QProcess proc;
  QString logoutput;
  proc.setProcessChannelMode(QProcess::MergedChannels);
  proc.setReadChannel(QProcess::StandardOutput);
  QObject::connect(&proc, &QProcess::readyReadStandardOutput, [&proc, &logoutput]() { logoutput.append(proc.readAllStandardOutput()); });
  proc.start(QString::fromUtf8(FindTexConvTool().GetData()), arguments);
  auto stat = proc.exitStatus();

  if (!proc.waitForFinished(60000))
    return ezStatus("TexConv.exe timed out");

  // Output log.
  ezString test = logoutput.toUtf8().data();
  ezLog::InfoPrintf("TexConv.exe log output:");
  QTextStream logoutputStream(&logoutput);
  while (!logoutputStream.atEnd())
  {
    QString line = logoutputStream.readLine();
    ezLog::InfoPrintf("%s", line.toUtf8().data());
  }

  if (proc.exitCode() != 0)
    return ezStatus(ezFmt("TexConv.exe returned error code {0}", proc.exitCode()));

  if (bUpdateThumbnail)
  {
    ezUInt64 uiThumbnailHash = ezAssetCurator::GetSingleton()->GetAssetReferenceHash(GetGuid());
    EZ_ASSERT_DEV(uiThumbnailHash != 0, "Thumbnail hash should never we nul when reaching this point!");
    ezAssetFileHeader assetThumbnailHeader;
    assetThumbnailHeader.SetFileHashAndVersion(uiThumbnailHash, GetAssetTypeVersion());
    AppendThumbnailInfo(sThumbnail, assetThumbnailHeader);
    InvalidateAssetThumbnail();
  }

  return ezStatus(EZ_SUCCESS);
}

#define USE_TEXCONV

ezStatus ezTextureAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szPlatform, const ezAssetFileHeader& AssetHeader)
{
  EZ_ASSERT_DEV(ezStringUtils::IsEqual(szPlatform, "PC"), "Platform '%s' is not supported", szPlatform);
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

const char* ezTextureAssetDocument::QueryAssetType() const
{
  if (GetProperties()->IsTexture2D())
    return "Texture 2D";

  if (GetProperties()->IsTextureCube())
    return "Texture Cube";

  return "Unknown";
}
