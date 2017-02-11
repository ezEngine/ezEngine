#pragma once

#include <EditorPluginFileserve/Plugin.h>
#include <Tools/EditorPluginFileserve/ui_FileserveWidget.h>
#include <QDialog>

class EZ_EDITORPLUGINFILESERVE_DLL ezQtFileserveWidget : public QWidget, public Ui_ezQtFileserveWidget
{
  Q_OBJECT

public:
  ezQtFileserveWidget(QWidget *parent = nullptr);

public slots:

private:


};

