#include <EditorPluginAssetsPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>

#include <Core/Assets/AssetFileHeader.h>

#include <RendererCore/Textures/Texture3DResource.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

#include <EditorPluginAssets/LUTAsset/AdobeCUBEReader.h>
#include <EditorPluginAssets/LUTAsset/LUTAsset.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetManager.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>

#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageHeader.h>
#include <Texture/ezTexFormat/ezTexFormat.h>

#include <ToolsFoundation/Reflection/PhantomRttiManager.h>


// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLUTAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezLUTAssetDocument::ezLUTAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezLUTAssetProperties>(szDocumentPath, ezAssetDocEngineConnection::Simple)
{
}

ezStatus ezLUTAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const auto props = GetProperties();

  // Read CUBE file, convert to 3D texture and write to file
  ezFileStats Stats;
  bool bStat = ezOSFile::GetFileStats(props->GetAbsoluteInputFilePath(), Stats).Succeeded();

  ezFileReader cubeFile;
  if (!bStat || cubeFile.Open(props->GetAbsoluteInputFilePath()).Failed())
  {
    return ezStatus(ezFmt("Couldn't open CUBE file '{0}'.", props->GetAbsoluteInputFilePath()));
  }

  ezAdobeCUBEReader cubeReader;
  auto parseRes = cubeReader.ParseFile(cubeFile);
  if (parseRes.Failed())
    return parseRes;

  const ezUInt32 lutSize = cubeReader.GetLUTSize();

  // Build an ezImage from the data
  ezImageHeader imgHeader;
  imgHeader.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM_SRGB);
  imgHeader.SetWidth(lutSize);
  imgHeader.SetHeight(lutSize);
  imgHeader.SetDepth(lutSize);

  ezImage img;
  img.ResetAndAlloc(imgHeader);

  if (!img.IsValid())
  {
    return ezStatus("Allocated ezImage for LUT data is not valid.");
  }



  for (ezUInt32 b = 0; b < lutSize; ++b)
  {
    for (ezUInt32 g = 0; g < lutSize; ++g)
    {
      for (ezUInt32 r = 0; r < lutSize; ++r)
      {
        const ezVec3 val = cubeReader.GetLUTEntry(r, g, b);

        ezColor col(val.x, val.y, val.z);
        ezColorGammaUB colUb(col);

        ezColorGammaUB* pPixel = img.GetPixelPointer<ezColorGammaUB>(0, 0, 0, r, g, b);

        *pPixel = colUb;
      }
    }
  }

  ezDeferredFileWriter file;
  file.SetOutput(szTargetFile);
  EZ_SUCCEED_OR_RETURN(AssetHeader.Write(file));

  ezTexFormat texFormat;
  texFormat.m_bSRGB = true;
  texFormat.m_AddressModeU = ezImageAddressMode::Clamp;
  texFormat.m_AddressModeV = ezImageAddressMode::Clamp;
  texFormat.m_AddressModeW = ezImageAddressMode::Clamp;
  texFormat.m_TextureFilter = ezTextureFilterSetting::FixedBilinear;

  texFormat.WriteTextureHeader(file);

  ezDdsFileFormat fmt;
  if (fmt.WriteImage(file, img, ezLog::GetThreadLocalLogSystem(), "dds").Failed())
    return ezStatus(ezFmt("Writing image to target file failed: '{0}'", szTargetFile));

  if (file.Close().Failed())
    return ezStatus(ezFmt("Writing to target file failed: '{0}'", szTargetFile));

  return ezStatus(EZ_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLUTAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezLUTAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezLUTAssetDocumentGenerator::ezLUTAssetDocumentGenerator()
{
  AddSupportedFileType("cube");
}

ezLUTAssetDocumentGenerator::~ezLUTAssetDocumentGenerator() = default;

void ezLUTAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  ezStringBuilder baseOutputFile = szParentDirRelativePath;

  const ezStringBuilder baseFilename = baseOutputFile.GetFileName();

  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
  info.m_Priority = ezAssetDocGeneratorPriority::DefaultPriority;
  info.m_sOutputFileParentRelative = baseOutputFile;

  info.m_sName = "LUTImport.Cube";
  info.m_sIcon = ":/AssetIcons/LUT.png";
}

ezStatus ezLUTAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, ezDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return ezStatus("Could not create target document");

  ezLUTAssetDocument* pAssetDoc = ezDynamicCast<ezLUTAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezLUTAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input", szDataDirRelativePath);

  return ezStatus(EZ_SUCCESS);
}
