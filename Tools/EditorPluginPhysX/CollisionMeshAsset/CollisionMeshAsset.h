#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetObjects.h>

class ezPhysXMeshResourceDescriptor;
class ezGeometry;
class ezChunkStreamWriter;

class ezCollisionMeshAssetDocument : public ezSimpleAssetDocument<ezCollisionMeshAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCollisionMeshAssetDocument, ezSimpleAssetDocument<ezCollisionMeshAssetProperties>);

public:
  ezCollisionMeshAssetDocument(const char* szDocumentPath);

  virtual const char* GetDocumentTypeDisplayString() const override { return "Collision Mesh Asset"; }

  virtual const char* QueryAssetType() const override;

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) override;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform) override;

  ezStatus CreateMeshFromFile(const ezCollisionMeshAssetProperties* pProp, bool bFlipTriangles, const ezMat3 &mTransformation, ezChunkStreamWriter& stream);

  virtual ezStatus InternalRetrieveAssetInfo(const char* szPlatform) override;

};
