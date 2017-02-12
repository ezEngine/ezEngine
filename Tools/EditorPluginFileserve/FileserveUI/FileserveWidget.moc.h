#pragma once

#include <EditorPluginFileserve/Plugin.h>
#include <Tools/EditorPluginFileserve/ui_FileserveWidget.h>
#include <Foundation/Time/Time.h>
#include <QWidget>

struct ezFileserverEvent;
class ezQtFileserveActivityModel;
class ezQtFileserveAllFilesModel;
enum class ezFileserveActivityType;

class EZ_EDITORPLUGINFILESERVE_DLL ezQtFileserveWidget : public QWidget, public Ui_ezQtFileserveWidget
{
  Q_OBJECT

public:
  ezQtFileserveWidget(QWidget *parent = nullptr);
  ~ezQtFileserveWidget();

public slots:
  void on_StartServerButton_clicked();
  void on_ClearActivityButton_clicked();
  void on_ClearAllFilesButton_clicked();

private:
  void FileserverEventHandler(const ezFileserverEvent& e);
  void LogActivity(const ezFormatString& text, ezFileserveActivityType type);

  bool m_bServerRunning = false;
  ezQtFileserveActivityModel* m_pActivityModel;
  ezQtFileserveAllFilesModel* m_pAllFilesModel;
  ezTime m_LastProgressUpdate;
};

