#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Tools/EditorFramework/ui_AssetImportDlg.h>
#include <QDialog>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>

class ezQtAssetImportDlg : public QDialog, public Ui_AssetImportDlg
{
  Q_OBJECT

public:
  ezQtAssetImportDlg(QWidget* parent, ezDynamicArray<ezAssetDocumentGenerator::ImportData>& allImports);
  ~ezQtAssetImportDlg();

private Q_SLOTS:
  void SelectedOptionChanged(int index);
  void on_ButtonImport_clicked();
  void TableCellChanged(int row, int column);
  void BrowseButtonClicked(bool);

private:
  void InitRow(ezUInt32 uiRow);
  void UpdateRow(ezUInt32 uiRow);
  void QueryRow(ezUInt32 uiRow);
  void UpdateAllRows();

  ezDynamicArray<ezAssetDocumentGenerator::ImportData>& m_allImports;
};
