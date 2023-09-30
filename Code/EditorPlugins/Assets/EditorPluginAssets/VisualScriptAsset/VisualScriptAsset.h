#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

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

class ezVisualScriptParameterString : public ezVisualScriptParameter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptParameterString, ezVisualScriptParameter);

public:
  ezString m_DefaultValue;
};

class ezVisualScriptAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptAssetProperties, ezReflectedClass);

public:
  ezDynamicArray<ezVisualScriptParameterBool> m_BoolParameters;
  ezDynamicArray<ezVisualScriptParameterNumber> m_NumberParameters;
  ezDynamicArray<ezVisualScriptParameterString> m_StringParameters;
};

class ezVisualScriptAssetDocument : public ezSimpleAssetDocument<ezVisualScriptAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptAssetDocument, ezSimpleAssetDocument<ezVisualScriptAssetProperties>);

public:
  ezVisualScriptAssetDocument(ezStringView sDocumentPath);

  void HandleVsActivityMsg(const ezVisualScriptActivityMsgToEditor* pActivityMsg);
  void OnInterDocumentMessage(ezReflectedClass* pMessage, ezDocument* pSender) override;

  ezEvent<const ezVisualScriptActivityEvent&> m_ActivityEvents;
  ezEvent<ezReflectedClass*> m_InterDocumentMessages;

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;

  virtual void GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const override;
  virtual bool Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, ezStringView sMimeType) override;

  virtual void InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable) override;

  ezResult GenerateVisualScriptDescriptor(ezVisualScriptResourceDescriptor& desc);

  void GetAllVsNodes(ezDynamicArray<const ezDocumentObject*>& allNodes) const;
  void HighlightConnections(const ezVisualScriptInstanceActivity& act);
};
