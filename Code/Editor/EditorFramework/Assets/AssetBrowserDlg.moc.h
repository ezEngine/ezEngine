#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetBrowserDlg.h>
#include <QDialog>

class EZ_EDITORFRAMEWORK_DLL ezQtAssetBrowserDlg : public QDialog, public Ui_AssetBrowserDlg
{
  Q_OBJECT

public:
  ezQtAssetBrowserDlg(QWidget* pParent, const ezUuid& preselectedAsset, ezStringView sVisibleFilters, ezStringView sWindowTitle = {}, ezStringView sRequiredTag = {});
  ezQtAssetBrowserDlg(QWidget* pParent, ezStringView sWindowTitle, ezStringView sPreselectedFileAbs, ezStringView sFileExtensions);
  ~ezQtAssetBrowserDlg();

  ezStringView GetSelectedAssetPathRelative() const { return m_sSelectedAssetPathRelative; }
  ezStringView GetSelectedAssetPathAbsolute() const { return m_sSelectedAssetPathAbsolute; }
  const ezUuid GetSelectedAssetGuid() const { return m_SelectedAssetGuid; }

private Q_SLOTS:
  void on_AssetBrowserWidget_ItemChosen(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, ezUInt8 uiAssetBrowserItemFlags);
  void on_AssetBrowserWidget_ItemSelected(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, ezUInt8 uiAssetBrowserItemFlags);
  void on_AssetBrowserWidget_ItemCleared();
  void on_ButtonSelect_clicked();

private:
  void Init(QWidget* pParent);

  ezString m_sSelectedAssetPathRelative;
  ezString m_sSelectedAssetPathAbsolute;
  ezUuid m_SelectedAssetGuid;
  ezString m_sVisibleFilters;
  ezString m_sRequiredTag;

  static bool s_bShowItemsInSubFolder;
  static bool s_bShowItemsInHiddenFolder;
  static bool s_bSortByRecentUse;
  static ezMap<ezString, ezString> s_TextFilter;
  static ezMap<ezString, ezString> s_PathFilter;
  static ezMap<ezString, ezString> s_TypeFilter;
};
