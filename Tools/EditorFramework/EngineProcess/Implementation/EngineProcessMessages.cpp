#include <PCH.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcessMessage, ezReflectedClass, 1, ezRTTINoAllocator );
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineMsg, ezProcessMessage, 1, ezRTTINoAllocator );
EZ_END_DYNAMIC_REFLECTED_TYPE();

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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEntityMsgToEngine, ezEditorEngineDocumentMsg, 1, ezRTTIDefaultAllocator<ezEntityMsgToEngine> );
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("MsgType", m_iMsgType),
    EZ_MEMBER_PROPERTY("NewChildIndex", m_uiNewChildIndex),
    EZ_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    EZ_MEMBER_PROPERTY("PreviousParentGuid", m_PreviousParentGuid),
    EZ_MEMBER_PROPERTY("NewParentGuid", m_NewParentGuid),
    EZ_MEMBER_PROPERTY("ObjectType", m_sObjectType),
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLogMsgToEditor, ezEditorEngineDocumentMsg, 1, ezRTTIDefaultAllocator<ezLogMsgToEditor> );
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Text", m_sText),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezUpdateReflectionTypeMsgToEditor, ezEditorEngineMsg, 1, ezRTTIDefaultAllocator<ezUpdateReflectionTypeMsgToEditor> );
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("NumProperties", m_uiNumProperties),
    EZ_MEMBER_PROPERTY("TypeName", m_sTypeName),
    EZ_MEMBER_PROPERTY("PluginName", m_sPluginName),
    EZ_MEMBER_PROPERTY("ParentTypeName", m_sParentTypeName),
    EZ_MEMBER_PROPERTY("DefaultInit", m_sDefaultInitialization),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezUpdateReflectionPropertyMsgToEditor, ezEditorEngineMsg, 1, ezRTTIDefaultAllocator<ezUpdateReflectionPropertyMsgToEditor> );
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Index", m_uiPropertyIndex),
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("TypeName", m_sType),
    EZ_MEMBER_PROPERTY("TypeEnum", m_Type),
    EZ_MEMBER_PROPERTY("Flags", m_Flags),
    EZ_MEMBER_PROPERTY("ConstantValue", m_ConstantValue),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

