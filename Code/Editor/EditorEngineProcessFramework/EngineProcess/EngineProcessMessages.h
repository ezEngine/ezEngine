#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/LogEntry.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

///////////////////////////////////// ezProcessMessages /////////////////////////////////////



///////////////////////////////////// ezEditorEngineMsg /////////////////////////////////////

/// \brief Base class for all messages between editor and engine that are not bound to any document
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEditorEngineMsg : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineMsg, ezProcessMessage);

public:
  ezEditorEngineMsg() = default;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezUpdateReflectionTypeMsgToEditor : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezUpdateReflectionTypeMsgToEditor, ezEditorEngineMsg);

public:
  // Mutable because it is eaten up by ezPhantomRttiManager.
  mutable ezReflectedTypeDescriptor m_desc;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezSetupProjectMsgToEngine : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSetupProjectMsgToEngine, ezEditorEngineMsg);

public:
  ezString m_sProjectDir;
  ezApplicationFileSystemConfig m_FileSystemConfig;
  ezApplicationPluginConfig m_PluginConfig;
  ezString m_sFileserveAddress; ///< Optionally used for remote processes to tell them with which IP address to connect to the host
  ezString m_sAssetProfile;
  float m_fDevicePixelRatio = 1.0f;
};

/// \brief Sent to remote processes to shut them down.
/// Local processes are simply killed through QProcess::close, but remote processes have to close themselves.
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezShutdownProcessMsgToEngine : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShutdownProcessMsgToEngine, ezEditorEngineMsg);

public:
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezProjectReadyMsgToEditor : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProjectReadyMsgToEditor, ezEditorEngineMsg);

public:
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezSimpleConfigMsgToEngine : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSimpleConfigMsgToEngine, ezEditorEngineMsg);

public:
  ezString m_sWhatToDo;
  ezString m_sPayload;
  double m_fPayload;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezSaveProfilingResponseToEditor : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSaveProfilingResponseToEditor, ezEditorEngineMsg);

public:
  ezString m_sProfilingFile;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezReloadResourceMsgToEngine : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezReloadResourceMsgToEngine, ezEditorEngineMsg);

public:
  ezString m_sResourceType;
  ezString m_sResourceID;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezResourceUpdateMsgToEngine : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezResourceUpdateMsgToEngine, ezEditorEngineMsg);

public:
  ezString m_sResourceType;
  ezString m_sResourceID;
  ezDataBuffer m_Data;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezRestoreResourceMsgToEngine : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRestoreResourceMsgToEngine, ezEditorEngineMsg);

public:
  ezString m_sResourceType;
  ezString m_sResourceID;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezChangeCVarMsgToEngine : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezChangeCVarMsgToEngine, ezEditorEngineMsg);

public:
  ezString m_sCVarName;
  ezVariant m_NewValue;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezConsoleCmdMsgToEngine : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConsoleCmdMsgToEngine, ezEditorEngineMsg);

public:
  ezInt8 m_iType; // 0 = execute, 1 = auto complete
  ezString m_sCommand;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezConsoleCmdResultMsgToEditor : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConsoleCmdResultMsgToEditor, ezEditorEngineMsg);

public:
  ezString m_sResult;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezDynamicStringEnumMsgToEditor : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDynamicStringEnumMsgToEditor, ezEditorEngineMsg);

public:
  ezString m_sEnumName;
  ezHybridArray<ezString, 8> m_EnumValues;
};

///////////////////////////////////// ezEditorEngineDocumentMsg /////////////////////////////////////

/// \brief Base class for all messages that are tied to some document.
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEditorEngineDocumentMsg : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineDocumentMsg, ezProcessMessage);

public:
  ezUuid m_DocumentGuid;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezSimpleDocumentConfigMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSimpleDocumentConfigMsgToEngine, ezEditorEngineDocumentMsg);

public:
  ezString m_sWhatToDo;
  ezString m_sPayload;
  ezVariant m_PayloadValue;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezSimpleDocumentConfigMsgToEditor : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSimpleDocumentConfigMsgToEditor, ezEditorEngineDocumentMsg);

public:
  ezString m_sWhatToDo;
  ezString m_sPayload;
  ezVariant m_PayloadValue;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezSyncWithProcessMsgToEngine : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSyncWithProcessMsgToEngine, ezProcessMessage);

public:
  ezUInt32 m_uiRedrawCount;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezSyncWithProcessMsgToEditor : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSyncWithProcessMsgToEditor, ezProcessMessage);

public:
  ezUInt32 m_uiRedrawCount;
};


class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEditorEngineViewMsg : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineViewMsg, ezEditorEngineDocumentMsg);

public:
  ezEditorEngineViewMsg() { m_uiViewID = 0xFFFFFFFF; }

  ezUInt32 m_uiViewID;
};

/// \brief For very simple uses cases where a custom message would be too much
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezDocumentConfigMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentConfigMsgToEngine, ezEditorEngineDocumentMsg);

public:
  ezString m_sWhatToDo;
  int m_iValue;
  float m_fValue;
  ezString m_sValue;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezDocumentOpenMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentOpenMsgToEngine, ezEditorEngineDocumentMsg);

