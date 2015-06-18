#include <GuiFoundation/PCH.h>
#include <GuiFoundation/DockWindow/DockWindow.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <QTimer>
#include <QSettings>

ezDocumentPanel::ezDocumentPanel(QWidget* parent) : QDockWidget(parent)
{
  setBackgroundRole(QPalette::ColorRole::Highlight);

  setFeatures(DockWidgetFeature::DockWidgetFloatable | DockWidgetFeature::DockWidgetMovable);
}

ezDocumentPanel::~ezDocumentPanel()
{
}


ezDynamicArray<ezApplicationPanel*> ezApplicationPanel::s_AllApplicationPanels;

ezApplicationPanel::ezApplicationPanel() : QDockWidget(nullptr)
{
  setBackgroundRole(QPalette::ColorRole::Highlight);

  s_AllApplicationPanels.PushBack(this);

  m_pContainerWindow = nullptr;

  /// \todo For now prevent closing
  setFeatures(DockWidgetFeature::DockWidgetFloatable | DockWidgetFeature::DockWidgetMovable);

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


