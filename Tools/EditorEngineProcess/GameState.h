#pragma once

#include <EditorFramework/IPC/ProcessCommunication.h>

#include <GameFoundation/GameApplication.h>
#include <Core/World/GameObject.h>
#include <Core/World/Component.h>

class ezEngineProcessDocumentContext;
class QApplication;
class ezEngineProcessViewContext;
class ezEntityMsgToEngine;



class ezEngineProcessGameState : public ezGameStateBase
{
public:
  ezEngineProcessGameState();
  void EventHandlerIPC(const ezProcessCommunication::Event& e);

  static ezEngineProcessGameState* GetInstance() { return s_pInstance; }

  ezProcessCommunication& ProcessCommunication() { return m_IPC; }

  void ProcessIPCMessages();

private:
  virtual void Activate() override;
  virtual void Deactivate() override;
  
  void LogWriter(const ezLoggingEventData& e);

  void SendReflectionInformation();
  void SendProjectReadyMessage();


  static ezEngineProcessGameState* s_pInstance;

  QApplication* m_pApp;
  ezProcessCommunication m_IPC;
};

