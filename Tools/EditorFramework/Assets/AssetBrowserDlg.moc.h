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


};