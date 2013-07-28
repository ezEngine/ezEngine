#include <PCH.h>
#include <InputTest/InputTableView.moc.h>
 #include <QKeyEvent>

InputTableView::InputTableView(QWidget* parent) : QTableView(parent)
{
}

void InputTableView::keyPressEvent(QKeyEvent* event)
{
  event->accept();
}