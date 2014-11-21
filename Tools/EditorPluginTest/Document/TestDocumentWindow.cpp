#include <PCH.h>
#include <EditorPluginTest/Document/TestDocumentWindow.moc.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <CoreUtils/Geometry/GeomUtils.h>
#include <CoreUtils/Geometry/OBJLoader.h>
#include <Foundation/IO/OSFile.h>
#include <QTimer>
#include <QPushButton>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <qlayout.h>

ezTestDocumentWindow::ezTestDocumentWindow(ezDocumentBase* pDocument) : ezDocumentWindow(pDocument)
{
  m_pCenterWidget = new ez3DViewWidget(this, this);
  m_pCenterWidget->setAutoFillBackground(false);
  setCentralWidget(m_pCenterWidget);

  m_pRestartButtonLayout = new QHBoxLayout(this);
  m_pRestartButtonLayout->setMargin(0);

  centralWidget()->setLayout(m_pRestartButtonLayout);

  m_pRestartButton = new QPushButton(centralWidget());
  m_pRestartButton->setText("Restart Engine View Process");
  m_pRestartButton->setVisible(ezEditorEngineProcessConnection::GetInstance()->IsProcessCrashed());
  m_pRestartButton->setMaximumWidth(200);
  m_pRestartButton->setMinimumHeight(50);
  m_pRestartButton->connect(m_pRestartButton, &QPushButton::clicked, this, &ezTestDocumentWindow::SlotRestartEngineProcess);

  m_pRestartButtonLayout->addWidget(m_pRestartButton);

  SetTargetFramerate(15);

  m_pEngineView = ezEditorEngineProcessConnection::GetInstance()->CreateEngineConnection(pDocument);
  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezDelegate<void (const ezEditorEngineProcessConnection::Event&)>(&ezTestDocumentWindow::EngineViewProcessEventHandler, this));
}

ezTestDocumentWindow::~ezTestDocumentWindow()
{
  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezDelegate<void (const ezEditorEngineProcessConnection::Event&)>(&ezTestDocumentWindow::EngineViewProcessEventHandler, this));
  ezEditorEngineProcessConnection::GetInstance()->DestroyEngineConnection(m_pEngineView);
}

void ezTestDocumentWindow::EngineViewProcessEventHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
  case ezEditorEngineProcessConnection::Event::Type::ProcessCrashed:
    {
      m_pRestartButton->setVisible(true);
      m_pRestartButton->update();
    }
    break;

  case ezEditorEngineProcessConnection::Event::Type::ProcessStarted:
    {
      m_pRestartButton->setVisible(false);
    }
    break;

  case ezEditorEngineProcessConnection::Event::Type::ProcessShutdown:
    break;
  }
}

void ezTestDocumentWindow::SlotRestartEngineProcess()
{
  ezEditorEngineProcessConnection::GetInstance()->RestartProcess();
}

void ezTestDocumentWindow::InternalRedraw()
{

  SendRedrawMsg();
}

void ezTestDocumentWindow::SendRedrawMsg()
{
  ezEngineViewRedrawMsg msg;
  msg.m_uiHWND = (ezUInt32) m_pCenterWidget->winId();
  msg.m_uiWindowWidth = m_pCenterWidget->width();
  msg.m_uiWindowHeight = m_pCenterWidget->height();

  m_pEngineView->SendMessage(&msg);
}
