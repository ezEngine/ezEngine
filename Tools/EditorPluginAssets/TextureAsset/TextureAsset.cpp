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

  const ezInt32 iNumInputFiles = pProp->GetNumInputFiles();

  for (ezInt32 i = 0; i< iNumInputFiles; ++i)
  {
    ezStringBuilder sTemp = pProp->GetInputFile(i);
    sTemp.MakeCleanPath();
    pInfo->m_FileDependencies.Insert(sTemp);
  }
}

ezResult ezTextureAssetDocument::RunTexConv(const char* szTargetFile, const ezAssetFileHeader& AssetHeader)
{
  const auto pProp = GetProperties();

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

  arguments << "-channels";
  arguments << ezConversionUtils::ToString(pProp->GetNumChannels()).GetData();

  if (pProp->m_bMipmaps)
    arguments << "-mipmaps";

  if (pProp->m_bCompression)
    arguments << "-compress";

  const bool sRGB = pProp->IsSRGB();
  if (sRGB)
    arguments << "-srgb";

  const ezInt32 iNumInputFiles = pProp->GetNumInputFiles();
  for (ezInt32 i = 0; i < iNumInputFiles; ++i)
  {
    temp.Format("-in%i", i);

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
      arguments << (sRGB ? "in0.rgb" : "in0.xyz");
    }
    break;
  case ezChannelMappingEnum::R1_G2_B3_2D:
    {
      arguments << "-r";
      arguments << (sRGB ? "in0.r" : "in0.x");
      arguments << "-g";
      arguments << (sRGB ? "in1.r" : "in1.x");
      arguments << "-b";
      arguments << (sRGB ? "in2.r" : "in2.x");
    }
    break;
  case ezChannelMappingEnum::RGBA1_2D:
    {
      arguments << "-rgba";
      arguments << (sRGB ? "in0.rgba" : "in0.xyzw");
    }
    break;
  case ezChannelMappingEnum::RGB1_A2_2D:
    {
      arguments << "-rgb";
      arguments << (sRGB ? "in0.rgb" : "in0.xyz");
      arguments << "-a";
      arguments << "in1.x";    }
    break;
  case ezChannelMappingEnum::R1_G2_B3_A4_2D:
    {
      arguments << "-r";
      arguments << (sRGB ? "in0.r" : "in0.x");
      arguments << "-g";
      arguments << (sRGB ? "in1.r" : "in1.x");
      arguments << "-b";
      arguments << (sRGB ? "in2.r" : "in2.x");
      arguments << "-a";
      arguments << "in3.x";
    }
    break;
  case ezChannelMappingEnum::RGB1_CUBE:
    {
      arguments << "-rgb0";
      arguments << (sRGB ? "in0.rgb" : "in0.xyz");
      arguments << "-rgb1";
      arguments << (sRGB ? "in1.rgb" : "in1.xyz");
      arguments << "-rgb2";
      arguments << (sRGB ? "in2.rgb" : "in2.xyz");
      arguments << "-rgb3";
      arguments << (sRGB ? "in3.rgb" : "in3.xyz");
      arguments << "-rgb4";
      arguments << (sRGB ? "in4.rgb" : "in4.xyz");
      arguments << "-rgb5";
      arguments << (sRGB ? "in5.rgb" : "in5.xyz");
    }
    break;
  case ezChannelMappingEnum::RGBA1_CUBE:
    {
      arguments << "-rgba0";
      arguments << (sRGB ? "in0.rgba" : "in0.xyzw");
      arguments << "-rgba1";
      arguments << (sRGB ? "in1.rgba" : "in1.xyzw");
      arguments << "-rgba2";
      arguments << (sRGB ? "in2.rgba" : "in2.xyzw");
      arguments << "-rgba3";
      arguments << (sRGB ? "in3.rgba" : "in3.xyzw");
      arguments << "-rgba4";
      arguments << (sRGB ? "in4.rgba" : "in4.xyzw");
      arguments << "-rgba5";
      arguments << (sRGB ? "in5.rgba" : "in5.xyzw");
    }
    break;
  }

  ezStringBuilder cmd;
  for (ezInt32 i = 0; i < arguments.size(); ++i)
    cmd.Append(" ", arguments[i].toUtf8().data());

  ezLog::Debug("TexConv.exe%s", cmd.GetData());
  
  QProcess proc;
  proc.start(QString::fromUtf8("TexConv.exe"), arguments);
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

  return EZ_SUCCESS;
}

#define USE_TEXCONV

ezStatus ezTextureAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szPlatform, const ezAssetFileHeader& AssetHeader)
{
  EZ_ASSERT_DEV(ezStringUtils::IsEqual(szPlatform, "PC"), "Platform '%s' is not supported", szPlatform);

  //const ezImage* pImage = &GetProperties()->GetImage();
  //SaveThumbnail(*pImage);

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
  if (GetProperties()->IsTexture2D())
    return "Texture 2D";

  if (GetProperties()->IsTextureCube())
    return "Texture Cube";

  return "Unknown";
}