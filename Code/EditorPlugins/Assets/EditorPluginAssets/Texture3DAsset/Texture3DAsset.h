#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <EditorPluginAssets/Texture3DAsset/Texture3DAssetObjects.h>

class ezTextureAssetProfileConfig;

struct ezTexture3DPreviewMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Slices,
    RayMarch,

    Default = RayMarch
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORPLUGINASSETS_DLL, ezTexture3DPreviewMode);

class ezTexture3DAssetDocument : public ezSimpleAssetDocument<ezTexture3DAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTexture3DAssetDocument, ezSimpleAssetDocument<ezTexture3DAssetProperties>);

public:
  ezTexture3DAssetDocument(ezStringView sDocumentPath);

  // for previewing purposes
  ezEnum<ezTexture3DPreviewMode> m_PreviewMode;
  float m_fSliceCoordinate = 0.5f;      // [0, 1], only used in Slices preview mode
  float m_fOpacityMultiplier = 1.0f;    // only used in RayMarch preview mode

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override { return ezStatus(EZ_SUCCESS); }
  virtual ezTransformStatus InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  ezStatus RunTexConv(const char* szTargetFile, const ezAssetFileHeader& AssetHeader, bool bUpdateThumbnail, const ezTextureAssetProfileConfig* pAssetConfig);
};

//////////////////////////////////////////////////////////////////////////

class ezTexture3DAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTexture3DAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezTexture3DAssetDocumentGenerator();
  ~ezTexture3DAssetDocumentGenerator();

  virtual void GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual ezStringView GetDocumentExtension() const override { return "ezTexture3DAsset"; }
  virtual ezStringView GetGeneratorGroup() const override { return "Images"; }
  virtual ezStatus Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments) override;
};
