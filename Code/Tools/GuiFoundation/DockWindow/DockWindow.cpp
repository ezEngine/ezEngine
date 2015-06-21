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

ezApplicationPanel::ezApplicationPanel(const char* szPanelName) : QDockWidget(ezContainerWindow::GetAllContainerWindows()[0])
{
  ezStringBuilder sPanel("AppPanel_", szPanelName);

  setObjectName(QString::fromUtf8(sPanel.GetData()));
  setWindowTitle(QString::fromUtf8(szPanelName));

  setBackgroundRole(QPalette::ColorRole::Highlight);

  s_AllApplicationPanels.PushBack(this);

  m_pContainerWindow = nullptr;

  ezContainerWindow::GetAllContainerWindows()[0]->MoveApplicationPanelToContainer(this);

  EZ_ASSERT_DEV(parent() != nullptr, "Invalid Qt parent window");
}

ezApplicationPanel::~ezApplicationPanel()
{
  s_AllApplicationPanels.RemoveSwap(this);
}

void ezApplicationPanel::EnsureVisible()
{
  m_pContainerWindow->EnsureVisible(this);

  EZ_ASSERT_DEV(parent() != nullptr, "Invalid Parent!");
}

void ezApplicationPanel::closeEvent(QCloseEvent* e)
{
  e->accept();

  EZ_ASSERT_DEV(parent() != nullptr, "Invalid Parent!");
}
