#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetObjects.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>

class ezPxMeshResourceDescriptor;
class ezGeometry;
class ezChunkStreamWriter;
struct ezPhysXCookingMesh;

class ezCollisionMeshAssetDocument : public ezSimpleAssetDocument<ezCollisionMeshAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCollisionMeshAssetDocument, ezSimpleAssetDocument<ezCollisionMeshAssetProperties>);

public:
  ezCollisionMeshAssetDocument(const char* szDocumentPath, bool bConvexMesh);

  /// \brief Overridden, because QueryAssetType() doesn't return a constant here
  virtual const char* GetDocumentTypeDisplayString() const override { return "Collision Mesh Asset"; }

  virtual const char* QueryAssetType() const override;

  static ezStatus WriteToStream(ezChunkStreamWriter& stream, const ezPhysXCookingMesh& mesh, const ezCollisionMeshAssetProperties* pProp);

protected:
  virtual void InitializeAfterLoading() override;

  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  ezStatus CreateMeshFromFile(const ezMat3 &mTransformation, ezPhysXCookingMesh& outMesh);
  ezStatus CreateMeshFromGeom(ezGeometry& geom, ezPhysXCookingMesh& outMesh);
  virtual ezStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  bool m_bIsConvexMesh = false;
};

//////////////////////////////////////////////////////////////////////////


class ezCollisionMeshAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCollisionMeshAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezCollisionMeshAssetDocumentGenerator();
  ~ezCollisionMeshAssetDocumentGenerator();

  virtual void GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual ezStatus Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "ezCollisionMeshAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "CollisionMeshes"; }
};

class ezConvexCollisionMeshAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConvexCollisionMeshAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezConvexCollisionMeshAssetDocumentGenerator();
  ~ezConvexCollisionMeshAssetDocumentGenerator();

  virtual void GetImportModes(const char* szParentDirRelativePath,
                              ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual ezStatus Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info,
                            ezDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "ezConvexCollisionMeshAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "CollisionMeshes"; }
};
