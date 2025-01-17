#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>

// clang-format off

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSyncWithProcessMsgToEngine, 1, ezRTTIDefaultAllocator<ezSyncWithProcessMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RedrawCount", m_uiRedrawCount),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSyncWithProcessMsgToEditor, 1, ezRTTIDefaultAllocator<ezSyncWithProcessMsgToEditor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RedrawCount", m_uiRedrawCount),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

///////////////////////////////////// ezEditorEngineMsg /////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineMsg, 1, ezRTTINoAllocator )
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezUpdateReflectionTypeMsgToEditor, 1, ezRTTIDefaultAllocator<ezUpdateReflectionTypeMsgToEditor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Descriptor", m_desc),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSetupProjectMsgToEngine, 1, ezRTTIDefaultAllocator<ezSetupProjectMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ProjectDir", m_sProjectDir),
    EZ_MEMBER_PROPERTY("FileSystemConfig", m_FileSystemConfig),
    EZ_MEMBER_PROPERTY("PluginConfig", m_PluginConfig),
    EZ_MEMBER_PROPERTY("FileserveAddress", m_sFileserveAddress),
    EZ_MEMBER_PROPERTY("Platform", m_sAssetProfile),
    EZ_MEMBER_PROPERTY("DevicePixelRatio", m_fDevicePixelRatio),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShutdownProcessMsgToEngine, 1, ezRTTIDefaultAllocator<ezShutdownProcessMsgToEngine>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProjectReadyMsgToEditor, 1, ezRTTIDefaultAllocator<ezProjectReadyMsgToEditor> )
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSimpleConfigMsgToEngine, 1, ezRTTIDefaultAllocator<ezSimpleConfigMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    EZ_MEMBER_PROPERTY("Payload", m_sPayload),
    EZ_MEMBER_PROPERTY("PayloadValue", m_fPayload),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSaveProfilingResponseToEditor, 1, ezRTTIDefaultAllocator<ezSaveProfilingResponseToEditor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ProfilingFile", m_sProfilingFile),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezReloadResourceMsgToEngine, 1, ezRTTIDefaultAllocator<ezReloadResourceMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Type", m_sResourceType),
    EZ_MEMBER_PROPERTY("ID", m_sResourceID),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezResourceUpdateMsgToEngine, 1, ezRTTIDefaultAllocator<ezResourceUpdateMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Type", m_sResourceType),
    EZ_MEMBER_PROPERTY("ID", m_sResourceID),
    EZ_MEMBER_PROPERTY("Data", m_Data),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRestoreResourceMsgToEngine, 1, ezRTTIDefaultAllocator<ezRestoreResourceMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Type", m_sResourceType),
    EZ_MEMBER_PROPERTY("ID", m_sResourceID),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezChangeCVarMsgToEngine, 1, ezRTTIDefaultAllocator<ezChangeCVarMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sCVarName),
    EZ_MEMBER_PROPERTY("Value", m_NewValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConsoleCmdMsgToEngine, 1, ezRTTIDefaultAllocator<ezConsoleCmdMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Type", m_iType),
    EZ_MEMBER_PROPERTY("Cmd", m_sCommand),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConsoleCmdResultMsgToEditor, 1, ezRTTIDefaultAllocator<ezConsoleCmdResultMsgToEditor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Result", m_sResult),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDynamicStringEnumMsgToEditor, 1, ezRTTIDefaultAllocator<ezDynamicStringEnumMsgToEditor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("EnumName", m_sEnumName),
    EZ_ARRAY_MEMBER_PROPERTY("EnumValues", m_EnumValues),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpReplicationMsg, 1, ezRTTIDefaultAllocator<ezLongOpReplicationMsg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("OpGuid", m_OperationGuid),
    EZ_MEMBER_PROPERTY("DocGuid", m_DocumentGuid),
    EZ_MEMBER_PROPERTY("Type", m_sReplicationType),
    EZ_MEMBER_PROPERTY("Data", m_ReplicationData),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpProgressMsg, 1, ezRTTIDefaultAllocator<ezLongOpProgressMsg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("OpGuid", m_OperationGuid),
    EZ_MEMBER_PROPERTY("Completion", m_fCompletion),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpResultMsg, 1, ezRTTIDefaultAllocator<ezLongOpResultMsg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("OpGuid", m_OperationGuid),
    EZ_MEMBER_PROPERTY("Success", m_bSuccess),
    EZ_MEMBER_PROPERTY("Data", m_ResultData),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

///////////////////////////////////// ezEditorEngineDocumentMsg /////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineDocumentMsg, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DocumentGuid", m_DocumentGuid),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentConfigMsgToEngine, 1, ezRTTIDefaultAllocator<ezDocumentConfigMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    EZ_MEMBER_PROPERTY("Int", m_iValue),
    EZ_MEMBER_PROPERTY("Float", m_fValue),
    EZ_MEMBER_PROPERTY("String", m_sValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineViewMsg, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ViewID", m_uiViewID),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentOpenMsgToEngine, 1, ezRTTIDefaultAllocator<ezDocumentOpenMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DocumentOpen", m_bDocumentOpen),
    EZ_MEMBER_PROPERTY("DocumentType", m_sDocumentType),
    EZ_MEMBER_PROPERTY("DocumentMetaData", m_DocumentMetaData),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentClearMsgToEngine, 1, ezRTTIDefaultAllocator<ezDocumentClearMsgToEngine>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentOpenResponseMsgToEditor, 1, ezRTTIDefaultAllocator<ezDocumentOpenResponseMsgToEditor> )
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewDestroyedMsgToEngine, 1, ezRTTIDefaultAllocator<ezViewDestroyedMsgToEngine>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewDestroyedResponseMsgToEditor, 1, ezRTTIDefaultAllocator<ezViewDestroyedResponseMsgToEditor>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewRedrawMsgToEngine, 1, ezRTTIDefaultAllocator<ezViewRedrawMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("HWND", m_uiHWND),
    EZ_MEMBER_PROPERTY("WindowWidth", m_uiWindowWidth),
    EZ_MEMBER_PROPERTY("WindowHeight", m_uiWindowHeight),
    EZ_MEMBER_PROPERTY("UpdatePickingData", m_bUpdatePickingData),
    EZ_MEMBER_PROPERTY("EnablePickSelected", m_bEnablePickingSelected),
    EZ_MEMBER_PROPERTY("EnablePickTransparent", m_bEnablePickTransparent),
    EZ_MEMBER_PROPERTY("UseCamOnDevice", m_bUseCameraTransformOnDevice),
    EZ_MEMBER_PROPERTY("CameraMode", m_iCameraMode),
    EZ_MEMBER_PROPERTY("NearPlane", m_fNearPlane),
    EZ_MEMBER_PROPERTY("FarPlane", m_fFarPlane),
    EZ_MEMBER_PROPERTY("FovOrDim", m_fFovOrDim),
    EZ_MEMBER_PROPERTY("Position", m_vPosition),
    EZ_MEMBER_PROPERTY("Forwards", m_vDirForwards),
    EZ_MEMBER_PROPERTY("Up", m_vDirUp),
    EZ_MEMBER_PROPERTY("Right", m_vDirRight),
    EZ_MEMBER_PROPERTY("ViewMat", m_ViewMatrix),
    EZ_MEMBER_PROPERTY("ProjMat", m_ProjMatrix),
    EZ_MEMBER_PROPERTY("RenderMode", m_uiRenderMode),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewScreenshotMsgToEngine, 1, ezRTTIDefaultAllocator<ezViewScreenshotMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("File", m_sOutputFile)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActivateRemoteViewMsgToEngine, 1, ezRTTIDefaultAllocator<ezActivateRemoteViewMsgToEngine>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEntityMsgToEngine, 1, ezRTTIDefaultAllocator<ezEntityMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Change", m_change),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSimpleDocumentConfigMsgToEngine, 1, ezRTTIDefaultAllocator<ezSimpleDocumentConfigMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    EZ_MEMBER_PROPERTY("Payload1", m_sPayload),
    EZ_MEMBER_PROPERTY("Payload2", m_PayloadValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSimpleDocumentConfigMsgToEditor, 1, ezRTTIDefaultAllocator<ezSimpleDocumentConfigMsgToEditor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    EZ_MEMBER_PROPERTY("Payload1", m_sPayload),
    EZ_MEMBER_PROPERTY("Payload2", m_PayloadValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezExportDocumentMsgToEngine, 1, ezRTTIDefaultAllocator<ezExportDocumentMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("OutputFile", m_sOutputFile),
    EZ_MEMBER_PROPERTY("AssetHash", m_uiAssetHash),
    EZ_MEMBER_PROPERTY("AssetVersion", m_uiVersion),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezExportDocumentMsgToEditor, 1, ezRTTIDefaultAllocator<ezExportDocumentMsgToEditor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("OutputSuccess", m_bOutputSuccess),
    EZ_MEMBER_PROPERTY("FailureMsg", m_sFailureMsg),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCreateThumbnailMsgToEngine, 1, ezRTTIDefaultAllocator<ezCreateThumbnailMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Width", m_uiWidth),
    EZ_MEMBER_PROPERTY("Height", m_uiHeight),
    EZ_ARRAY_MEMBER_PROPERTY("ViewExcludeTags", m_ViewExcludeTags),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCreateThumbnailMsgToEditor, 1, ezRTTIDefaultAllocator<ezCreateThumbnailMsgToEditor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ThumbnailData", m_ThumbnailData),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewPickingMsgToEngine, 1, ezRTTIDefaultAllocator<ezViewPickingMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("PickPosX", m_uiPickPosX),
    EZ_MEMBER_PROPERTY("PickPosY", m_uiPickPosY),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewPickingResultMsgToEditor, 1, ezRTTIDefaultAllocator<ezViewPickingResultMsgToEditor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    EZ_MEMBER_PROPERTY("ComponentGuid", m_ComponentGuid),
    EZ_MEMBER_PROPERTY("OtherGuid", m_OtherGuid),
    EZ_MEMBER_PROPERTY("PartIndex", m_uiPartIndex),
    EZ_MEMBER_PROPERTY("PickedPos", m_vPickedPosition),
    EZ_MEMBER_PROPERTY("PickedNormal", m_vPickedNormal),
    EZ_MEMBER_PROPERTY("PickRayStart", m_vPickingRayStartPosition),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewMarqueePickingMsgToEngine, 1, ezRTTIDefaultAllocator<ezViewMarqueePickingMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("PickPosX0", m_uiPickPosX0),
    EZ_MEMBER_PROPERTY("PickPosY0", m_uiPickPosY0),
    EZ_MEMBER_PROPERTY("PickPosX1", m_uiPickPosX1),
    EZ_MEMBER_PROPERTY("PickPosY1", m_uiPickPosY1),
    EZ_MEMBER_PROPERTY("what", m_uiWhatToDo),
    EZ_MEMBER_PROPERTY("aid", m_uiActionIdentifier),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewMarqueePickingResultMsgToEditor, 1, ezRTTIDefaultAllocator<ezViewMarqueePickingResultMsgToEditor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Objects", m_ObjectGuids),
    EZ_MEMBER_PROPERTY("what", m_uiWhatToDo),
    EZ_MEMBER_PROPERTY("aid", m_uiActionIdentifier),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewHighlightMsgToEngine, 1, ezRTTIDefaultAllocator<ezViewHighlightMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("HighlightObject", m_HighlightObject),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLogMsgToEditor, 1, ezRTTIDefaultAllocator<ezLogMsgToEditor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Entry", m_Entry),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCVarMsgToEditor, 1, ezRTTIDefaultAllocator<ezCVarMsgToEditor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("Plugin", m_sPlugin),
    EZ_MEMBER_PROPERTY("Desc", m_sDescription),
    EZ_MEMBER_PROPERTY("Value", m_Value),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineSyncObjectMsg, 1, ezRTTIDefaultAllocator<ezEditorEngineSyncObjectMsg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    EZ_MEMBER_PROPERTY("ObjectType", m_sObjectType),
    EZ_ACCESSOR_PROPERTY("ObjectData", GetObjectData, SetObjectData),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezObjectTagMsgToEngine, 1, ezRTTIDefaultAllocator<ezObjectTagMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    EZ_MEMBER_PROPERTY("Tag", m_sTag),
    EZ_MEMBER_PROPERTY("Set", m_bSetTag),
    EZ_MEMBER_PROPERTY("Recursive", m_bApplyOnAllChildren),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezObjectSelectionMsgToEngine, 1, ezRTTIDefaultAllocator<ezObjectSelectionMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Selection", m_sSelection),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSimulationSettingsMsgToEngine, 1, ezRTTIDefaultAllocator<ezSimulationSettingsMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SimulateWorld", m_bSimulateWorld),
    EZ_MEMBER_PROPERTY("SimulationSpeed", m_fSimulationSpeed),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGridSettingsMsgToEngine, 1, ezRTTIDefaultAllocator<ezGridSettingsMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("GridDensity", m_fGridDensity),
    EZ_MEMBER_PROPERTY("GridCenter", m_vGridCenter),
    EZ_MEMBER_PROPERTY("GridTangent1", m_vGridTangent1),
    EZ_MEMBER_PROPERTY("GridTangent2", m_vGridTangent2),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGlobalSettingsMsgToEngine, 1, ezRTTIDefaultAllocator<ezGlobalSettingsMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("GizmoScale", m_fGizmoScale),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezWorldSettingsMsgToEngine, 1, ezRTTIDefaultAllocator<ezWorldSettingsMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RenderOverlay", m_bRenderOverlay),
    EZ_MEMBER_PROPERTY("ShapeIcons", m_bRenderShapeIcons),
    EZ_MEMBER_PROPERTY("RenderSelectionBoxes", m_bRenderSelectionBoxes),
    EZ_MEMBER_PROPERTY("AddAmbient", m_bAddAmbientLight),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameModeMsgToEngine, 1, ezRTTIDefaultAllocator<ezGameModeMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Run", m_bEnablePTG),
    EZ_MEMBER_PROPERTY("UsePos", m_bUseStartPosition),
    EZ_MEMBER_PROPERTY("Pos", m_vStartPosition),
    EZ_MEMBER_PROPERTY("Dir", m_vStartDirection),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameModeMsgToEditor, 1, ezRTTIDefaultAllocator<ezGameModeMsgToEditor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Run", m_bRunningPTG),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezQuerySelectionBBoxMsgToEngine, 1, ezRTTIDefaultAllocator<ezQuerySelectionBBoxMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ViewID", m_uiViewID),
    EZ_MEMBER_PROPERTY("Purpose", m_iPurpose),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezQuerySelectionBBoxResultMsgToEditor, 1, ezRTTIDefaultAllocator<ezQuerySelectionBBoxResultMsgToEditor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Center", m_vCenter),
    EZ_MEMBER_PROPERTY("Extents", m_vHalfExtents),
    EZ_MEMBER_PROPERTY("ViewID", m_uiViewID),
    EZ_MEMBER_PROPERTY("Purpose", m_iPurpose),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGatherObjectsOfTypeMsgInterDoc, 1, ezRTTIDefaultAllocator<ezGatherObjectsOfTypeMsgInterDoc>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGatherObjectsForDebugVisMsgInterDoc, 1, ezRTTIDefaultAllocator<ezGatherObjectsForDebugVisMsgInterDoc>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezObjectsForDebugVisMsgToEngine, 1, ezRTTIDefaultAllocator<ezObjectsForDebugVisMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Objects", m_Objects),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

