#include <GuiFoundationPCH.h>

#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <QCloseEvent>

ezDynamicArray<ezQtDocumentPanel*> ezQtDocumentPanel::s_AllDocumentPanels;

ezQtDocumentPanel::ezQtDocumentPanel(QWidget* parent)
    : QDockWidget(parent)
{
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
