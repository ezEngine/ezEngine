#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

#include <ads/DockAreaWidget.h>
#include <ads/DockContainerWidget.h>
#include <ads/DockWidgetTab.h>

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezQtApplicationPanel, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

ezDynamicArray<ezQtApplicationPanel*> ezQtApplicationPanel::s_AllApplicationPanels;

ezQtApplicationPanel::ezQtApplicationPanel(ads::CDockManager* pDockManager, const char* szPanelName)
  : ads::CDockWidget(pDockManager, szPanelName, ezQtContainerWindow::GetContainerWindow())
{
  ezStringBuilder sPanel("AppPanel_", szPanelName);

  setObjectName(ezMakeQString(sPanel));
  setWindowTitle(ezMakeQString(ezTranslate(szPanelName)));

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

bool ezQtApplicationPanel::event(QEvent* pEvent)
{
  if (pEvent->type() == QEvent::ShortcutOverride || pEvent->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
    if (ezQtProxy::TriggerDocumentAction(nullptr, keyEvent, pEvent->type() == QEvent::ShortcutOverride))
      return true;
  }
  return ads::CDockWidget::event(pEvent);
}