public:
  ezDocumentOpenMsgToEngine() { m_bDocumentOpen = false; }

  bool m_bDocumentOpen;
  ezString m_sDocumentType;
  ezVariant m_DocumentMetaData;
};

/// \brief Used to reset the engine side to an empty document before sending the full document state over
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezDocumentClearMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentClearMsgToEngine, ezEditorEngineDocumentMsg);

public:
  ezDocumentClearMsgToEngine() = default;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezDocumentOpenResponseMsgToEditor : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentOpenResponseMsgToEditor, ezEditorEngineDocumentMsg);

public:
  ezDocumentOpenResponseMsgToEditor() = default;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezViewDestroyedMsgToEngine : public ezEditorEngineViewMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewDestroyedMsgToEngine, ezEditorEngineViewMsg);
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezViewDestroyedResponseMsgToEditor : public ezEditorEngineViewMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewDestroyedResponseMsgToEditor, ezEditorEngineViewMsg);
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezViewRedrawMsgToEngine : public ezEditorEngineViewMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewRedrawMsgToEngine, ezEditorEngineViewMsg);

public:
  ezUInt64 m_uiHWND;
  ezUInt16 m_uiWindowWidth;
  ezUInt16 m_uiWindowHeight;
  bool m_bUpdatePickingData;
  bool m_bEnablePickingSelected;
  bool m_bEnablePickTransparent;
  bool m_bUseCameraTransformOnDevice = true;

  ezInt8 m_iCameraMode; ///< ezCameraMode::Enum
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

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezViewScreenshotMsgToEngine : public ezEditorEngineViewMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewScreenshotMsgToEngine, ezEditorEngineViewMsg);

public:
  ezString m_sOutputFile;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezActivateRemoteViewMsgToEngine : public ezEditorEngineViewMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActivateRemoteViewMsgToEngine, ezEditorEngineViewMsg);

public:
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEntityMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEntityMsgToEngine, ezEditorEngineDocumentMsg);

public:
  ezObjectChange m_change;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezExportDocumentMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExportDocumentMsgToEngine, ezEditorEngineDocumentMsg);

public:
  ezExportDocumentMsgToEngine() = default;

  ezString m_sOutputFile;
  ezUInt64 m_uiAssetHash = 0;
  ezUInt16 m_uiVersion = 0;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezExportDocumentMsgToEditor : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExportDocumentMsgToEditor, ezEditorEngineDocumentMsg);

public:
  bool m_bOutputSuccess = false;
  ezString m_sFailureMsg;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezCreateThumbnailMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCreateThumbnailMsgToEngine, ezEditorEngineDocumentMsg);

public:
  ezUInt16 m_uiWidth = 0;
  ezUInt16 m_uiHeight = 0;
  ezHybridArray<ezString, 1> m_ViewExcludeTags;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezCreateThumbnailMsgToEditor : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCreateThumbnailMsgToEditor, ezEditorEngineDocumentMsg);

public:
  ezCreateThumbnailMsgToEditor() = default;
  ezDataBuffer m_ThumbnailData; ///< Raw 8-bit RGBA data (256x256x4 bytes)
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezViewPickingMsgToEngine : public ezEditorEngineViewMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewPickingMsgToEngine, ezEditorEngineViewMsg);

public:
  ezUInt16 m_uiPickPosX;
  ezUInt16 m_uiPickPosY;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezViewPickingResultMsgToEditor : public ezEditorEngineViewMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewPickingResultMsgToEditor, ezEditorEngineViewMsg);

public:
  ezUuid m_ObjectGuid;
  ezUuid m_ComponentGuid;
  ezUuid m_OtherGuid;
  ezUInt32 m_uiPartIndex;

  ezVec3 m_vPickedPosition;
  ezVec3 m_vPickedNormal;
  ezVec3 m_vPickingRayStartPosition;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezViewMarqueePickingMsgToEngine : public ezEditorEngineViewMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewMarqueePickingMsgToEngine, ezEditorEngineViewMsg);

public:
  ezUInt16 m_uiPickPosX0;
  ezUInt16 m_uiPickPosY0;

  ezUInt16 m_uiPickPosX1;
  ezUInt16 m_uiPickPosY1;

  ezUInt8 m_uiWhatToDo; // 0 == select, 1 == add, 2 == remove
  ezUInt32 m_uiActionIdentifier;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezViewMarqueePickingResultMsgToEditor : public ezEditorEngineViewMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewMarqueePickingResultMsgToEditor, ezEditorEngineViewMsg);

public:
  ezDynamicArray<ezUuid> m_ObjectGuids;
  ezUInt8 m_uiWhatToDo; // 0 == select, 1 == add, 2 == remove
  ezUInt32 m_uiActionIdentifier;
};


class ezEditorEngineConnection;

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezViewHighlightMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewHighlightMsgToEngine, ezEditorEngineDocumentMsg);

public:
  ezUuid m_HighlightObject;
  // currently used for highlighting which object the mouse hovers over
  // extend this message if other types of highlighting become necessary
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLogMsgToEditor : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLogMsgToEditor, ezEditorEngineMsg);

