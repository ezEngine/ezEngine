#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetImportDlg.h>
#include <QDialog>

class ezQtAssetImportDlg : public QDialog, public Ui_AssetImportDlg
{
  Q_OBJECT

public:
  ezQtAssetImportDlg(QWidget* pParent, ezDynamicArray<ezAssetDocumentGenerator::ImportGroupOptions>& ref_allImports);
  ~ezQtAssetImportDlg();

private Q_SLOTS:
  void SelectedOptionChanged(int index);
  void on_ButtonImport_clicked();

private:
  void InitRow(ezUInt32 uiRow);

  ezDynamicArray<ezAssetDocumentGenerator::ImportGroupOptions>& m_AllImports;
};
