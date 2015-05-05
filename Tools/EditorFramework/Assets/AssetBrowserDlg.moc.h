#pragma once

#include <EditorFramework/Plugin.h>
#include <Tools/EditorFramework/ui_AssetBrowserDlg.h>
#include <QDialog>

class ezAssetBrowserDlg : public QDialog, public Ui_AssetBrowserDlg
{
  Q_OBJECT

public:
  ezAssetBrowserDlg(QWidget* parent, const char* szPreselectedAsset, const char* szVisibleFilters);
  ~ezAssetBrowserDlg();

  const char* GetSelectedPath() const { return m_sSelectedPath; }

private slots:
  void on_ButtonFileDialog_clicked();
  void on_AssetBrowserWidget_ItemChosen(QString sItemPath);
  void on_AssetBrowserWidget_ItemSelected(QString sItemPath);
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();

private:
  ezStringBuilder m_sSelectedPath;
};