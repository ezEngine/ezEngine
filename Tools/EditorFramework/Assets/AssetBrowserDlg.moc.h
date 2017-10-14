#pragma once

#include <EditorFramework/Plugin.h>
#include <Tools/EditorFramework/ui_AssetBrowserDlg.h>
#include <QDialog>

class ezQtAssetBrowserDlg : public QDialog, public Ui_AssetBrowserDlg
{
  Q_OBJECT

public:
  ezQtAssetBrowserDlg(QWidget* parent, const ezUuid& preselectedAsset, const char* szVisibleFilters);
  ~ezQtAssetBrowserDlg();

  const char* GetSelectedAssetPathRelative() const { return m_sSelectedAssetPathRelative; }
  const char* GetSelectedAssetPathAbsolute() const { return m_sSelectedAssetPathAbsolute; }
  const ezUuid GetSelectedAssetGuid() const { return m_SelectedAssetGuid; }

private slots:
  void on_ButtonFileDialog_clicked();
  void on_AssetBrowserWidget_ItemChosen(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute);
  void on_AssetBrowserWidget_ItemSelected(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute);
  void on_AssetBrowserWidget_ItemCleared();
  void on_ButtonSelect_clicked();

private:
  ezString m_sSelectedAssetPathRelative;
  ezString m_sSelectedAssetPathAbsolute;
  ezUuid m_SelectedAssetGuid;
  ezString m_sVisibleFilters;

  static bool s_bShowItemsInSubFolder;
  static bool s_bShowItemsInHiddenFolder;
  static bool s_bSortByRecentUse;
  static ezMap<ezString, ezString> s_sTextFilter;
  static ezMap<ezString, ezString> s_sPathFilter;
  static ezMap<ezString, ezString> s_sTypeFilter;
};
