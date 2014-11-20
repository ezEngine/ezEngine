#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/IPC/ProcessCommunication.h>

class EZ_EDITORFRAMEWORK_DLL ezEngineViewMsg : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEngineViewMsg);

public:
  ezEngineViewMsg()
  {
    m_iTargetID = -1;
  }

  ezInt32 m_iTargetID;
};

class EZ_EDITORFRAMEWORK_DLL ezEngineViewRedrawMsg : public ezEngineViewMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEngineViewRedrawMsg);

public:

  ezUInt32 m_uiHWND;
  ezUInt16 m_uiWindowWidth;
  ezUInt16 m_uiWindowHeight;
};







