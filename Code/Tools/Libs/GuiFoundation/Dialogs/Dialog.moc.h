#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <QDialog>

/// Base class for the editor's modal dialogs, which does nothing except refuse to block when no user is present.
///
/// A QDialog::exec() runs a nested event loop and only returns once someone closes the window. In an
/// unattended session (see ezQtUiServices::IsUnattended()) no one ever does, so the application hangs for
/// good - it cannot even be asked to quit, because that request would be handled by the very event loop
/// that is stuck. Deriving from this class turns that into a rejected dialog plus a log message.
///
/// Derive from it instead of QDialog for anything that is exec()ed. Dialogs that are only ever show()n
/// don't block and don't need it, but there is no harm in deriving anyway, and doing so uniformly means
/// nobody has to check which kind a dialog is when adding an exec() call later.
///
/// Note that this only unblocks the caller - it does not make the dialog's functionality available to an
/// automated caller. Whatever the dialog would have configured has to be reachable some other way for
/// that, e.g. through a dedicated function that takes the same settings as arguments.
class EZ_GUIFOUNDATION_DLL ezQtDialog : public QDialog
{
  Q_OBJECT

public:
  ezQtDialog(QWidget* pParent);

  /// Shows the dialog modally, unless no user is present, in which case it reports itself as suppressed
  /// (see ezQtUiServices::ReportSuppressedDialog()) and returns QDialog::Rejected without showing anything.
  virtual int exec() override;
};
