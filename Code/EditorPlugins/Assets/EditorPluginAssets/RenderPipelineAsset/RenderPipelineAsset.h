#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class ezRenderPipelineNodeManager : public ezDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& ref_types) const override;

  virtual ezStatus InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_result) const override;
};

/// \brief This custom mirror additionally sends over the connection meta data as properties to the engine side so that the graph can be reconstructed there.
/// This is necessary as DocumentNodeManager_DefaultConnection does not contain any data, instead all data is in the DocumentNodeManager_ConnectionMetaData object.
class ezRenderPipelineObjectMirrorEditor : public ezIPCObjectMirrorEditor
{
  using SUPER = ezIPCObjectMirrorEditor;
public:
  void InitNodeSender(const ezDocumentNodeManager* pNodeManager);
  void DeInitNodeSender();
  virtual void ApplyOp(ezObjectChange& ref_change) override;

private:
  void NodeEventsHandler(const ezDocumentNodeManagerEvent& e);
  void SendConnection(const ezConnection& connection);

  const ezDocumentNodeManager* m_pNodeManager = nullptr;
};

class ezRenderPipelineAssetDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelineAssetDocument, ezAssetDocument);

public:
  ezRenderPipelineAssetDocument(const char* szDocumentPath);
  ~ezRenderPipelineAssetDocument();

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual ezTransformStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  virtual void GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const override;
  virtual bool Paste(
    const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType) override;

  virtual void InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable) override;
};
