#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>
#include <ModelImporter/Declarations.h>

class ezMeshResourceDescriptor;
class ezMaterialAssetDocument;

namespace ezModelImporter
{
  struct Material;
  class Mesh;
  class Scene;
  struct TextureReference;
}

class ezAnimatedMeshAssetDocument : public ezSimpleAssetDocument<ezAnimatedMeshAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimatedMeshAssetDocument, ezSimpleAssetDocument<ezAnimatedMeshAssetProperties>);

public:
  ezAnimatedMeshAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override { return "Animated Mesh"; }

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform,
                                          const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;

  ezStatus CreateMeshFromFile(ezAnimatedMeshAssetProperties* pProp, ezMeshResourceDescriptor& desc);


  virtual ezStatus InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader) override;
};

//////////////////////////////////////////////////////////////////////////

class ezAnimatedMeshAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimatedMeshAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezAnimatedMeshAssetDocumentGenerator();
  ~ezAnimatedMeshAssetDocumentGenerator();

  virtual void GetImportModes(const char* szParentDirRelativePath,
                              ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual ezStatus Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info,
                            ezDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "ezAnimatedMeshAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "Meshes"; }
};
