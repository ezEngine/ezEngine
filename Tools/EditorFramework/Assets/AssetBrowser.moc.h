#pragma once

#include <EditorFramework/Plugin.h>
#include <Tools/EditorFramework/ui_AssetBrowser.h>
#include <EditorFramework/Assets/AssetCuratorModel.moc.h>

class ezAssetBrowser : public QWidget, public Ui_AssetBrowser
{
  Q_OBJECT

public:
  ezAssetBrowser(QWidget* parent);
  ~ezAssetBrowser();

private slots:
  void on_ListAssets_doubleClicked(const QModelIndex& index);
  void on_ButtonListMode_clicked();
  void on_ButtonIconMode_clicked();
  void on_IconSizeSlider_valueChanged(int iValue);
  void on_ListAssets_ViewZoomed(ezInt32 iIconSizePercentage);

private:
  ezAssetCuratorModel* m_pModel;

};