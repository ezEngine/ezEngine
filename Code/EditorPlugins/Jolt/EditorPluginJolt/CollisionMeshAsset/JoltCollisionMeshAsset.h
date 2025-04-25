#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetObjects.h>

class ezGeometry;
class ezChunkStreamWriter;
struct ezJoltCookingMesh;

class ezJoltCollisionMeshAssetDocument : public ezSimpleAssetDocument<ezJoltCollisionMeshAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezJoltCollisionMeshAssetDocument, ezSimpleAssetDocument<ezJoltCollisionMeshAssetProperties>);

public:
  ezJoltCollisionMeshAssetDocument(ezStringView sDocumentPath, bool bConvexMesh);

  static ezStatus WriteToStream(ezChunkStreamWriter& inout_stream, const ezJoltCookingMesh& mesh, const ezJoltCollisionMeshAssetProperties* pProp);

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  ezStatus CreateMeshFromFile(ezJoltCookingMesh& outMesh);
  ezStatus CreateMeshFromGeom(ezGeometry& geom, ezJoltCookingMesh& outMesh);
  virtual ezTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  bool m_bIsConvexMesh = false;

  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////


class ezJoltCollisionMeshAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezJoltCollisionMeshAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezJoltCollisionMeshAssetDocumentGenerator();
  ~ezJoltCollisionMeshAssetDocumentGenerator();

  virtual void GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual ezStringView GetDocumentExtension() const override { return "ezJoltCollisionMeshAsset"; }
  virtual ezStringView GetGeneratorGroup() const override { return "Meshes"; }
  virtual ezStatus Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments) override;
};

class ezJoltConvexCollisionMeshAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezJoltConvexCollisionMeshAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezJoltConvexCollisionMeshAssetDocumentGenerator();
  ~ezJoltConvexCollisionMeshAssetDocumentGenerator();

  virtual void GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual ezStringView GetDocumentExtension() const override { return "ezJoltConvexCollisionMeshAsset"; }
  virtual ezStringView GetGeneratorGroup() const override { return "Meshes"; }
  virtual ezStatus Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments) override;
};
