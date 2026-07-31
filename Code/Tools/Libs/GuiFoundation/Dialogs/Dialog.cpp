#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Dialogs/Dialog.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

ezQtDialog::ezQtDialog(QWidget* pParent)
  : QDialog(pParent)
{
}

int ezQtDialog::exec()
{
  if (ezQtUiServices::IsUnattended())
  {
    // The Qt class name rather than a hand written string, so that no dialog can report a stale name, and
    // adding a dialog needs no further work. It is also what a caller would search the code base for.
    ezQtUiServices::ReportSuppressedDialog(metaObject()->className());
    return QDialog::Rejected;
  }

  return QDialog::exec();
}
