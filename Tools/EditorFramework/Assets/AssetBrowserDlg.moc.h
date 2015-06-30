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

  const char* GetSelectedAssetPathRelative() const { return m_sSelectedAssetPathRelative; }
  const char* GetSelectedAssetPathAbsolute() const { return m_sSelectedAssetPathAbsolute; }
  const char* GetSelectedAssetGuid() const { return m_sSelectedAssetGuid; }

private slots:
  void on_ButtonFileDialog_clicked();
  void on_AssetBrowserWidget_ItemChosen(QString sAssetGUID, QString sAssetPathRelative, QString sAssetPathAbsolute);
  void on_AssetBrowserWidget_ItemSelected(QString sAssetGUID, QString sAssetPathRelative, QString sAssetPathAbsolute);
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();

private:
  ezString m_sSelectedAssetPathRelative;
  ezString m_sSelectedAssetPathAbsolute;
  ezString m_sSelectedAssetGuid;
  ezString m_sVisibleFilters;

  static bool s_bShowItemsInSubFolder;
  static bool s_bSortByRecentUse;
  static ezMap<ezString, ezString> s_sTextFilter;
  static ezMap<ezString, ezString> s_sPathFilter;
  static ezMap<ezString, ezString> s_sTypeFilter;
};