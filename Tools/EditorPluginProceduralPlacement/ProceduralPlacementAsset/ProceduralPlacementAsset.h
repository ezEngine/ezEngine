#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <ProceduralPlacementPlugin/VM/ExpressionAST.h>

class ezDocumentObjectConverterWriter;
class ezProceduralPlacementNodeBase;

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

private:
  friend class ezProceduralPlacementAction;

  struct CachedNode
  {
    ezProceduralPlacementNodeBase* m_pPPNode = nullptr;
    ezExpressionAST::Node* m_pASTNode = nullptr;
  };

  ezExpressionAST::Node* GenerateExpressionAST(const ezDocumentObject* outputNode, ezDocumentObjectConverterWriter& objectWriter, ezRttiConverterReader& rttiConverter,
    ezHashTable<const ezDocumentObject*, CachedNode>& nodeCache, ezExpressionAST& out_Ast) const;

  void DumpSelectedOutput(bool bAst, bool bDisassembly) const;
};
