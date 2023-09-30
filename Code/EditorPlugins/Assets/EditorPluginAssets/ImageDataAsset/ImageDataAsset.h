#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetObjects.h>

struct ezImageDataAssetEvent
{
  enum class Type
  {
    Transformed,
  };

  Type m_Type = Type::Transformed;
};

class ezImageDataAssetDocument : public ezSimpleAssetDocument<ezImageDataAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezImageDataAssetDocument, ezSimpleAssetDocument<ezImageDataAssetProperties>);

public:
  ezImageDataAssetDocument(ezStringView sDocumentPath);

  const ezEvent<const ezImageDataAssetEvent&>& Events() const { return m_Events; }

protected:
  ezEvent<const ezImageDataAssetEvent&> m_Events;

  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override { return ezStatus(EZ_SUCCESS); }
  virtual ezTransformStatus InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  ezStatus RunTexConv(const char* szTargetFile, const ezAssetFileHeader& AssetHeader, bool bUpdateThumbnail);
};
