#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetObjects.h>

class ezPxMeshResourceDescriptor;
class ezGeometry;
class ezChunkStreamWriter;
struct ezPhysXCookingMesh;

class ezCollisionMeshAssetDocument : public ezSimpleAssetDocument<ezCollisionMeshAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCollisionMeshAssetDocument, ezSimpleAssetDocument<ezCollisionMeshAssetProperties>);

public:
  ezCollisionMeshAssetDocument(const char* szDocumentPath);

  /// \brief Overridden, because QueryAssetType() doesn't return a constant here
  virtual const char* GetDocumentTypeDisplayString() const override { return "Collision Mesh Asset"; }

  virtual const char* QueryAssetType() const override;

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;

  ezStatus CreateMeshFromFile(const ezMat3 &mTransformation, ezPhysXCookingMesh& outMesh);
  ezStatus CreateMeshFromGeom(ezGeometry& geom, ezPhysXCookingMesh& outMesh);
  ezStatus WriteToStream(ezChunkStreamWriter& stream, const ezPhysXCookingMesh& mesh);
  virtual ezStatus InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader) override;
};
