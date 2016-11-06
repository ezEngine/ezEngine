#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>

struct ezTextureChannelMode
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    RGB,
    Red,
    Green,
    Blue,
    Alpha,

    Default = RGB
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTextureChannelMode);

class ezTextureAssetDocument : public ezSimpleAssetDocument<ezTextureAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureAssetDocument, ezSimpleAssetDocument<ezTextureAssetProperties>);

public:
  ezTextureAssetDocument(const char* szDocumentPath);

  /// \brief Overridden, because QueryAssetType() doesn't return a constant here
  virtual const char* GetDocumentTypeDisplayString() const override { return "Texture Asset"; }

  virtual const char* QueryAssetType() const override;

  // for previewing purposes
  ezEnum<ezTextureChannelMode> m_ChannelMode;

protected:
  virtual ezStatus InternalTransformAsset(const char* szTargetFile, const char* szPlatform, const ezAssetFileHeader& AssetHeader) override;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform, const ezAssetFileHeader& AssetHeader) override { return ezStatus(EZ_SUCCESS); }

  ezString FindTexConvTool() const;
  ezStatus RunTexConv(const char* szTargetFile, const ezAssetFileHeader& AssetHeader, bool bUpdateThumbnail);
};
