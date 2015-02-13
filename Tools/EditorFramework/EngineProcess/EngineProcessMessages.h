#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/IPC/ProcessCommunication.h>

///////////////////////////////////// ezEditorEngineMsg ///////////////////////////////////// 

class EZ_EDITORFRAMEWORK_DLL ezEditorEngineMsg : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineMsg);

public:
  ezEditorEngineMsg()
  {
  }

};

class EZ_EDITORFRAMEWORK_DLL ezUpdateReflectionTypeMsgToEditor : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezUpdateReflectionTypeMsgToEditor);

public:
  
  ezUInt32 m_uiNumProperties;
  ezString m_sTypeName;
  ezString m_sPluginName;
  ezString m_sParentTypeName;
  ezString m_sDefaultInitialization;
};

class EZ_EDITORFRAMEWORK_DLL ezUpdateReflectionPropertyMsgToEditor : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezUpdateReflectionPropertyMsgToEditor);

public:

  ezUInt32 m_uiPropertyIndex;
  ezString m_sName;
  ezString m_sType;
  ezUInt32 m_Type;
  ezUInt32 m_Flags;
  ezVariant m_ConstantValue;
};

///////////////////////////////////// ezEditorEngineDocumentMsg ///////////////////////////////////// 

class EZ_EDITORFRAMEWORK_DLL ezEditorEngineDocumentMsg : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineDocumentMsg);

public:
  ezEditorEngineDocumentMsg()
  {
    m_uiViewID = 0xFFFFFFFF;
  }

  ezUuid m_DocumentGuid;
  ezUInt32 m_uiViewID;
};

class EZ_EDITORFRAMEWORK_DLL ezDocumentOpenMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentOpenMsgToEngine);

public:
  ezDocumentOpenMsgToEngine()
  {
    m_bDocumentOpen = false;
  }

  bool m_bDocumentOpen;
};

class EZ_EDITORFRAMEWORK_DLL ezDocumentOpenResponseMsgToEditor : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentOpenResponseMsgToEditor);

public:
  ezDocumentOpenResponseMsgToEditor()
  {
  }
};

class EZ_EDITORFRAMEWORK_DLL ezViewRedrawMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewRedrawMsgToEngine);

public:

  ezUInt32 m_uiHWND;
  ezUInt16 m_uiWindowWidth;
  ezUInt16 m_uiWindowHeight;
};

class EZ_EDITORFRAMEWORK_DLL ezViewCameraMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewCameraMsgToEngine);

public:

  ezInt8 m_iCameraMode;
  float m_fNearPlane;
  float m_fFarPlane;
  float m_fFovOrDim;

  ezVec3 m_vPosition;
  ezVec3 m_vDirForwards;
  ezVec3 m_vDirUp;
  ezVec3 m_vDirRight;
  ezMat4 m_ViewMatrix;
  ezMat4 m_ProjMatrix;
};

class EZ_EDITORFRAMEWORK_DLL ezEntityMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEntityMsgToEngine);

public:
  enum Type
  {
    ObjectAdded,
    ObjectRemoved,
    ObjectMoved,
    PropertyChanged,
  };

  ezInt8 m_iMsgType;
  ezUInt16 m_uiNewChildIndex;
  ezUuid m_ObjectGuid;
  ezUuid m_PreviousParentGuid;
  ezUuid m_NewParentGuid;
  ezString m_sObjectData;

  const char* GetObjectData() const { return m_sObjectData; }
  void SetObjectData(const char* s) { m_sObjectData = s; }
};

class EZ_EDITORFRAMEWORK_DLL ezLogMsgToEditor : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLogMsgToEditor);

public:

  //const char* GetText() const { return m_sText; }
  //void SetText(const char* sz) { m_sText = sz; }

  ezString m_sText;
};







