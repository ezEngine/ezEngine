#pragma once

#include <Core/Application/Application.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <QApplication>
#include <EditorFramework/IPC/ProcessCommunication.h>
#include <EditorEngineProcess/ViewContext.h>
#include <RendererFoundation/Device/Device.h>

class ezEngineProcessDocumentContext;

class ezEditorProcessApp : public ezApplication
{
public:
  ezEditorProcessApp();
  void EventHandlerIPC(const ezProcessCommunication::Event& e);

  virtual void AfterEngineInit() override;
  virtual void BeforeEngineShutdown() override;
  virtual ApplicationExecution Run() override;

private:
  void HandlerEntityMsg(ezEngineProcessDocumentContext* pDocumentContext, ezViewContext* pViewContext, ezEntityMsgToEngine* pMsg);
  void UpdateProperties(ezEntityMsgToEngine* pMsg, void* pObject, const ezRTTI* pRtti);
  void HandlerGameObjectMsg(ezEngineProcessDocumentContext* pDocumentContext, ezViewContext* pViewContext, ezEntityMsgToEngine* pMsg, ezRTTI* pRtti);
  void HandleComponentMsg(ezEngineProcessDocumentContext* pDocumentContext, ezViewContext* pViewContext, ezEntityMsgToEngine* pMsg, ezRTTI* pRtti);

  void InitDevice();
  void SendReflectionInformation();
  void SendProjectReadyMessage();

  QApplication* m_pApp;
  ezProcessCommunication m_IPC;
  ezGALDevice* s_pDevice;
};

