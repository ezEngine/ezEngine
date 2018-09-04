#pragma once

#include <EditorPluginScene/Plugin.h>
#include <Tools/EditorPluginScene/ui_ExtractGeometryDlg.h>
#include <QPushButton>

class ezQtExtractGeometryDlg : public QDialog, public Ui_ExtractGeometryDlg
{
  Q_OBJECT

public:
  ezQtExtractGeometryDlg(QWidget* parent);

  static QString s_sDestinationFile;
  static bool s_bOnlySelection;
  static int s_iExtractionMode;

private slots:
  void on_ButtonBox_clicked(QAbstractButton* button);
  void on_BrowseButton_clicked();

private:
  void UpdateUI();
  void QueryUI();

};
