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

class EZ_EDITORFRAMEWORK_DLL ezEngineProcessEntityMsg : public ezEngineProcessMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEngineProcessEntityMsg);

public:
  enum Type
  {
    ObjectAdded,
    ObjectRemoved,
    ObjectMoved,
  };

  ezInt8 m_iMsgType;
  ezUInt16 m_uiNewChildIndex;
  ezUuid m_ObjectGuid;
  ezUuid m_PreviousParentGuid;
  ezUuid m_NewParentGuid;
  ezString m_sObjectData;
  ezVec3 m_vPosition;

  const char* GetObjectData() const { return m_sObjectData; }
  void SetObjectData(const char* s) { m_sObjectData = s; }
};







