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
  ezJoltCollisionMeshAssetDocument(const char* szDocumentPath, bool bConvexMesh);

  static ezStatus WriteToStream(ezChunkStreamWriter& stream, const ezJoltCookingMesh& mesh, const ezJoltCollisionMeshAssetProperties* pProp);

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  ezStatus CreateMeshFromFile(ezJoltCookingMesh& outMesh);
  ezStatus CreateMeshFromGeom(ezGeometry& geom, ezJoltCookingMesh& outMesh);
  virtual ezStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

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

  virtual void GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual ezStatus Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "ezJoltCollisionMeshAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "JoltCollisionMeshes"; }
};

class ezJoltConvexCollisionMeshAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezJoltConvexCollisionMeshAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezJoltConvexCollisionMeshAssetDocumentGenerator();
  ~ezJoltConvexCollisionMeshAssetDocumentGenerator();

  virtual void GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual ezStatus Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "ezJoltConvexCollisionMeshAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "JoltCollisionMeshes"; }
};
