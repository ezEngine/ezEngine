#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class ezRenderPipelineNodeManager : public ezDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node) override;
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override;

  virtual ezStatus InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_Result) const override;
};

class ezRenderPipelineAssetDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelineAssetDocument, ezAssetDocument);

public:
  ezRenderPipelineAssetDocument(const char* szDocumentPath);

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  virtual void GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const override;
  virtual bool Paste(
    const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType) override;

  virtual void InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable) override;
};
