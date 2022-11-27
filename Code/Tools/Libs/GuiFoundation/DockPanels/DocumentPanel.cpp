#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <QCloseEvent>

ezDynamicArray<ezQtDocumentPanel*> ezQtDocumentPanel::s_AllDocumentPanels;

ezQtDocumentPanel::ezQtDocumentPanel(QWidget* parent, ezDocument* pDocument)
  : QDockWidget(parent)
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

bool ezQtDocumentPanel::event(QEvent* event)
{
  if (event->type() == QEvent::ShortcutOverride)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if (ezQtProxy::TriggerDocumentAction(m_pDocument, keyEvent))
      return true;
  }
  return QDockWidget::event(event);
}
