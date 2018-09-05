#pragma once

#include <SharedPluginScene/Plugin.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>

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
