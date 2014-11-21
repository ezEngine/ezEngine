#include <PCH.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcessMessage, ezReflectedClass, 1, ezRTTINoAllocator );
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEngineProcessMsg, ezProcessMessage, 1, ezRTTINoAllocator );
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("DocumentGuid", m_DocumentGuid),
    EZ_MEMBER_PROPERTY("ViewID", m_uiViewID),
    EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEngineViewRedrawMsg, ezEngineProcessMsg, 1, ezRTTIDefaultAllocator<ezEngineViewRedrawMsg> );
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("HWND", m_uiHWND),
    EZ_MEMBER_PROPERTY("WindowWidth", m_uiWindowWidth),
    EZ_MEMBER_PROPERTY("WindowHeight", m_uiWindowHeight),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

