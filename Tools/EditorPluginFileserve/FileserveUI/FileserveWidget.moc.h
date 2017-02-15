#pragma once

#include <EditorPluginFileserve/Plugin.h>
#include <Tools/EditorPluginFileserve/ui_FileserveWidget.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Containers/HashTable.h>
#include <QWidget>

struct ezFileserverEvent;
class ezQtFileserveActivityModel;
class ezQtFileserveAllFilesModel;
enum class ezFileserveActivityType;

/// \brief A GUI for the ezFileServer
///
/// By default the file server does not run at startup. Using the command line option "-fs_start" the server is started immediately.
class EZ_EDITORPLUGINFILESERVE_DLL ezQtFileserveWidget : public QWidget, public Ui_ezQtFileserveWidget
{
  Q_OBJECT

public:
  ezQtFileserveWidget(QWidget *parent = nullptr);

  void FindOwnIP(ezStringBuilder& out_Display, ezStringBuilder& out_FirstIP);

  ~ezQtFileserveWidget();

signals:
  void ServerStarted(const QString& ip, ezUInt16 uiPort);
  void ServerStopped();

public slots:
  void on_StartServerButton_clicked();
  void on_ClearActivityButton_clicked();
  void on_ClearAllFilesButton_clicked();

private:
  void FileserverEventHandler(const ezFileserverEvent& e);
  void LogActivity(const ezFormatString& text, ezFileserveActivityType type);
  void UpdateSpecialDirectoryUI();

  ezQtFileserveActivityModel* m_pActivityModel;
  ezQtFileserveAllFilesModel* m_pAllFilesModel;
  ezTime m_LastProgressUpdate;

  struct DataDirInfo
  {
    ezString m_sName;
    ezString m_sPath;
    ezString m_sRedirectedPath;
  };

  struct ClientData
  {
    bool m_bConnected = false;
    ezHybridArray<DataDirInfo, 8> m_DataDirs;
  };

  struct SpecialDir
  {
    ezString m_sName;
    ezString m_sPath;
  };

  ezHybridArray<SpecialDir, 4> m_SpecialDirectories;

  ezHashTable<ezUInt32, ClientData> m_Clients;
  void UpdateClientList();
  void ConfigureSpecialDirectories();
};

