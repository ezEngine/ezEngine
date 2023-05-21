#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>

struct ezTextureCubeChannelMode
{
  using StorageType = ezUInt8;

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

  // for previewing purposes
  ezEnum<ezTextureCubeChannelMode> m_ChannelMode;
  ezInt32 m_iTextureLod; // -1 == regular sampling, >= 0 == sample that level

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override
  {
    return ezStatus(EZ_SUCCESS);
  }
  virtual ezTransformStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  ezStatus RunTexConv(const char* szTargetFile, const ezAssetFileHeader& AssetHeader, bool bUpdateThumbnail);

  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////

class ezTextureCubeAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureCubeAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezTextureCubeAssetDocumentGenerator();
  ~ezTextureCubeAssetDocumentGenerator();

  virtual void GetImportModes(ezStringView sParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_modes) const override;
  virtual ezStatus Generate(ezStringView sDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual ezStringView GetDocumentExtension() const override { return "ezTextureCubeAsset"; }
  virtual ezStringView GetGeneratorGroup() const override { return "Images"; }
};
