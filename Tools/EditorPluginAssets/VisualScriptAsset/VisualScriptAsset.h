#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

struct ezVisualScriptResourceDescriptor;
struct ezVisualScriptInstanceActivity;

struct ezVisualScriptActivityEvent
{
  ezUuid m_ObjectGuid;
  const ezVisualScriptInstanceActivity* m_pActivityData;
};

class ezVisualScriptParameter : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptParameter, ezReflectedClass);

public:
  ezString m_sName;
  bool m_bExpose = false;
};

class ezVisualScriptParameterBool : public ezVisualScriptParameter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptParameterBool, ezVisualScriptParameter);

public:
  bool m_DefaultValue = false;
};

class ezVisualScriptParameterNumber : public ezVisualScriptParameter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptParameterNumber, ezVisualScriptParameter);

public:
  double m_DefaultValue = 0;
};

class ezVisualScriptAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptAssetProperties, ezReflectedClass);

public:

  ezDynamicArray<ezVisualScriptParameterBool> m_BoolParameters;
  ezDynamicArray<ezVisualScriptParameterNumber> m_NumberParameters;
};

class ezVisualScriptAssetDocument : public ezSimpleAssetDocument<ezVisualScriptAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptAssetDocument, ezSimpleAssetDocument<ezVisualScriptAssetProperties>);

public:
  ezVisualScriptAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override { return "Visual Script"; }

  void HandleVsActivityMsg(const ezVisualScriptActivityMsgToEditor* pActivityMsg);
  void OnInterDocumentMessage(ezReflectedClass* pMessage, ezDocument* pSender) override;

  ezEvent<const ezVisualScriptActivityEvent&> m_ActivityEvents;
  ezEvent<ezReflectedClass*> m_InterDocumentMessages;

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezAssetProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;

  virtual void GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const override;
  virtual bool Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType) override;

  virtual void InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable) override;

  ezResult GenerateVisualScriptDescriptor(ezVisualScriptResourceDescriptor& desc);

  void GetAllVsNodes(ezDynamicArray<const ezDocumentObject *> &allNodes) const;
  void HighlightConnections(const ezVisualScriptInstanceActivity& act);


};
