#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class ezCollisionMeshAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCollisionMeshAssetDocumentManager, ezAssetDocumentManager);

public:
  ezCollisionMeshAssetDocumentManager();
  ~ezCollisionMeshAssetDocumentManager();

  virtual ezString GetResourceTypeExtension(const char* szDocumentPath) const override
  {
    // TODO: should use a different file extension for concave/convex meshes, otherwise one can create
    // two meshes with the same name that will overwrite each other every time they are transformed
    return "ezPhysXMesh";
  }

  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const override
  {
    inout_AssetTypeNames.Insert("Collision Mesh");
    inout_AssetTypeNames.Insert("Collision Mesh (Convex)");
  }

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument,
    ezDocument*& out_pDocument) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  ezBitflags<ezAssetDocumentFlags> GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }

  virtual ezUInt64 ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const override;

private:
  ezDocumentTypeDescriptor m_AssetDesc;
  ezDocumentTypeDescriptor m_ConvexAssetDesc;
};
