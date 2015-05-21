#pragma once

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <QApplication>

#include <EditorFramework/IPC/ProcessCommunication.h>
#include <EditorEngineProcess/ViewContext.h>

#include <GameFoundation/GameApplication.h>

class ezEngineProcessDocumentContext;

class ezEditorGameState : public ezGameStateBase
{
public:
  ezEditorGameState();
  void EventHandlerIPC(const ezProcessCommunication::Event& e);

private:
  virtual void Activate() override;
  virtual void Deactivate() override;
  virtual void BeforeWorldUpdate() override;

  void HandlerEntityMsg(ezEngineProcessDocumentContext* pDocumentContext, ezViewContext* pViewContext, ezEntityMsgToEngine* pMsg);
  void UpdateProperties(ezEntityMsgToEngine* pMsg, void* pObject, const ezRTTI* pRtti);
  void HandlerGameObjectMsg(ezEngineProcessDocumentContext* pDocumentContext, ezViewContext* pViewContext, ezEntityMsgToEngine* pMsg, ezRTTI* pRtti);
  void HandleComponentMsg(ezEngineProcessDocumentContext* pDocumentContext, ezViewContext* pViewContext, ezEntityMsgToEngine* pMsg, ezRTTI* pRtti);

  void InitDevice();
  void SendReflectionInformation();
  void SendProjectReadyMessage();

  QApplication* m_pApp;
  ezProcessCommunication m_IPC;
};

