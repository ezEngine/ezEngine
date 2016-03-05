#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/IPC/ProcessCommunication.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <Core/Application/Config/FileSystemConfig.h>
#include <Core/Application/Config/PluginConfig.h>

///////////////////////////////////// ezProcessMessages ///////////////////////////////////// 


class EZ_EDITORFRAMEWORK_DLL ezSyncWithProcessMsgToEngine : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSyncWithProcessMsgToEngine, ezProcessMessage);
};

class EZ_EDITORFRAMEWORK_DLL ezSyncWithProcessMsgToEditor : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSyncWithProcessMsgToEditor, ezProcessMessage);
};

///////////////////////////////////// ezEditorEngineMsg ///////////////////////////////////// 

/// \brief Base class for all messages between editor and engine that are not bound to any document
class EZ_EDITORFRAMEWORK_DLL ezEditorEngineMsg : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineMsg, ezProcessMessage);

public:
  ezEditorEngineMsg()
  {
  }

};

class EZ_EDITORFRAMEWORK_DLL ezUpdateReflectionTypeMsgToEditor : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezUpdateReflectionTypeMsgToEditor, ezEditorEngineMsg);

public:
  ezReflectedTypeDescriptor m_desc;
};

class EZ_EDITORFRAMEWORK_DLL ezSetupProjectMsgToEngine : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSetupProjectMsgToEngine, ezEditorEngineMsg);

public:
  ezString m_sProjectDir;
  ezApplicationFileSystemConfig m_FileSystemConfig;
  ezApplicationPluginConfig m_PluginConfig;
};

class EZ_EDITORFRAMEWORK_DLL ezProjectReadyMsgToEditor : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProjectReadyMsgToEditor, ezEditorEngineMsg);

public:

};

class EZ_EDITORFRAMEWORK_DLL ezSimpleConfigMsgToEngine : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSimpleConfigMsgToEngine, ezEditorEngineMsg);

public:
  ezString m_sWhatToDo;

};

///////////////////////////////////// ezEditorEngineDocumentMsg ///////////////////////////////////// 

/// \brief Base class for all messages that are tied to some document.
class EZ_EDITORFRAMEWORK_DLL ezEditorEngineDocumentMsg : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineDocumentMsg, ezProcessMessage);

public:
  ezUuid m_DocumentGuid;
};

class EZ_EDITORFRAMEWORK_DLL ezEditorEngineViewMsg : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineViewMsg, ezEditorEngineDocumentMsg);

public:
  ezEditorEngineViewMsg()
  {
    m_uiViewID = 0xFFFFFFFF;
  }

  ezUInt32 m_uiViewID;
};


class EZ_EDITORFRAMEWORK_DLL ezDocumentOpenMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentOpenMsgToEngine, ezEditorEngineDocumentMsg);

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
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentOpenResponseMsgToEditor, ezEditorEngineDocumentMsg);

public:
  ezDocumentOpenResponseMsgToEditor()
  {
  }
};

class EZ_EDITORFRAMEWORK_DLL ezViewDestroyedMsgToEngine : public ezEditorEngineViewMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewDestroyedMsgToEngine, ezEditorEngineViewMsg);

};

class EZ_EDITORFRAMEWORK_DLL ezViewRedrawMsgToEngine : public ezEditorEngineViewMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewRedrawMsgToEngine, ezEditorEngineViewMsg);

public:

  ezUInt64 m_uiHWND;
  ezUInt16 m_uiWindowWidth;
  ezUInt16 m_uiWindowHeight;
  bool m_bUpdatePickingData;
  bool m_bEnablePickingSelected;

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
  EZ_ADD_DYNAMIC_REFLECTION(ezEntityMsgToEngine, ezEditorEngineDocumentMsg);

public:
  ezObjectChange m_change;
};

class EZ_EDITORFRAMEWORK_DLL ezExportSceneMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExportSceneMsgToEngine, ezEditorEngineDocumentMsg);

public:
  ezString m_sOutputFile;
  ezUInt64 m_uiAssetHash;
};

