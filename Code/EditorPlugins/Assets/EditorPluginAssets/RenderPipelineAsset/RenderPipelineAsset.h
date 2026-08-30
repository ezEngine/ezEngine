#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

/// Declares the type of pin to prevent connecting textures to buffers.
struct ezRenderPipelineResourceType
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Texture,
    Buffer,
    Default = Texture,
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezRenderPipelineResourceType);


/// Stored inside ezRenderPipelineAssetMetaData to declare an input / output pin of a sub-graph.
struct ezRenderPipelineAssetPinInfo
{
  ezEnum<ezRenderPipelineResourceType> m_ResourceType;
  ezString m_sName;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezRenderPipelineAssetPinInfo);


// Metadata attached to a ezRenderPipelineAssetDocument to declare a sub-graph's input / output.
class ezRenderPipelineAssetMetaData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelineAssetMetaData, ezReflectedClass);

public:
  ezDynamicArray<ezRenderPipelineAssetPinInfo> m_Inputs;
  ezDynamicArray<ezRenderPipelineAssetPinInfo> m_Outputs;
};


/// Custom pin class so that InternalCanConnect can prevent connecting textures to buffers.
class ezRenderPipelineNodeGraphPin : public ezVisualGraphPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelineNodeGraphPin, ezVisualGraphPin);

public:
  ezRenderPipelineNodeGraphPin(ezVisualGraphPin::Type type, const char* szName, const ezColorGammaUB& color, const ezDocumentObject* pObject, ezRenderPipelineResourceType::Enum resourceType);
  ~ezRenderPipelineNodeGraphPin();

  ezRenderPipelineResourceType::Enum m_ResourceType = ezRenderPipelineResourceType::Texture;
};


/// Object manager for render pipeline graphs.
///
/// Manages the node graph that defines a rendering pipeline, including render passes, resources, and their connections.
/// Validates connections to ensure render pipeline integrity.
class ezRenderPipelineNodeManager : public ezVisualGraphObjectManager
{
public:
  ezRenderPipelineNodeManager();
  ~ezRenderPipelineNodeManager();

  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(ezDynamicArray<const ezRTTI*>& out_types) const override;

  virtual ezStatus InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObject* pParent, ezStringView sParentProperty, const ezVariant& index) const override;
  virtual ezStatus InternalCanConnect(const ezVisualGraphPin& source, const ezVisualGraphPin& target, CanConnectResult& out_result) const override;

  virtual bool InternalIsDynamicPinProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProp) const override;

private:
  struct SubGraphCache
  {
    const ezDocumentObject* m_pObject = nullptr;
    ezUuid m_SourceAssetGuid;
    ezUInt64 m_uiMetaDataHash = 0;
  };

  void AssetCuratorEventHandler(const ezAssetCuratorEvent& e);
  void NodeEventHandler(const ezVisualGraphObjectManagerEvent& e);

  ezMap<ezUuid, SubGraphCache> m_SubGraphs;
};


class ezRenderPipelineAssetDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelineAssetDocument, ezAssetDocument);

public:
  ezRenderPipelineAssetDocument(ezStringView sDocumentPath);
  ~ezRenderPipelineAssetDocument();

protected:
  virtual ezTransformStatus InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  // Copy & Paste support
  virtual void GetSupportedMimeTypesForPasting(ezDynamicArray<ezString>& out_mimeTypes) const override;
  virtual bool CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const override;
  virtual bool Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, ezStringView sMimeType) override;

  ezStatus Validate() const;

  // meta data stores node positions and the pipeline's input/output interface
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
  virtual void InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable) override;
};
