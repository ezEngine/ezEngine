#include <PCH.h>
#include <EditorFramework/DocumentWindow/DocumentWindow3D.moc.h>
#include <QPushButton>
#include <qlayout.h>

//ezEvent<const ezDocumentWindow::Event&> ezDocumentWindow::s_Events;

ezDocumentWindow3D::ezDocumentWindow3D(ezDocumentBase* pDocument) : ezDocumentWindow(pDocument)
{
  m_pRestartButtonLayout = nullptr;
  m_pRestartButton = nullptr;
  m_pEngineView = nullptr;

  m_pEngineView = ezEditorEngineProcessConnection::GetInstance()->CreateEngineConnection(this);
  
  
  
  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezDelegate<void (const ezEditorEngineProcessConnection::Event&)>(&ezDocumentWindow3D::EngineViewProcessEventHandler, this));

  m_pEngineView->SendDocument();
}

ezDocumentWindow3D::~ezDocumentWindow3D()
{
  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezDelegate<void (const ezEditorEngineProcessConnection::Event&)>(&ezDocumentWindow3D::EngineViewProcessEventHandler, this));

  ezEditorEngineProcessConnection::GetInstance()->DestroyEngineConnection(this);
}

void ezDocumentWindow3D::SlotRestartEngineProcess()
{
  ezEditorEngineProcessConnection::GetInstance()->RestartProcess();
}

void ezDocumentWindow3D::ShowRestartButton(bool bShow)
{
  if (m_pRestartButtonLayout == nullptr && bShow == true)
  {
    m_pRestartButtonLayout = new QHBoxLayout(this);
    m_pRestartButtonLayout->setMargin(0);

    centralWidget()->setLayout(m_pRestartButtonLayout);

    m_pRestartButton = new QPushButton(centralWidget());
    m_pRestartButton->setText("Restart Engine View Process");
    m_pRestartButton->setVisible(ezEditorEngineProcessConnection::GetInstance()->IsProcessCrashed());
    m_pRestartButton->setMaximumWidth(200);
    m_pRestartButton->setMinimumHeight(50);
    m_pRestartButton->connect(m_pRestartButton, &QPushButton::clicked, this, &ezDocumentWindow3D::SlotRestartEngineProcess);

    m_pRestartButtonLayout->addWidget(m_pRestartButton);
  }

  if (m_pRestartButton)
  {
    m_pRestartButton->setVisible(bShow);

    if (bShow)
      m_pRestartButton->update();
  }
}

bool ezDocumentWindow3D::HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezDocumentOpenResponseMsgToEditor>())
  {
    m_pEngineView->SendDocument();
    return true;
  }

  return false;
}

void ezDocumentWindow3D::EngineViewProcessEventHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
  case ezEditorEngineProcessConnection::Event::Type::ProcessCrashed:
    {
      ShowRestartButton(true);
    }
    break;

  case ezEditorEngineProcessConnection::Event::Type::ProcessStarted:
    {
      ShowRestartButton(false);
    }
    break;

  case ezEditorEngineProcessConnection::Event::Type::ProcessShutdown:
    break;
  }
}

