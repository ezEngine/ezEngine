#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

/// Object manager for render pipeline graphs.
///
/// Manages the node graph that defines a rendering pipeline, including render passes, resources, and their connections.
/// Validates connections to ensure render pipeline integrity.
class ezRenderPipelineNodeManager : public ezVisualGraphObjectManager
{
public:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(ezDynamicArray<const ezRTTI*>& out_types) const override;

  virtual ezStatus InternalCanConnect(const ezVisualGraphPin& source, const ezVisualGraphPin& target, CanConnectResult& out_result) const override;
};

class ezRenderPipelineAssetDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelineAssetDocument, ezAssetDocument);

public:
  ezRenderPipelineAssetDocument(ezStringView sDocumentPath);
  ~ezRenderPipelineAssetDocument();

protected:
  virtual ezTransformStatus InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  virtual void GetSupportedMimeTypesForPasting(ezDynamicArray<ezString>& out_mimeTypes) const override;
  virtual bool CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const override;
  virtual bool Paste(
    const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, ezStringView sMimeType) override;

  virtual void InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable) override;
};
