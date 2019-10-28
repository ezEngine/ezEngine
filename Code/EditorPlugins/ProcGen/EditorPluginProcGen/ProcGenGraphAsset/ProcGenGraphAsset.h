#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>

class ezDocumentObjectConverterWriter;
class ezProcGenNodeBase;
class ezProcGenPlacementOutput;
class ezPin;

class ezProcGenGraphSharedData;

class ezProcGenGraphAssetDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcGenGraphAssetDocument, ezAssetDocument);

public:
  ezProcGenGraphAssetDocument(const char* szDocumentPath);

  virtual const char* GetDocumentTypeDisplayString() const override { return "ProcGen Graph Asset"; }

  virtual const char* QueryAssetType() const override { return "ProcGen Graph"; }

  void SetDebugPin(const ezPin* pDebugPin);

  ezStatus WriteAsset(ezStreamWriter& stream, const ezPlatformProfile* pAssetProfile) const;

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  virtual void GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const override;
  virtual bool Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition,
    const char* szMimeType) override;

  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable) override;

  void GetAllOutputNodes(
    ezDynamicArray<const ezDocumentObject*>& placementNodes, ezDynamicArray<const ezDocumentObject*>& vertexColorNodes) const;

private:
  friend class ezProcGenAction;

  struct CachedNode
  {
    ezProcGenNodeBase* m_pPPNode = nullptr;
  };

  ezExpressionAST::Node* GenerateExpressionAST(const ezDocumentObject* outputNode, ezDocumentObjectConverterWriter& objectWriter,
    ezRttiConverterReader& rttiConverter, ezHashTable<const ezDocumentObject*, CachedNode>& nodeCache, ezExpressionAST& out_Ast,
    ezProcGenNodeBase::GenerateASTContext& context) const;

  ezExpressionAST::Node* GenerateDebugExpressionAST(ezDocumentObjectConverterWriter& objectWriter, ezRttiConverterReader& rttiConverter,
    ezHashTable<const ezDocumentObject*, CachedNode>& nodeCache, ezExpressionAST& out_Ast, ezProcGenNodeBase::GenerateASTContext& context) const;

  void DumpSelectedOutput(bool bAst, bool bDisassembly) const;

  void CreateDebugNode();

  const ezPin* m_pDebugPin = nullptr;
  ezUniquePtr<ezProcGenPlacementOutput> m_pDebugNode;
};
