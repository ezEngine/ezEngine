#include <PCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

ezDynamicArray<ezQtApplicationPanel*> ezQtApplicationPanel::s_AllApplicationPanels;

ezQtApplicationPanel::ezQtApplicationPanel(const char* szPanelName)
    : QDockWidget(ezQtContainerWindow::GetAllContainerWindows()[0])
{
  ezStringBuilder sPanel("AppPanel_", szPanelName);

  setObjectName(QString::fromUtf8(sPanel.GetData()));
  setWindowTitle(QString::fromUtf8(ezTranslate(szPanelName)));

  setBackgroundRole(QPalette::ColorRole::Highlight);

  s_AllApplicationPanels.PushBack(this);

  m_pContainerWindow = nullptr;

  ezQtContainerWindow::GetAllContainerWindows()[0]->MoveApplicationPanelToContainer(this);

  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezQtApplicationPanel::ToolsProjectEventHandler, this));
}

ezQtApplicationPanel::~ezQtApplicationPanel()
{
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtApplicationPanel::ToolsProjectEventHandler, this));

  s_AllApplicationPanels.RemoveSwap(this);
}

void ezQtApplicationPanel::EnsureVisible()
{
  m_pContainerWindow->EnsureVisible(this);

  EZ_ASSERT_DEV(parent() != nullptr, "Invalid Parent!");
}


void ezQtApplicationPanel::ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case ezToolsProjectEvent::Type::ProjectClosing:
      setEnabled(false);
      break;
    case ezToolsProjectEvent::Type::ProjectOpened:
      setEnabled(true);
      break;

    default:
      break;
  }
}
