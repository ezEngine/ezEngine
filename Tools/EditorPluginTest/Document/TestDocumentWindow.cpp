#include <PCH.h>
#include <EditorPluginTest/Document/TestDocumentWindow.moc.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <CoreUtils/Geometry/GeomUtils.h>
#include <CoreUtils/Geometry/OBJLoader.h>
#include <Foundation/IO/OSFile.h>
#include <QTimer>
#include <QPushButton>
#include <EditorFramework/EngineView/EngineViewMessages.h>
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
  m_pRestartButton->setVisible(ezEditorEngineViewProcess::GetInstance()->IsProcessCrashed());
  m_pRestartButton->setMaximumWidth(200);
  m_pRestartButton->setMinimumHeight(50);
  m_pRestartButton->connect(m_pRestartButton, &QPushButton::clicked, this, &ezTestDocumentWindow::SlotRestartEngineProcess);

  m_pRestartButtonLayout->addWidget(m_pRestartButton);

  SetTargetFramerate(15);

  m_pEngineView = ezEditorEngineViewProcess::GetInstance()->CreateEngineView();
  ezEditorEngineViewProcess::s_Events.AddEventHandler(ezDelegate<void (const ezEditorEngineViewProcess::Event&)>(&ezTestDocumentWindow::EngineViewProcessEventHandler, this));
}

ezTestDocumentWindow::~ezTestDocumentWindow()
{
  ezEditorEngineViewProcess::s_Events.RemoveEventHandler(ezDelegate<void (const ezEditorEngineViewProcess::Event&)>(&ezTestDocumentWindow::EngineViewProcessEventHandler, this));
  ezEditorEngineViewProcess::GetInstance()->DestroyEngineView(m_pEngineView);
}

void ezTestDocumentWindow::EngineViewProcessEventHandler(const ezEditorEngineViewProcess::Event& e)
{
  switch (e.m_Type)
  {
  case ezEditorEngineViewProcess::Event::Type::ProcessCrashed:
    {
      m_pRestartButton->setVisible(true);
      m_pRestartButton->update();
    }
    break;

  case ezEditorEngineViewProcess::Event::Type::ProcessStarted:
    {
      m_pRestartButton->setVisible(false);
    }
    break;

  case ezEditorEngineViewProcess::Event::Type::ProcessShutdown:
    break;
  }
}

void ezTestDocumentWindow::SlotRestartEngineProcess()
{
  ezEditorEngineViewProcess::GetInstance()->RestartProcess();
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
