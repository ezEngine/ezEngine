#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetImportDlg.h>
#include <QDialog>

class ezQtAssetImportDlg : public QDialog, public Ui_AssetImportDlg
{
  Q_OBJECT

public:
  ezQtAssetImportDlg(QWidget* pParent, ezDynamicArray<ezAssetDocumentGenerator::ImportData>& ref_allImports);
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

  ezDynamicArray<ezAssetDocumentGenerator::ImportData>& m_AllImports;
};

