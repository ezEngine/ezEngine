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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezTextureAssetDocument::ezTextureAssetDocument(const char* szDocumentPath) : ezSimpleAssetDocument<ezTextureAssetProperties>(szDocumentPath)
{
}

void ezTextureAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo)
{
  const ezTextureAssetProperties* pProp = GetProperties();

  ezStringBuilder sTemp = pProp->GetInputFile();
  sTemp.MakeCleanPath();

  pInfo->m_FileDependencies.Insert(sTemp);
}

ezResult ezTextureAssetDocument::RunTexConv(const char* szTargetFile, const ezAssetFileHeader& AssetHeader)
{
  QStringList arguments;
  arguments << "-out";
  arguments << szTargetFile;

  if (GetProperties()->IsSRGB())
    arguments << "-srgb";

  arguments << "-mipmaps";
  arguments << "-compress";
  arguments << "-channels";
  arguments << "3";

  arguments << "-in0";
  arguments << QString(GetProperties()->GetAbsoluteInputFilePath().GetData());

  arguments << "-rgba";
  arguments << "in0.rgba";

  /// \todo Asset Hash + Version
  
  QProcess proc;
  proc.start(QString::fromUtf8("TexConv.exe"), arguments);
  if (!proc.waitForFinished(60000))
    return EZ_FAILURE;

  if (proc.exitCode() != 0)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezStatus ezTextureAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szPlatform, const ezAssetFileHeader& AssetHeader)
{
  EZ_ASSERT_DEV(ezStringUtils::IsEqual(szPlatform, "PC"), "Platform '%s' is not supported", szPlatform);

  const ezImage* pImage = &GetProperties()->GetImage();
  SaveThumbnail(*pImage);

#ifdef USE_TEXCONV
  RunTexConv(szTargetFile, AssetHeader);
#else

  ezDeferredFileWriter stream;
  stream.SetOutput(szTargetFile);
  AssetHeader.Write(stream);

  // set the input file again to ensure it is reloaded
  GetProperties()->SetInputFile(GetProperties()->GetInputFile());

  ezImage ConvertedImage;

  stream << GetProperties()->IsSRGB();

  ezImageFormat::Enum TargetFormat = pImage->GetImageFormat();

  if (pImage->GetImageFormat() == ezImageFormat::B8G8R8_UNORM)
  {
    TargetFormat = ezImageFormat::B8G8R8A8_UNORM;
  }

  if (pImage->GetImageFormat() == ezImageFormat::A8_UNORM)
  {
    // convert alpha channel only format to red channel only
    TargetFormat = ezImageFormat::R8_UNORM;
  }

  if (TargetFormat != pImage->GetImageFormat())
  {
    if (ezImageConversion::Convert(*pImage, ConvertedImage, TargetFormat).Failed())
      return ezStatus("Conversion to from source format '%s' to target format '%s' failed", ezImageFormat::GetName(pImage->GetImageFormat()), ezImageFormat::GetName(TargetFormat));

    pImage = &ConvertedImage;
  }

  ezDdsFileFormat writer;
  if (writer.WriteImage(stream, *pImage, ezGlobalLog::GetOrCreateInstance()).Failed())
  {
    return ezStatus("Writing the image data as DDS failed");
  }

  if (stream.Close().Failed())
  {
    ezLog::Error("Could not open file for writing: '%s'", szTargetFile);
    return ezStatus("Opening the asset output file failed");
  }

#endif

  return ezStatus(EZ_SUCCESS);
}

const char* ezTextureAssetDocument::QueryAssetType() const
{
  switch (GetProperties()->GetTextureType())
  {
  case ezTextureTypeEnum::Texture2D:
    return "Texture 2D";
  case ezTextureTypeEnum::Texture3D:
    return "Texture 3D";
  case ezTextureTypeEnum::TextureCube:
    return "Texture Cube";
  }

  return "Unknown";
}