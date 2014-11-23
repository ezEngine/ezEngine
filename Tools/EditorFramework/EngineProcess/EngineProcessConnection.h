#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <Foundation/Communication/Event.h>

class ezEditorEngineConnection;
class ezDocumentBase;
class ezDocumentObjectBase;
struct ezDocumentObjectTreePropertyEvent;
struct ezDocumentObjectTreeStructureEvent;

class EZ_EDITORFRAMEWORK_DLL ezEditorEngineProcessConnection
{
  static ezEditorEngineProcessConnection* s_pInstance;

public:
  ezEditorEngineProcessConnection();
  ~ezEditorEngineProcessConnection();

  static ezEditorEngineProcessConnection* GetInstance() { return s_pInstance; }

  void Update();
  void RestartProcess();
  bool IsProcessCrashed() const { return m_bProcessCrashed; }

  ezEditorEngineConnection* CreateEngineConnection(ezDocumentBase* pDocument);
  void DestroyEngineConnection(ezEditorEngineConnection* pView);

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
  ezUInt32 m_uiNextEngineViewID;
  ezInt32 m_iNumViews;
  ezProcessCommunication m_IPC;
  ezHashTable<ezUInt32, ezEditorEngineConnection*> m_EngineViewsByID;
};

class EZ_EDITORFRAMEWORK_DLL ezEditorEngineConnection
{
public:

  void SendMessage(ezEngineProcessMsg* pMessage);

  void SendObjectProperties(const ezDocumentObjectTreePropertyEvent& e);
  void SendDocumentTreeChange(const ezDocumentObjectTreeStructureEvent& e);
  void SendDocument();

private:
  friend class ezEditorEngineProcessConnection;
  ezEditorEngineConnection(ezDocumentBase* pDocument, ezInt32 iEngineViewID) { m_pDocument = pDocument; m_iEngineViewID = iEngineViewID; }
  ~ezEditorEngineConnection() { }

  void SendObject(const ezDocumentObjectBase* pObject);

  ezDocumentBase* m_pDocument;
  ezInt32 m_iEngineViewID;
};
