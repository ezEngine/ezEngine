#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <QCloseEvent>

ezDynamicArray<ezQtDocumentPanel*> ezQtDocumentPanel::s_AllDocumentPanels;

ezQtDocumentPanel::ezQtDocumentPanel(QWidget* pParent, ezDocument* pDocument)
  : QDockWidget(pParent)
{
  m_pDocument = pDocument;
  s_AllDocumentPanels.PushBack(this);

  setBackgroundRole(QPalette::ColorRole::Highlight);

  setFeatures(DockWidgetFeature::DockWidgetFloatable | DockWidgetFeature::DockWidgetMovable);
}

ezQtDocumentPanel::~ezQtDocumentPanel()
{
  s_AllDocumentPanels.RemoveAndSwap(this);
}

void ezQtDocumentPanel::closeEvent(QCloseEvent* e)
{
  e->ignore();
}

bool ezQtDocumentPanel::event(QEvent* pEvent)
{
  if (pEvent->type() == QEvent::ShortcutOverride)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
    if (ezQtProxy::TriggerDocumentAction(m_pDocument, keyEvent))
      return true;
  }
  return QDockWidget::event(pEvent);
}
