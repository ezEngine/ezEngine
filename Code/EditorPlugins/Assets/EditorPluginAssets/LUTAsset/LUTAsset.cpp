#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>



#include <EditorPluginAssets/LUTAsset/AdobeCUBEReader.h>
#include <EditorPluginAssets/LUTAsset/LUTAsset.h>

#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/ezTexFormat/ezTexFormat.h>



// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLUTAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezLUTAssetDocument::ezLUTAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezLUTAssetProperties>(sDocumentPath, ezAssetDocEngineConnection::None)
{
}

ezTransformStatus ezLUTAssetDocument::InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
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
  if (fmt.WriteImage(file, img, "dds").Failed())
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

void ezLUTAssetDocumentGenerator::GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const
{
  ezAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
  info.m_Priority = ezAssetDocGeneratorPriority::DefaultPriority;
  info.m_sName = "LUTImport.Cube";
  info.m_sIcon = ":/AssetIcons/LUT.svg";
}

ezStatus ezLUTAssetDocumentGenerator::Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments)
{
  ezStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());

  if (ezOSFile::ExistsFile(sOutFile))
  {
    ezLog::Info("Skipping LUT import, file has been imported before: '{}'", sOutFile);
    return ezStatus(EZ_SUCCESS);
  }

  auto pApp = ezQtEditorApp::GetSingleton();

  ezStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  ezDocument* pDoc = pApp->CreateDocument(sOutFile, ezDocumentFlags::None);
  if (pDoc == nullptr)
    return ezStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  ezLUTAssetDocument* pAssetDoc = ezDynamicCast<ezLUTAssetDocument*>(pDoc);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input", sInputFileRel.GetView());

  ezLog::Success("Imported LUT: '{}'", sOutFile);

  return ezStatus(EZ_SUCCESS);
}
