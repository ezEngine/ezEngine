#pragma once

#include <EditorFramework/Plugin.h>
#include <Tools/EditorFramework/ui_AssetImportDlg.h>
#include <QDialog>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>

class ezQtAssetImportDlg : public QDialog, public Ui_AssetImportDlg
{
  Q_OBJECT

public:
  ezQtAssetImportDlg(QWidget* parent, ezDynamicArray<ezAssetDocumentGenerator::ImportData>& allImports);
  ~ezQtAssetImportDlg();

private slots:
  void SelectedOptionChanged(int index);
  void on_ButtonImport_clicked();

private:
  void InitRow(ezUInt32 uiRow);
  void UpdateRow(ezUInt32 uiRow);
  void QueryRow(ezUInt32 uiRow);

  ezDynamicArray<ezAssetDocumentGenerator::ImportData>& m_allImports;
};
