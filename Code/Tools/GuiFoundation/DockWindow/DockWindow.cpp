#include <GuiFoundation/PCH.h>
#include <GuiFoundation/DockWindow/DockWindow.moc.h>

ezDockWindow::ezDockWindow(QWidget* parent) : QDockWidget(parent)
{
  setBackgroundRole(QPalette::ColorRole::Highlight);


}

ezDockWindow::~ezDockWindow()
{
}
