#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>
#include <EditorFramework/Preferences/Preferences.h>

struct ezVisualScriptResourceDescriptor;
struct ezVisualScriptInstanceActivity;

struct ezVisualScriptActivityEvent
{
  ezUuid m_ObjectGuid;
  const ezVisualScriptInstanceActivity* m_pActivityData;
};

class ezVisualScriptAssetDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptAssetDocument, ezAssetDocument);

public:
  ezVisualScriptAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override { return "Visual Script"; }

  void HandleVsActivityMsg(const ezVisualScriptActivityMsgToEditor* pActivityMsg);
  void OnInterDocumentMessage(ezReflectedClass* pMessage, ezDocument* pSender) override;

  ezEvent<const ezVisualScriptActivityEvent&> m_ActivityEvents;
  ezEvent<ezReflectedClass*> m_InterDocumentMessages;

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;

  virtual void GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const override;
  virtual bool Copy(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const override;
  virtual bool Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType) override;

  virtual void InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph) override;

  ezResult GenerateVisualScriptDescriptor(ezVisualScriptResourceDescriptor& desc);

  void GetAllVsNodes(ezDynamicArray<const ezDocumentObject *> &allNodes) const;
  void HighlightConnections(const ezVisualScriptInstanceActivity& act);
};
