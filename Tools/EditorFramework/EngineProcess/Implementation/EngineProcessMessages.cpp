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
    EZ_ACCESSOR_PROPERTY("ObjectData", GetObjectData, SetObjectData),
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
    EZ_ACCESSOR_PROPERTY("Text", GetText, SetText),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();



