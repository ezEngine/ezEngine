#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <Code/Tools/GuiFoundation/ui_QtWaitForOperationDlg.h>
#include <Foundation/Types/Delegate.h>

class QWinTaskbarProgress;
class QWinTaskbarButton;

class EZ_GUIFOUNDATION_DLL ezQtWaitForOperationDlg : public QDialog, public Ui_QtWaitForOperationDlg
{
  Q_OBJECT

public:
  ezQtWaitForOperationDlg(QWidget* parent);
  ~ezQtWaitForOperationDlg();

  ezDelegate<bool()> m_OnIdle;

private slots:
  void on_ButtonCancel_clicked();
  void onIdle();

private:

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  QWinTaskbarButton* m_pWinTaskBarButton;
  QWinTaskbarProgress* m_pWinTaskBarProgress;
#endif

};


