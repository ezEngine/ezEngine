#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>

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
  ezInt32 m_iTextureLod; // -1 == regular sampling, >= 0 == sample that level

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override { return ezStatus(EZ_SUCCESS); }
  virtual ezStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;

  ezStatus RunTexConv(const char* szTargetFile, const ezAssetFileHeader& AssetHeader, bool bUpdateThumbnail);
};

//////////////////////////////////////////////////////////////////////////


class ezTextureAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezTextureAssetDocumentGenerator();
  ~ezTextureAssetDocumentGenerator();

  virtual void GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual ezStatus Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "ezTextureAsset"; }
};
