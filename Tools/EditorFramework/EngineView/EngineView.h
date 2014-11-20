#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/EngineView/EngineViewMessages.h>
#include <Foundation/Communication/Event.h>

class ezEditorEngineView;

class EZ_EDITORFRAMEWORK_DLL ezEditorEngineViewProcess
{
  static ezEditorEngineViewProcess* s_pInstance;

public:
  ezEditorEngineViewProcess();
  ~ezEditorEngineViewProcess();

  static ezEditorEngineViewProcess* GetInstance() { return s_pInstance; }

  void Update();
  void RestartProcess();
  bool IsProcessCrashed() const { return m_bProcessCrashed; }

  ezEditorEngineView* CreateEngineView();
  void DestroyEngineView(ezEditorEngineView* pView);

  void SendMessage(ezProcessMessage* pMessage);

  struct Event
  {
    enum class Type
    {
      ProcessStarted,
      ProcessCrashed,
      ProcessShutdown,
    };

    Type m_Type;
  };

  static ezEvent<const Event&> s_Events;

private:
  void Initialize();
  void Deinitialize();

  bool m_bProcessShouldBeRunning;
  bool m_bProcessCrashed;
  ezInt32 m_iNextEngineViewID;
  ezInt32 m_iNumViews;
  ezProcessCommunication m_IPC;
  ezHashTable<ezInt32, ezEditorEngineView*> m_EngineViewsByID;
};

class EZ_EDITORFRAMEWORK_DLL ezEditorEngineView
{
public:

  void SendMessage(ezEngineViewMsg* pMessage);

private:
  friend class ezEditorEngineViewProcess;
  ezEditorEngineView() { m_iEngineViewID = -2; }
  ~ezEditorEngineView() { }

  ezInt32 m_iEngineViewID;
};
