#include <GuiFoundation/PCH.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <QCloseEvent>

ezDynamicArray<ezDocumentPanel*> ezDocumentPanel::s_AllDocumentPanels;

ezDocumentPanel::ezDocumentPanel(QWidget* parent) : QDockWidget(parent)
{
  s_AllDocumentPanels.PushBack(this);

  setBackgroundRole(QPalette::ColorRole::Highlight);

  setFeatures(DockWidgetFeature::DockWidgetFloatable | DockWidgetFeature::DockWidgetMovable);
}

ezDocumentPanel::~ezDocumentPanel()
{
  s_AllDocumentPanels.RemoveSwap(this);
}

void ezDocumentPanel::closeEvent(QCloseEvent* e)
{
  e->ignore();
}
