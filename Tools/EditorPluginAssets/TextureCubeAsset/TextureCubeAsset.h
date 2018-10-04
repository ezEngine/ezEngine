#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>

struct ezTextureCubeChannelMode
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
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTextureCubeChannelMode);

class ezTextureCubeAssetDocument : public ezSimpleAssetDocument<ezTextureCubeAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureCubeAssetDocument, ezSimpleAssetDocument<ezTextureCubeAssetProperties>);

public:
  ezTextureCubeAssetDocument(const char* szDocumentPath);

  /// \brief Overridden, because QueryAssetType() doesn't return a constant here
  virtual const char* GetDocumentTypeDisplayString() const override { return "TextureCube Asset"; }

  virtual const char* QueryAssetType() const override;

  // for previewing purposes
  ezEnum<ezTextureCubeChannelMode> m_ChannelMode;
  ezInt32 m_iTextureLod; // -1 == regular sampling, >= 0 == sample that level

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezAssetProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override { return ezStatus(EZ_SUCCESS); }
  virtual ezStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const ezAssetProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;

  ezStatus RunTexConv(const char* szTargetFile, const ezAssetFileHeader& AssetHeader, bool bUpdateThumbnail);
};

//////////////////////////////////////////////////////////////////////////

class ezTextureCubeAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureCubeAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezTextureCubeAssetDocumentGenerator();
  ~ezTextureCubeAssetDocumentGenerator();

  virtual void GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual ezStatus Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "ezTextureCubeAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "Images"; }
};
