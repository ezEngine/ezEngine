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
  void OnServerStarted(const QString& ip, ezUInt16 uiPort);
  void OnServerStopped();

private:
  ezApplication* m_pApp;
  ezQtFileserveWidget* m_pFileserveWidget = nullptr;
};

void CreateFileserveMainWindow(ezApplication* pApp);

#endif

