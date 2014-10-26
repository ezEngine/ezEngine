#include <PCH.h>
#include <EditorFramework/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/EditorFramework.h>
#include <QSettings>
#include <QMessageBox>

ezEvent<const ezDocumentWindow::Event&> ezDocumentWindow::s_Events;

ezDocumentWindow::ezDocumentWindow(const char* szUniqueName, ezContainerWindow* pContainer)
{
  m_pContainer = pContainer;

  m_sUniqueName = szUniqueName;
  setObjectName(QLatin1String(szUniqueName));

  setAutoFillBackground(true);
  setBackgroundRole(QPalette::ColorRole::Highlight);
}

ezDocumentWindow::~ezDocumentWindow()
{
}

void ezDocumentWindow::closeEvent(QCloseEvent* e)
{
}

bool ezDocumentWindow::CanClose()
{
  return InternalCanClose();
}

bool ezDocumentWindow::InternalCanClose()
{
  // todo check if modified

  QMessageBox::StandardButton res = QMessageBox::question(this, QLatin1String("Close?"), QLatin1String("Save before closing?"), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Cancel);

  if (res == QMessageBox::StandardButton::Cancel)
    return false;

  // todo save ... 

  return true;
}

void ezDocumentWindow::CloseDocument()
{
  Event e;
  e.m_pDocument = this;

  e.m_Type = Event::Type::BeforeDocumentClosed;
  s_Events.Broadcast(e);

  InternalCloseDocument();

  e.m_Type = Event::Type::AfterDocumentClosed;
  s_Events.Broadcast(e);
}

void ezDocumentWindow::InternalCloseDocument()
{
}


