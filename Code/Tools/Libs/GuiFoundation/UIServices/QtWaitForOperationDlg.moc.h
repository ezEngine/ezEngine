#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Delegate.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_QtWaitForOperationDlg.h>

class QWinTaskbarProgress;
class QWinTaskbarButton;

class EZ_GUIFOUNDATION_DLL ezQtWaitForOperationDlg : public ezQtDialog, public Ui_QtWaitForOperationDlg
{
  Q_OBJECT

public:
  ezQtWaitForOperationDlg(QWidget* pParent);
  ~ezQtWaitForOperationDlg();

  ezDelegate<bool()> m_OnIdle;

private Q_SLOTS:
  void on_ButtonCancel_clicked();
  void onIdle();
};
