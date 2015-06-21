#include <GuiFoundation/PCH.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <QTimer>

ezDynamicArray<ezApplicationPanel*> ezApplicationPanel::s_AllApplicationPanels;

ezApplicationPanel::ezApplicationPanel(const char* szPanelName) : QDockWidget(ezContainerWindow::GetAllContainerWindows()[0])
{
  ezStringBuilder sPanel("AppPanel_", szPanelName);

  setObjectName(QString::fromUtf8(sPanel.GetData()));
  setWindowTitle(QString::fromUtf8(szPanelName));

  setBackgroundRole(QPalette::ColorRole::Highlight);

  s_AllApplicationPanels.PushBack(this);

  m_pContainerWindow = nullptr;

  ezContainerWindow::GetAllContainerWindows()[0]->MoveApplicationPanelToContainer(this);

  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezApplicationPanel::ToolsProjectEventHandler, this));
}

ezApplicationPanel::~ezApplicationPanel()
{
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezApplicationPanel::ToolsProjectEventHandler, this));

  s_AllApplicationPanels.RemoveSwap(this);
}

void ezApplicationPanel::EnsureVisible()
{
  m_pContainerWindow->EnsureVisible(this);

  EZ_ASSERT_DEV(parent() != nullptr, "Invalid Parent!");
}


void ezApplicationPanel::ToolsProjectEventHandler(const ezToolsProject::Event& e)
{
  switch (e.m_Type)
  {
  case ezToolsProject::Event::Type::ProjectClosing:
    setEnabled(false);
    break;
  case ezToolsProject::Event::Type::ProjectOpened:
    setEnabled(true);
    break;
  }
}