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
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Core/World/GameObject.h>
#include <QKeyEvent>

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

  SetTargetFramerate(24);

  m_pEngineView = ezEditorEngineProcessConnection::GetInstance()->CreateEngineConnection(pDocument);
  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezDelegate<void (const ezEditorEngineProcessConnection::Event&)>(&ezTestDocumentWindow::EngineViewProcessEventHandler, this));

  GetDocument()->GetObjectTree()->m_StructureEvents.AddEventHandler(ezDelegate<void (const ezDocumentObjectTreeStructureEvent&)>(&ezTestDocumentWindow::DocumentTreeEventHandler, this));
}

ezTestDocumentWindow::~ezTestDocumentWindow()
{
  GetDocument()->GetObjectTree()->m_StructureEvents.RemoveEventHandler(ezDelegate<void (const ezDocumentObjectTreeStructureEvent&)>(&ezTestDocumentWindow::DocumentTreeEventHandler, this));
  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezDelegate<void (const ezEditorEngineProcessConnection::Event&)>(&ezTestDocumentWindow::EngineViewProcessEventHandler, this));
  ezEditorEngineProcessConnection::GetInstance()->DestroyEngineConnection(m_pEngineView);
}

void ezTestDocumentWindow::DocumentTreeEventHandler(const ezDocumentObjectTreeStructureEvent& e)
{
  ezEngineProcessEntityMsg msg;
  msg.m_DocumentGuid = GetDocument()->GetGuid();
  msg.m_ObjectGuid = e.m_pObject->GetGuid();
  msg.m_uiNewChildIndex = e.m_uiNewChildIndex;

  if (e.m_pPreviousParent)
    msg.m_PreviousParentGuid = e.m_pPreviousParent->GetGuid();
  if (e.m_pNewParent)
    msg.m_NewParentGuid = e.m_pNewParent->GetGuid();

  // TODO lalala
  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  ezMemoryStreamReader reader(&storage);
  ezToolsReflectionUtils::WriteObjectToJSON(writer, e.m_pObject->GetTypeAccessor());

  ezStringBuilder sData;
  sData.ReadAll(reader);

  msg.SetObjectData(sData);

  switch (e.m_EventType)
  {
  case ezDocumentObjectTreeStructureEvent::Type::AfterObjectAdded:
    {
      msg.m_iMsgType = ezEngineProcessEntityMsg::ObjectAdded;
    }
    break;

  case ezDocumentObjectTreeStructureEvent::Type::AfterObjectMoved:
    {
      msg.m_iMsgType = ezEngineProcessEntityMsg::ObjectMoved;
    }
    break;

  case ezDocumentObjectTreeStructureEvent::Type::BeforeObjectRemoved:
    {
      msg.m_iMsgType = ezEngineProcessEntityMsg::ObjectRemoved;
    }
    break;

  case ezDocumentObjectTreeStructureEvent::Type::AfterObjectRemoved:
  case ezDocumentObjectTreeStructureEvent::Type::BeforeObjectAdded:
  case ezDocumentObjectTreeStructureEvent::Type::BeforeObjectMoved:
    return;

  default:
    EZ_REPORT_FAILURE("Unknown event type");
    return;
  }

  m_pEngineView->SendMessage(&msg);
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

void ezTestDocumentWindow::keyPressEvent(QKeyEvent* e)
{
  if (e->modifiers() == Qt::KeyboardModifier::ControlModifier)
  {
    if (e->key() == Qt::Key_Z)
    {
      if (GetDocument()->GetCommandHistory()->CanUndo())
      {
        GetDocument()->GetCommandHistory()->Undo();
        return;
      }
    }
    else if (e->key() == Qt::Key_Y)
    {
      if (GetDocument()->GetCommandHistory()->CanRedo())
      {
        GetDocument()->GetCommandHistory()->Redo();
        return;
      }
    }
  }

  ezDocumentWindow::keyPressEvent(e);
}