class EZ_EDITORFRAMEWORK_DLL ezExportSceneMsgToEditor : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExportSceneMsgToEditor, ezEditorEngineDocumentMsg);

public:

  bool m_bSuccess;
};

class EZ_EDITORFRAMEWORK_DLL ezViewPickingMsgToEngine : public ezEditorEngineViewMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewPickingMsgToEngine, ezEditorEngineViewMsg);

public:

  ezUInt16 m_uiPickPosX;
  ezUInt16 m_uiPickPosY;
};

class EZ_EDITORFRAMEWORK_DLL ezViewPickingResultMsgToEditor : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewPickingResultMsgToEditor, ezEditorEngineDocumentMsg);

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
  EZ_ADD_DYNAMIC_REFLECTION(ezViewHighlightMsgToEngine, ezEditorEngineDocumentMsg);

public:
  void SendHighlightObjectMessage(ezEditorEngineConnection* pConnection);

  ezUuid m_HighlightObject;
  // currently used for highlighting which object the mouse hovers over
  // extend this message if other types of highlighting become necessary
};

class EZ_EDITORFRAMEWORK_DLL ezLogMsgToEditor : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLogMsgToEditor, ezEditorEngineMsg);

public:
  ezString m_sText;
  ezString m_sTag;
  ezInt8 m_iMsgType;
  ezUInt8 m_uiIndentation;
};


class EZ_EDITORFRAMEWORK_DLL ezEditorEngineSyncObjectMsg : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineSyncObjectMsg, ezEditorEngineDocumentMsg);

public:
  
  ezUuid m_ObjectGuid;
  ezString m_sObjectType;
  ezString m_sObjectData;

  const char* GetObjectData() const { return m_sObjectData; }
  void SetObjectData(const char* s) { m_sObjectData = s; }
};

class EZ_EDITORFRAMEWORK_DLL ezObjectTagMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezObjectTagMsgToEngine, ezEditorEngineDocumentMsg);

public:

  ezUuid m_ObjectGuid;
  ezString m_sTag;
  bool m_bSetTag;
};

class EZ_EDITORFRAMEWORK_DLL ezObjectSelectionMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezObjectSelectionMsgToEngine, ezEditorEngineDocumentMsg);

public:

  ezString m_sSelection;
};

class EZ_EDITORFRAMEWORK_DLL ezSceneSettingsMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneSettingsMsgToEngine, ezEditorEngineDocumentMsg);

public:

  bool m_bSimulateWorld;
  float m_fSimulationSpeed;
  bool m_bRenderOverlay;
  bool m_bRenderShapeIcons;
};

class EZ_EDITORFRAMEWORK_DLL ezGameModeMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION( ezGameModeMsgToEngine, ezEditorEngineDocumentMsg);

public:
  bool m_bEnablePTG;
};

class EZ_EDITORFRAMEWORK_DLL ezGameModeMsgToEditor : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameModeMsgToEditor, ezEditorEngineDocumentMsg);

public:
  bool m_bRunningPTG;
};

class EZ_EDITORFRAMEWORK_DLL ezQuerySelectionBBoxMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezQuerySelectionBBoxMsgToEngine, ezEditorEngineDocumentMsg);
public:
  ezUInt32 m_uiViewID; /// passed through to ezQuerySelectionBBoxResultMsgToEditor
  ezInt32 m_iPurpose; /// passed through to ezQuerySelectionBBoxResultMsgToEditor
};

class EZ_EDITORFRAMEWORK_DLL ezQuerySelectionBBoxResultMsgToEditor : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezQuerySelectionBBoxResultMsgToEditor, ezEditorEngineDocumentMsg);
public:
  ezVec3 m_vCenter;
  ezVec3 m_vHalfExtents;

  ezUInt32 m_uiViewID; /// passed through from ezQuerySelectionBBoxMsgToEngine
  ezInt32 m_iPurpose; /// passed through from ezQuerySelectionBBoxMsgToEngine
};

