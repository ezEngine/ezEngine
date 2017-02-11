#pragma once

#include <Fileserve/Main.h>

#ifdef EZ_USE_QT

#include <QMainWindow>

class ezApplication;
class ezQtFileserveWidget;

class ezQtFileserveMainWnd : public QMainWindow
{
  Q_OBJECT
public:
  ezQtFileserveMainWnd(ezApplication* pApp, QWidget* parent = nullptr);

private slots:
  void UpdateNetworkSlot();

private:
  ezApplication* m_pApp;
  ezQtFileserveWidget* m_pFileserveWidget = nullptr;
};

void CreateFileserveMainWindow(ezApplication* pApp);

#endif

