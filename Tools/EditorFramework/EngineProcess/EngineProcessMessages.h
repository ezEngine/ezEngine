#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/IPC/ProcessCommunication.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <Core/Application/Config/FileSystemConfig.h>

///////////////////////////////////// ezProcessMessages ///////////////////////////////////// 


class EZ_EDITORFRAMEWORK_DLL ezSyncWithProcessMsgToEngine : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSyncWithProcessMsgToEngine);
};

class EZ_EDITORFRAMEWORK_DLL ezSyncWithProcessMsgToEditor : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSyncWithProcessMsgToEditor);
};

///////////////////////////////////// ezEditorEngineMsg ///////////////////////////////////// 

/// \brief Base class for all messages between editor and engine that are not bound to any document
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
  ezReflectedTypeDescriptor m_desc;
};

class EZ_EDITORFRAMEWORK_DLL ezSetupProjectMsgToEngine : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSetupProjectMsgToEngine);

public:
  ezString m_sProjectDir;
  ezApplicationFileSystemConfig m_Config;
};

class EZ_EDITORFRAMEWORK_DLL ezProjectReadyMsgToEditor : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProjectReadyMsgToEditor);

public:

};

class EZ_EDITORFRAMEWORK_DLL ezSimpleConfigMsgToEngine : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSimpleConfigMsgToEngine);

public:
  ezString m_sWhatToDo;

};

///////////////////////////////////// ezEditorEngineDocumentMsg ///////////////////////////////////// 

/// \brief Base class for all messages that are tied to some document.
class EZ_EDITORFRAMEWORK_DLL ezEditorEngineDocumentMsg : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineDocumentMsg);

public:
  ezUuid m_DocumentGuid;
};

class EZ_EDITORFRAMEWORK_DLL ezEditorEngineViewMsg : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineViewMsg);

public:
  ezEditorEngineViewMsg()
  {
    m_uiViewID = 0xFFFFFFFF;
  }

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
  ezString m_sDocumentType;
};

class EZ_EDITORFRAMEWORK_DLL ezDocumentOpenResponseMsgToEditor : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentOpenResponseMsgToEditor);

public:
  ezDocumentOpenResponseMsgToEditor()
  {
  }
};

class EZ_EDITORFRAMEWORK_DLL ezViewDestroyedMsgToEngine : public ezEditorEngineViewMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewDestroyedMsgToEngine);

};

class EZ_EDITORFRAMEWORK_DLL ezViewRedrawMsgToEngine : public ezEditorEngineViewMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewRedrawMsgToEngine);

public:

  ezUInt64 m_uiHWND;
  ezUInt16 m_uiWindowWidth;
  ezUInt16 m_uiWindowHeight;
  bool m_bUpdatePickingData;
};

class EZ_EDITORFRAMEWORK_DLL ezViewCameraMsgToEngine : public ezEditorEngineViewMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewCameraMsgToEngine);

public:
  ezInt8 m_iCameraMode;
  float m_fNearPlane;
  float m_fFarPlane;
  float m_fFovOrDim;
  ezUInt8 m_uiRenderMode; ///< ezViewRenderMode::Enum

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
  ezUuid m_ObjectGuid;
  ezUuid m_PreviousParentGuid;
  ezUuid m_NewParentGuid;
  ezString m_sObjectType;
  ezString m_sParentProperty;
  ezVariant m_PropertyIndex;

  ezString m_sObjectData;
};

class EZ_EDITORFRAMEWORK_DLL ezViewPickingMsgToEngine : public ezEditorEngineViewMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewPickingMsgToEngine);

public:

  ezUInt16 m_uiPickPosX;
  ezUInt16 m_uiPickPosY;
};

class EZ_EDITORFRAMEWORK_DLL ezViewPickingResultMsgToEditor : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewPickingResultMsgToEditor);

public:
  ezUuid m_ObjectGuid;
  ezUuid m_ComponentGuid;
  ezUuid m_OtherGuid;
  ezUInt32 m_uiPartIndex;

  ezVec3 m_vPickedPosition;
  ezVec3 m_vPickedNormal;
  ezVec3 m_vPickingRayStartPosition;
};

class ezEditorEngineConnection;

class EZ_EDITORFRAMEWORK_DLL ezViewHighlightMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewHighlightMsgToEngine);

public:
  void SendHighlightObjectMessage(ezEditorEngineConnection* pConnection);

  ezUuid m_HighlightObject;
  // currently used for highlighting which object the mouse hovers over
  // extend this message if other types of highlighting become necessary
};

class EZ_EDITORFRAMEWORK_DLL ezLogMsgToEditor : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLogMsgToEditor);

public:
  ezString m_sText;
  ezString m_sTag;
  ezInt8 m_iMsgType;
  ezUInt8 m_uiIndentation;
};


class EZ_EDITORFRAMEWORK_DLL ezEditorEngineSyncObjectMsg : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineSyncObjectMsg);

public:
  
  ezUuid m_ObjectGuid;
  ezString m_sObjectType;
  ezString m_sObjectData;

  const char* GetObjectData() const { return m_sObjectData; }
  void SetObjectData(const char* s) { m_sObjectData = s; }
};

class EZ_EDITORFRAMEWORK_DLL ezObjectTagMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezObjectTagMsgToEngine);

public:

  ezUuid m_ObjectGuid;
  ezString m_sTag;
  bool m_bSetTag;
};

class EZ_EDITORFRAMEWORK_DLL ezObjectSelectionMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezObjectSelectionMsgToEngine);

public:

  ezString m_sSelection;
};

class EZ_EDITORFRAMEWORK_DLL ezQuerySelectionBBoxMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezQuerySelectionBBoxMsgToEngine);
public:
  ezUInt32 m_uiViewID; /// passed through to ezQuerySelectionBBoxResultMsgToEditor
  ezInt32 m_iPurpose; /// passed through to ezQuerySelectionBBoxResultMsgToEditor
};

class EZ_EDITORFRAMEWORK_DLL ezQuerySelectionBBoxResultMsgToEditor : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezQuerySelectionBBoxResultMsgToEditor);
public:
  ezVec3 m_vCenter;
  ezVec3 m_vHalfExtents;

  ezUInt32 m_uiViewID; /// passed through from ezQuerySelectionBBoxMsgToEngine
  ezInt32 m_iPurpose; /// passed through from ezQuerySelectionBBoxMsgToEngine
};

