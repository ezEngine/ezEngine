#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Delegate.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_QtWaitForOperationDlg.h>

class QWinTaskbarProgress;
class QWinTaskbarButton;

#undef EZ_USE_WIN_EXTRAS
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  define EZ_USE_WIN_EXTRAS EZ_ON
#else
#  define EZ_USE_WIN_EXTRAS EZ_OFF
#endif

class EZ_GUIFOUNDATION_DLL ezQtWaitForOperationDlg : public QDialog, public Ui_QtWaitForOperationDlg
{
  Q_OBJECT

public:
  ezQtWaitForOperationDlg(QWidget* parent);
  ~ezQtWaitForOperationDlg();

  ezDelegate<bool()> m_OnIdle;

private Q_SLOTS:
  void on_ButtonCancel_clicked();
  void onIdle();

private:
#if EZ_ENABLED(EZ_USE_WIN_EXTRAS)
  QWinTaskbarButton* m_pWinTaskBarButton;
  QWinTaskbarProgress* m_pWinTaskBarProgress;
#endif
};
