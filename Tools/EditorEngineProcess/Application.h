#pragma once

#include <Core/Application/Application.h>
#include <EditorFramework/EditorApp.moc.h>
#include <QApplication>
#include <EditorFramework/IPC/ProcessCommunication.h>
#include <EditorEngineProcess/ViewContext.h>
#include <RendererFoundation/Device/Device.h>

class ezEditorProcessApp : public ezApplication
{
public:
  ezEditorProcessApp();
  void EventHandlerIPC(const ezProcessCommunication::Event& e);

  virtual void AfterEngineInit() override;
  virtual void BeforeEngineShutdown() override;
  virtual ApplicationExecution Run() override;

private:

  void InitDevice();

  QApplication* m_pApp;
  ezProcessCommunication m_IPC;
  ezGALDevice* s_pDevice;
};

