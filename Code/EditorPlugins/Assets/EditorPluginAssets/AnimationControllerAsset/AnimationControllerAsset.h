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

  bool m_bMultiInputPin = false;
  ezAnimGraphPin::Type m_DataType = ezAnimGraphPin::Invalid;
};

class ezAnimationControllerNodeManager : public ezDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& ref_types) const override;

  virtual ezStatus InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_result) const override;
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

  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  virtual void GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const override;
  virtual bool Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType) override;

  virtual void InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable) override;
};
