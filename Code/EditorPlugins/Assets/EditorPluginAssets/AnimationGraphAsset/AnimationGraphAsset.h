#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

using ezAnimationClipResourceHandle = ezTypedResourceHandle<class ezAnimationClipResource>;

class ezAnimGraphInstance;
class ezAnimGraphNode;

class ezAnimationGraphNodePin : public ezPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationGraphNodePin, ezPin);

public:
  ezAnimationGraphNodePin(Type type, const char* szName, const ezColorGammaUB& color, const ezDocumentObject* pObject);
  ~ezAnimationGraphNodePin();

  bool m_bMultiInputPin = false;
  ezAnimGraphPin::Type m_DataType = ezAnimGraphPin::Invalid;
};

class ezAnimationGraphNodeManager : public ezDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& ref_types) const override;

  virtual ezStatus InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_result) const override;

private:
  virtual bool InternalIsDynamicPinProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProp) const override;
};

class ezAnimationGraphAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationGraphAssetProperties, ezReflectedClass);

public:
  ezDynamicArray<ezString> m_IncludeGraphs;
  ezDynamicArray<ezAnimationClipMapping> m_AnimationClipMapping;
};

class ezAnimationGraphAssetDocument : public ezSimpleAssetDocument<ezAnimationGraphAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationGraphAssetDocument, ezSimpleAssetDocument<ezAnimationGraphAssetProperties>);

public:
  ezAnimationGraphAssetDocument(ezStringView sDocumentPath);

protected:
  struct PinCount
  {
    ezUInt16 m_uiInputCount = 0;
    ezUInt16 m_uiInputIdx = 0;
    ezUInt16 m_uiOutputCount = 0;
    ezUInt16 m_uiOutputIdx = 0;
  };

  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  virtual void GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const override;
  virtual bool Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, ezStringView sMimeType) override;

  virtual void InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable) override;
};
