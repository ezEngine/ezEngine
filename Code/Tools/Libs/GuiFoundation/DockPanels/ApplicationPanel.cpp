#include <GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

#include <ads/DockAreaWidget.h>
#include <ads/DockContainerWidget.h>
#include <ads/DockWidgetTab.h>

ezDynamicArray<ezQtApplicationPanel*> ezQtApplicationPanel::s_AllApplicationPanels;

ezQtApplicationPanel::ezQtApplicationPanel(const char* szPanelName)
  : ads::CDockWidget(szPanelName, ezQtContainerWindow::GetContainerWindow())
{
  ezStringBuilder sPanel("AppPanel_", szPanelName);

  setObjectName(QString::fromUtf8(sPanel.GetData()));
  setWindowTitle(QString::fromUtf8(ezTranslate(szPanelName)));

  s_AllApplicationPanels.PushBack(this);

  m_pContainerWindow = nullptr;

  ezQtContainerWindow::GetContainerWindow()->AddApplicationPanel(this);

  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezQtApplicationPanel::ToolsProjectEventHandler, this));
}

ezQtApplicationPanel::~ezQtApplicationPanel()
{
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtApplicationPanel::ToolsProjectEventHandler, this));

  s_AllApplicationPanels.RemoveAndSwap(this);
}

void ezQtApplicationPanel::EnsureVisible()
{
  m_pContainerWindow->EnsureVisible(this).IgnoreResult();

  QWidget* pThis = this;

  if (dockAreaWidget())
  {
    dockAreaWidget()->setCurrentDockWidget(this);
  }

  while (pThis)
  {
    pThis->raise();
    pThis = qobject_cast<QWidget*>(pThis->parent());
  }
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
