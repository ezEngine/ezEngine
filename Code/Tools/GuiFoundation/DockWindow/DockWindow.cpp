#include <GuiFoundation/PCH.h>
#include <GuiFoundation/DockWindow/DockWindow.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <QTimer>
#include <QSettings>
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

ezDynamicArray<ezApplicationPanel*> ezApplicationPanel::s_AllApplicationPanels;

ezApplicationPanel::ezApplicationPanel() : QDockWidget(nullptr)
{
  setBackgroundRole(QPalette::ColorRole::Highlight);

  s_AllApplicationPanels.PushBack(this);

  m_pContainerWindow = nullptr;

  ezContainerWindow::GetAllContainerWindows()[0]->MoveApplicationPanelToContainer(this);

}

ezApplicationPanel::~ezApplicationPanel()
{
  s_AllApplicationPanels.RemoveSwap(this);
}

void ezApplicationPanel::EnsureVisible()
{
  m_pContainerWindow->EnsureVisible(this);
}

void ezApplicationPanel::closeEvent(QCloseEvent* e)
{
  /// \todo For now prevent closing
  e->ignore();
}
