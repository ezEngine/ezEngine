#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/IPC/ProcessCommunication.h>

class EZ_EDITORFRAMEWORK_DLL ezEngineProcessMsg : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEngineProcessMsg);

public:
  ezEngineProcessMsg()
  {
    m_uiViewID = 0xFFFFFFFF;
  }

  ezUuid m_DocumentGuid;
  ezUInt32 m_uiViewID;
};

class EZ_EDITORFRAMEWORK_DLL ezEngineViewRedrawMsg : public ezEngineProcessMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEngineViewRedrawMsg);

public:

  ezUInt32 m_uiHWND;
  ezUInt16 m_uiWindowWidth;
  ezUInt16 m_uiWindowHeight;
};







