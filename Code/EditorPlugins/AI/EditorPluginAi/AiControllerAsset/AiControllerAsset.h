#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

class ezAiInputPinAttribute;

/// A pin of a node in an AI controller graph.
///
/// The data type shown for a pin is the accepted type (input pins), the produced perception type (generator output pins)
/// or the node type (sensor and consideration output pins).
class ezAiControllerPin : public ezVisualGraphPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiControllerPin, ezVisualGraphPin);

public:
  ezAiControllerPin(Type type, ezStringView sName, const ezColorGammaUB& color, const ezDocumentObject* pObject, const ezRTTI* pDataType);

  virtual ezStringView GetDataTypeName() const override;

  const ezRTTI* m_pDataType = nullptr;
  const ezRTTI* m_pNodeType = nullptr;                      ///< Output pins only.
  const ezAiInputPinAttribute* m_pInputAttribute = nullptr; ///< Input pins only.
};

/// Object manager for AI controller graphs.
///
/// The nodes are the runtime sensor, perception generator, behavior and consideration types (anything derived from ezAiSensor,
/// ezAiPerceptionGenerator, ezAiBehavior or ezAiConsideration). All but behaviors have one output pin ("Out").
/// Every ezAiInputPin member of a node becomes an input pin with the property name, accepting nodes of the type
/// from its ezAiInputPinAttribute.
class ezAiControllerNodeManager : public ezVisualGraphObjectManager
{
public:
  enum class NodeKind
  {
    None,
    Sensor,
    PerceptionGenerator,
    Behavior,
    Consideration,
  };

  static NodeKind GetNodeKind(const ezRTTI* pType);

  /// The type name without the 'ezAiSensor', 'ezAiPerceptionGen', 'ezAiBehavior' or 'ezAiConsideration' prefix.
  static ezStringView GetShortTypeName(const ezRTTI* pType);

  virtual void GetNodeCreationTemplates(ezDynamicArray<ezVisualGraphNodeDesc>& out_templates) const override;

private:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual ezStatus InternalCanConnect(const ezVisualGraphPin& source, const ezVisualGraphPin& target, CanConnectResult& out_result) const override;
  virtual void GetCreateableTypes(ezDynamicArray<const ezRTTI*>& out_types) const override;
};

/// Asset document for an ezAiControllerResource.
///
/// The transform builds an ezAiControllerDesc from the nodes (in node creation order), fills the ezAiInputPin members
/// with the indices of the connected nodes and writes it with ezAiControllerDesc::Serialize().
class ezAiControllerAssetDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiControllerAssetDocument, ezAssetDocument);

public:
  ezAiControllerAssetDocument(ezStringView sDocumentPath);

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  virtual void GetSupportedMimeTypesForPasting(ezDynamicArray<ezString>& out_mimeTypes) const override;
  virtual bool CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const override;
  virtual bool Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, ezStringView sMimeType) override;

  virtual void InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable) override;
};
