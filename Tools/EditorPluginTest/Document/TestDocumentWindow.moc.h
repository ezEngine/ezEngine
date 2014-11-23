#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/IPC/ProcessCommunication.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererCore/Pipeline/RenderHelper.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <System/Window/Window.h>
#include <QProcess>
#include <QSharedMemory>

class QWidget;
class QHBoxLayout;
class QPushButton;

class ez3DViewWidget : public QWidget
{
public:
  ez3DViewWidget(QWidget* pParent, ezDocumentWindow* pDocument) : QWidget(pParent), m_pDocument(pDocument) { }

protected:
  virtual void resizeEvent(QResizeEvent* event) override
  {
    m_pDocument->TriggerRedraw();
  }

  ezDocumentWindow* m_pDocument;
};

class ezTestDocumentWindow : public ezDocumentWindow
{
  Q_OBJECT

public:
  ezTestDocumentWindow(ezDocumentBase* pDocument);
  ~ezTestDocumentWindow();

protected:
  void keyPressEvent(QKeyEvent* e) override;

private slots:
  void SlotRestartEngineProcess();

private:
  virtual void InternalRedraw() override;
  void EngineViewProcessEventHandler(const ezEditorEngineProcessConnection::Event& e);
  void DocumentTreeEventHandler(const ezDocumentObjectTreeStructureEvent& e);
  void PropertyEventHandler(const ezDocumentObjectTreePropertyEvent& e);

  void SendRedrawMsg();

  ez3DViewWidget* m_pCenterWidget;
  ezEditorEngineConnection* m_pEngineView;
  QHBoxLayout* m_pRestartButtonLayout;
  QPushButton* m_pRestartButton;
};