#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>

class ezPin;

class ezProcGenGraphAssetDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcGenGraphAssetDocument, ezAssetDocument);

public:
  ezProcGenGraphAssetDocument(const char* szDocumentPath);

  void SetDebugPin(const ezPin* pDebugPin);

  ezStatus WriteAsset(ezStreamWriter& inout_stream, const ezPlatformProfile* pAssetProfile, bool bAllowDebug) const;

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  virtual void GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const override;
  virtual bool Paste(
    const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType) override;

  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable) override;

  void GetAllOutputNodes(ezDynamicArray<const ezDocumentObject*>& placementNodes, ezDynamicArray<const ezDocumentObject*>& vertexColorNodes) const;

private:
  friend class ezProcGenAction;

  virtual void InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const override;

  struct GenerateContext;

  ezExpressionAST::Node* GenerateExpressionAST(const ezDocumentObject* outputNode, const char* szOutputName, GenerateContext& context, ezExpressionAST& out_Ast) const;
  ezExpressionAST::Node* GenerateDebugExpressionAST(GenerateContext& context, ezExpressionAST& out_Ast) const;

  void DumpSelectedOutput(bool bAst, bool bDisassembly) const;

  void CreateDebugNode();

  const ezPin* m_pDebugPin = nullptr;
  ezUniquePtr<ezProcGen_PlacementOutput> m_pDebugNode;
};
