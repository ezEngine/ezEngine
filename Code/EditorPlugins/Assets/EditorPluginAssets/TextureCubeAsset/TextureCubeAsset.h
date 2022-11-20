#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>

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

  // for previewing purposes
  ezEnum<ezTextureCubeChannelMode> m_ChannelMode;
  ezInt32 m_iTextureLod; // -1 == regular sampling, >= 0 == sample that level

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override
  {
    return ezStatus(EZ_SUCCESS);
  }
  virtual ezTransformStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

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

  virtual void GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual ezStatus Generate(
    const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "ezTextureCubeAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "Images"; }
};
