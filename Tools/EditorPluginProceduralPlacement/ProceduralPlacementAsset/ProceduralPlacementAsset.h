#pragma once

#include <EditorFramework/Assets/AssetDocument.h>

class ezProceduralPlacementAssetDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProceduralPlacementAssetDocument, ezAssetDocument);

public:
  ezProceduralPlacementAssetDocument(const char* szDocumentPath);

  virtual const char* GetDocumentTypeDisplayString() const override { return "Procedural Placement Asset"; }

  virtual const char* QueryAssetType() const override { return "Procedural Placement"; }

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable) override;

  void GetAllOutputNodes(ezDynamicArray<const ezDocumentObject*>& allNodes) const;
};
