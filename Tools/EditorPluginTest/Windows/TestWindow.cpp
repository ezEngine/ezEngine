#include <PCH.h>
#include <EditorPluginTest/Windows/TestWindow.moc.h>
#include <QSettings>

ezTestWindow::ezTestWindow(QWidget* parent) : QMainWindow(parent)
{
  setupUi(this);
}

ezTestWindow::~ezTestWindow()
{
}
