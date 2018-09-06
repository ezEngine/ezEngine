#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <SharedPluginScene/Plugin.h>

class EZ_SHAREDPLUGINSCENE_DLL ezExposedSceneProperty : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExposedSceneProperty, ezReflectedClass);

public:
  ezString m_sName;
  ezUuid m_Object;
  ezString m_sPropertyPath;
};

class EZ_SHAREDPLUGINSCENE_DLL ezExposedDocumentObjectPropertiesMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExposedDocumentObjectPropertiesMsgToEngine, ezEditorEngineDocumentMsg);

public:
  ezDynamicArray<ezExposedSceneProperty> m_Properties;
};

class EZ_SHAREDPLUGINSCENE_DLL ezExportSceneGeometryMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExportSceneGeometryMsgToEngine, ezEditorEngineDocumentMsg);

public:
  bool m_bSelectionOnly = false;
  ezString m_sOutputFile;
  int m_iExtractionMode; // ezWorldGeoExtractionUtil::ExtractionMode
  ezMat3 m_Transform;
};

class EZ_SHAREDPLUGINSCENE_DLL ezPullObjectStateMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPullObjectStateMsgToEngine, ezEditorEngineDocumentMsg);
};

struct ezPushObjectStateData
{
  ezUuid m_ObjectGuid;
  ezVec3 m_vPosition;
  ezQuat m_qRotation;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_SHAREDPLUGINSCENE_DLL, ezPushObjectStateData);

class EZ_SHAREDPLUGINSCENE_DLL ezPushObjectStateMsgToEditor : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPushObjectStateMsgToEditor, ezEditorEngineDocumentMsg);

public:

  ezDynamicArray<ezPushObjectStateData> m_ObjectStates;
};
