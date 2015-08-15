#include <PCH.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcessMessage, ezReflectedClass, 1, ezRTTINoAllocator );
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("Timestamp", m_iSentTimeStamp)
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

///////////////////////////////////// ezEditorEngineMsg ///////////////////////////////////// 

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineMsg, ezProcessMessage, 1, ezRTTINoAllocator );
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezUpdateReflectionTypeMsgToEditor, ezEditorEngineMsg, 1, ezRTTIDefaultAllocator<ezUpdateReflectionTypeMsgToEditor> );
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Descriptor", m_desc),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSetupProjectMsgToEngine, ezEditorEngineMsg, 1, ezRTTIDefaultAllocator<ezSetupProjectMsgToEngine> );
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("ProjectDir", m_sProjectDir),
    EZ_MEMBER_PROPERTY("Config", m_Config),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProjectReadyMsgToEditor, ezEditorEngineMsg, 1, ezRTTIDefaultAllocator<ezProjectReadyMsgToEditor> );
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSimpleConfigMsgToEngine, ezEditorEngineMsg, 1, ezRTTIDefaultAllocator<ezSimpleConfigMsgToEngine>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

///////////////////////////////////// ezEditorEngineDocumentMsg ///////////////////////////////////// 

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineDocumentMsg, ezProcessMessage, 1, ezRTTINoAllocator );
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("DocumentGuid", m_DocumentGuid),
    EZ_MEMBER_PROPERTY("ViewID", m_uiViewID),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentOpenMsgToEngine, ezEditorEngineDocumentMsg, 1, ezRTTIDefaultAllocator<ezDocumentOpenMsgToEngine> );
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("DocumentOpen", m_bDocumentOpen),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentOpenResponseMsgToEditor, ezEditorEngineDocumentMsg, 1, ezRTTIDefaultAllocator<ezDocumentOpenResponseMsgToEditor> );
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewRedrawMsgToEngine, ezEditorEngineDocumentMsg, 1, ezRTTIDefaultAllocator<ezViewRedrawMsgToEngine> );
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("HWND", m_uiHWND),
    EZ_MEMBER_PROPERTY("WindowWidth", m_uiWindowWidth),
    EZ_MEMBER_PROPERTY("WindowHeight", m_uiWindowHeight),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewRedrawFinishedMsgToEditor, ezEditorEngineDocumentMsg, 1, ezRTTIDefaultAllocator<ezViewRedrawFinishedMsgToEditor>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEntityMsgToEngine, ezEditorEngineDocumentMsg, 1, ezRTTIDefaultAllocator<ezEntityMsgToEngine> );
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("MsgType", m_iMsgType),
    EZ_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    EZ_MEMBER_PROPERTY("PreviousParentGuid", m_PreviousParentGuid),
    EZ_MEMBER_PROPERTY("NewParentGuid", m_NewParentGuid),
    EZ_MEMBER_PROPERTY("ObjectType", m_sObjectType),
    EZ_MEMBER_PROPERTY("ParentProperty", m_sParentProperty),
    EZ_MEMBER_PROPERTY("PropertyIndex", m_PropertyIndex),
    EZ_MEMBER_PROPERTY("EditorProperty", m_bEditorProperty),

    EZ_MEMBER_PROPERTY("ObjectData", m_sObjectData),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewCameraMsgToEngine, ezEditorEngineDocumentMsg, 1, ezRTTIDefaultAllocator<ezViewCameraMsgToEngine> );
  EZ_BEGIN_PROPERTIES
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
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewPickingMsgToEngine, ezEditorEngineDocumentMsg, 1, ezRTTIDefaultAllocator<ezViewPickingMsgToEngine>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("PickPosX", m_uiPickPosX),
    EZ_MEMBER_PROPERTY("PickPosY", m_uiPickPosY),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewPickingResultMsgToEditor, ezEditorEngineDocumentMsg, 1, ezRTTIDefaultAllocator<ezViewPickingResultMsgToEditor>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    EZ_MEMBER_PROPERTY("ComponentGuid", m_ComponentGuid),
    EZ_MEMBER_PROPERTY("OtherGuid", m_OtherGuid),
    EZ_MEMBER_PROPERTY("PartIndex", m_uiPartIndex),
    EZ_MEMBER_PROPERTY("PickedPos", m_vPickedPosition),
    EZ_MEMBER_PROPERTY("PickedNormal", m_vPickedNormal),
    EZ_MEMBER_PROPERTY("PickRayStart", m_vPickingRayStartPosition),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewHighlightMsgToEngine, ezEditorEngineDocumentMsg, 1, ezRTTIDefaultAllocator<ezViewHighlightMsgToEngine>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("HighlightObject", m_HighlightObject),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

void ezViewHighlightMsgToEngine::SendHighlightObjectMessage(ezEditorEngineConnection* pConnection)
{
  // without this check there will be so many messages, that the editor comes to a crawl (< 10 FPS)
  // This happens because Qt sends hundreds of mouse-move events and since each 'SendMessageToEngine'
  // requires a round-trip to the engine process, doing this too often will be sloooow

  static ezUuid LastHighlightGuid;

  if (LastHighlightGuid == m_HighlightObject)
    return;

  LastHighlightGuid = m_HighlightObject;
  pConnection->SendMessage(this);
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLogMsgToEditor, ezEditorEngineMsg, 1, ezRTTIDefaultAllocator<ezLogMsgToEditor> );
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Text", m_sText),
    EZ_MEMBER_PROPERTY("Tag", m_sTag),
    EZ_MEMBER_PROPERTY("Type", m_iMsgType),
    EZ_MEMBER_PROPERTY("Indentation", m_uiIndentation),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineSyncObjectMsg, ezEditorEngineDocumentMsg, 1, ezRTTIDefaultAllocator<ezEditorEngineSyncObjectMsg> );
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    EZ_MEMBER_PROPERTY("ObjectType", m_sObjectType),
    EZ_ACCESSOR_PROPERTY("ObjectData", GetObjectData, SetObjectData),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezObjectTagMsgToEngine, ezEditorEngineDocumentMsg, 1, ezRTTIDefaultAllocator<ezObjectTagMsgToEngine>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    EZ_MEMBER_PROPERTY("Tag", m_sTag),
    EZ_MEMBER_PROPERTY("Set", m_bSetTag),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

