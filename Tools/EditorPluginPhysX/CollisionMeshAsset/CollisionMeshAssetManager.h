#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <ToolsFoundation/Basics/Status.h>

class ezCollisionMeshAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCollisionMeshAssetDocumentManager, ezAssetDocumentManager);

public:
  ezCollisionMeshAssetDocumentManager();
  ~ezCollisionMeshAssetDocumentManager();

  virtual ezString GetResourceTypeExtension() const override { return "ezPhysXMesh"; }

  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const override
  {
    inout_AssetTypeNames.Insert("Collision Mesh");
    inout_AssetTypeNames.Insert("Collision Mesh (Convex)");
  }

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual ezStatus InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const;
  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument);
  virtual void InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const;

  virtual bool GeneratesPlatformSpecificAssets() const override { return true; }
};

