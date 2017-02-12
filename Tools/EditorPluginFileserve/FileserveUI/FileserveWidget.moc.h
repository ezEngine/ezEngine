#pragma once

#include <EditorPluginFileserve/Plugin.h>
#include <Tools/EditorPluginFileserve/ui_FileserveWidget.h>
#include <QDialog>

struct ezFileserverEvent;

class EZ_EDITORPLUGINFILESERVE_DLL ezQtFileserveWidget : public QWidget, public Ui_ezQtFileserveWidget
{
  Q_OBJECT

public:
  ezQtFileserveWidget(QWidget *parent = nullptr);
  ~ezQtFileserveWidget();

public slots:
  void on_StartServerButton_clicked();

private:
  void FileserverEventHandler(const ezFileserverEvent& e);
  void LogActivity(const ezFormatString& text);

  bool m_bServerRunning = false;
};

