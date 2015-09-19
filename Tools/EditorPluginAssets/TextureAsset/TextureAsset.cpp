#include <PCH.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <CoreUtils/Image/Formats/DdsFileFormat.h>
#include <CoreUtils/Image/ImageConversion.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetDocument, ezAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTextureAssetDocument::ezTextureAssetDocument(const char* szDocumentPath) : ezSimpleAssetDocument<ezTextureAssetProperties>(szDocumentPath)
{
}

void ezTextureAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo)
{
  const ezTextureAssetProperties* pProp = GetProperties();

  ezStringBuilder sTemp = pProp->GetInputFile();
  sTemp.MakeCleanPath();

  pInfo->m_FileDependencies.PushBack(sTemp);
}

ezStatus ezTextureAssetDocument::InternalTransformAsset(ezStreamWriterBase& stream, const char* szPlatform)
{
  EZ_ASSERT_DEV(ezStringUtils::IsEqual(szPlatform, "PC"), "Platform '%s' is not supported", szPlatform);

  const ezImage* pImage = &GetProperties()->GetImage();
  ezImage ConvertedImage;

  stream << GetProperties()->IsSRGB();

  ezImageFormat::Enum TargetFormat = pImage->GetImageFormat();

  if (pImage->GetImageFormat() == ezImageFormat::B8G8R8_UNORM)
  {
    /// \todo A conversion to B8G8R8X8_UNORM currently fails
    TargetFormat = ezImageFormat::B8G8R8A8_UNORM;
  }

  if (pImage->GetImageFormat() == ezImageFormat::A8_UNORM)
  {
    // convert alpha channel only format to red channel only
    TargetFormat = ezImageFormat::R8_UNORM;
  }

  if (TargetFormat != pImage->GetImageFormat())
  {
    if (ezImageConversionBase::Convert(*pImage, ConvertedImage, TargetFormat).Failed())
      return ezStatus("Conversion to from source format '%s' to target format '%s' failed", ezImageFormat::GetName(pImage->GetImageFormat()), ezImageFormat::GetName(TargetFormat));

    pImage = &ConvertedImage;
  }

  ezDdsFileFormat writer;
  if (writer.WriteImage(stream, *pImage, ezGlobalLog::GetInstance()).Failed())
  {
    return ezStatus("Writing the image data as DDS failed");
  }

  SaveThumbnail(*pImage);

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