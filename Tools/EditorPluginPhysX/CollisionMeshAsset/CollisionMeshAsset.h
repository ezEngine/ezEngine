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

  /// \brief Overridden, because QueryAssetType() doesn't return a constant here
  virtual const char* GetDocumentTypeDisplayString() const override { return "Collision Mesh Asset"; }

  virtual const char* QueryAssetType() const override;

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader) override;

  ezStatus CreateMeshFromFile(ezCollisionMeshAssetProperties* pProp, bool bFlipTriangles, const ezMat3 &mTransformation, ezChunkStreamWriter& stream);

};
