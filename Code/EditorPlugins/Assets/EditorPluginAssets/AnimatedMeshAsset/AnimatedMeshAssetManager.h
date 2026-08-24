#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class ezAnimatedMeshAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimatedMeshAssetDocumentManager, ezAssetDocumentManager);

public:
  ezAnimatedMeshAssetDocumentManager();
  ~ezAnimatedMeshAssetDocumentManager();

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual void InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }
  virtual void AppendAssetInfoSummary(ezStringBuilder& ref_sOut, const ezAssetInfoFile& info, ezStringView sLinePrefix = "\n"_ezsv) const override;

  ezAssetDocumentTypeDescriptor m_DocTypeDesc;
};