public:
  ezLogEntry m_Entry;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezCVarMsgToEditor : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCVarMsgToEditor, ezEditorEngineMsg);

public:
  ezString m_sName;
  ezString m_sPlugin;
  ezString m_sDescription;
  ezVariant m_Value;
};


class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpReplicationMsg : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpReplicationMsg, ezEditorEngineMsg);

public:
  ezUuid m_OperationGuid;
  ezUuid m_DocumentGuid;
  ezString m_sReplicationType;
  ezDataBuffer m_ReplicationData;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpProgressMsg : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpProgressMsg, ezEditorEngineMsg);

public:
  ezUuid m_OperationGuid;
  float m_fCompletion;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpResultMsg : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpResultMsg, ezEditorEngineMsg);

public:
  ezUuid m_OperationGuid;
  bool m_bSuccess;
  ezDataBuffer m_ResultData;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEditorEngineSyncObjectMsg : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineSyncObjectMsg, ezEditorEngineDocumentMsg);

public:
  ezUuid m_ObjectGuid;
  ezString m_sObjectType;
  ezDataBuffer m_ObjectData;

  const ezDataBuffer& GetObjectData() const { return m_ObjectData; }
  void SetObjectData(const ezDataBuffer& s) { m_ObjectData = s; }
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezObjectTagMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezObjectTagMsgToEngine, ezEditorEngineDocumentMsg);

public:
  ezObjectTagMsgToEngine()
  {
    m_bSetTag = false;
    m_bApplyOnAllChildren = false;
  }

  ezUuid m_ObjectGuid;
  ezString m_sTag;
  bool m_bSetTag;
  bool m_bApplyOnAllChildren;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezObjectSelectionMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezObjectSelectionMsgToEngine, ezEditorEngineDocumentMsg);

public:
  ezString m_sSelection;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezSimulationSettingsMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSimulationSettingsMsgToEngine, ezEditorEngineDocumentMsg);

public:
  bool m_bSimulateWorld = false;
  float m_fSimulationSpeed = 1.0f;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezGridSettingsMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGridSettingsMsgToEngine, ezEditorEngineDocumentMsg);

public:
  float m_fGridDensity = 0.0f;
  ezVec3 m_vGridCenter;
  ezVec3 m_vGridTangent1;
  ezVec3 m_vGridTangent2;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezGlobalSettingsMsgToEngine : public ezEditorEngineMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGlobalSettingsMsgToEngine, ezEditorEngineMsg);

public:
  float m_fGizmoScale = 0.0f;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezWorldSettingsMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezWorldSettingsMsgToEngine, ezEditorEngineDocumentMsg);

public:
  bool m_bRenderOverlay = false;
  bool m_bRenderShapeIcons = false;
  bool m_bRenderSelectionBoxes = false;
  bool m_bAddAmbientLight = false;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezGameModeMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameModeMsgToEngine, ezEditorEngineDocumentMsg);

public:
  bool m_bEnablePTG = false;
  bool m_bUseStartPosition = false;
  ezVec3 m_vStartPosition;
  ezVec3 m_vStartDirection;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezGameModeMsgToEditor : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameModeMsgToEditor, ezEditorEngineDocumentMsg);

public:
  bool m_bRunningPTG;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezQuerySelectionBBoxMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezQuerySelectionBBoxMsgToEngine, ezEditorEngineDocumentMsg);

public:
  ezUInt32 m_uiViewID; /// passed through to ezQuerySelectionBBoxResultMsgToEditor
  ezInt32 m_iPurpose;  /// passed through to ezQuerySelectionBBoxResultMsgToEditor
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezQuerySelectionBBoxResultMsgToEditor : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezQuerySelectionBBoxResultMsgToEditor, ezEditorEngineDocumentMsg);

public:
  ezVec3 m_vCenter;
  ezVec3 m_vHalfExtents;

  ezUInt32 m_uiViewID; /// passed through from ezQuerySelectionBBoxMsgToEngine
  ezInt32 m_iPurpose;  /// passed through from ezQuerySelectionBBoxMsgToEngine
};

/// \brief Send between editor documents, such that one document can know about objects in another document.
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezGatherObjectsOfTypeMsgInterDoc : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGatherObjectsOfTypeMsgInterDoc, ezReflectedClass);

public:
  const ezRTTI* m_pType;

  struct Result
  {
    const ezDocument* m_pDocument;
    ezUuid m_ObjectGuid;
    ezString m_sDisplayName;
  };

  ezDynamicArray<Result> m_Results;
};

/// Send by the editor scene document to all other editor documents, to gather on which objects debug visualization should be enabled during
/// play-the-game.
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezGatherObjectsForDebugVisMsgInterDoc : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGatherObjectsForDebugVisMsgInterDoc, ezReflectedClass);

public:
  ezDynamicArray<ezUuid> m_Objects;
};

/// Send by the editor scene document to the runtime scene document, to tell it about the poll results (see ezGatherObjectsForDebugVisMsgInterDoc).
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezObjectsForDebugVisMsgToEngine : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezObjectsForDebugVisMsgToEngine, ezEditorEngineDocumentMsg);

public:
  ezDataBuffer m_Objects;
};
