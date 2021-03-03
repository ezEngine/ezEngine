#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class ezAnimGraph;
class ezAnimGraphNode;

class ezAnimationControllerNodePin : public ezPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationControllerNodePin, ezPin);

public:
  ezAnimationControllerNodePin(Type type, const char* szName, const ezColorGammaUB& color, const ezDocumentObject* pObject);
  ~ezAnimationControllerNodePin();

  ezAnimGraphPin::Type m_DataType = ezAnimGraphPin::Invalid;
};

class ezAnimationControllerNodeManager : public ezDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node) override;
  virtual void InternalDestroyPins(const ezDocumentObject* pObject, NodeInternal& node) override;
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override;

  virtual ezStatus InternalCanConnect(const ezPin* pSource, const ezPin* pTarget, CanConnectResult& out_Result) const override;
};

class ezAnimationControllerAssetDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationControllerAssetDocument, ezAssetDocument);

public:
  ezAnimationControllerAssetDocument(const char* szDocumentPath);

protected:
  struct PinCount
  {
    ezUInt16 m_uiInputCount = 0;
    ezUInt16 m_uiInputIdx = 0;
    ezUInt16 m_uiOutputCount = 0;
    ezUInt16 m_uiOutputIdx = 0;
  };

  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  void CountPinTypes(const ezDocumentNodeManager* pNodeManager, ezDynamicArray<const ezDocumentObject*>& allNodes, ezMap<ezUInt8, PinCount>& pinCounts) const;
  void CreateOutputGraphNodes(const ezDynamicArray<const ezDocumentObject*>& allNodes, ezAnimGraph& animController, ezDynamicArray<ezAnimGraphNode*>& newNodes) const;
  void SetInputPinIndices(const ezDynamicArray<ezAnimGraphNode*>& newNodes, const ezDynamicArray<const ezDocumentObject*>& allNodes, const ezDocumentNodeManager* pNodeManager, ezMap<ezUInt8, PinCount>& pinCounts, ezMap<const ezPin*, ezUInt16>& inputPinIndices, ezAbstractMemberProperty* pIdxProperty) const;
  void SetOutputPinIndices(const ezDynamicArray<ezAnimGraphNode*>& newNodes, const ezDynamicArray<const ezDocumentObject*>& allNodes, const ezDocumentNodeManager* pNodeManager, ezMap<ezUInt8, PinCount>& pinCounts, ezAnimGraph& animController, ezAbstractMemberProperty* pIdxProperty, const ezMap<const ezPin*, ezUInt16>& inputPinIndices) const;

  virtual void GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const override;
  virtual bool Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType) override;

  virtual void InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable) override;
};
