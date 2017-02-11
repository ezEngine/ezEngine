#pragma once

#include <EditorFramework/Plugin.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <QModelIndex>
#include <QLineEdit>
#include <EditorFramework/PropertyGrid/QtAssetLineEdit.moc.h>


/// *** Asset Browser ***

class EZ_EDITORFRAMEWORK_DLL ezQtAssetPropertyWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtAssetPropertyWidget();

  bool IsValidAssetType(const char* szAssetReference) const;

private slots:
  void on_BrowseFile_clicked();

protected slots:
  void on_TextFinished_triggered();
  void on_TextChanged_triggered(const QString& value);
  void ThumbnailLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2);
  void ThumbnailInvalidated(QString sPath, ezUInt32 uiImageID);
  void on_customContextMenuRequested(const QPoint& pt);
  void OnOpenAssetDocument();
  void OnSelectInAssetBrowser();
  void OnOpenExplorer();
  void OnCopyAssetGuid();
  void OnCreateNewAsset();
  void OnClearReference();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

protected:
  void UpdateThumbnail(const ezUuid& guid, const char* szThumbnailPath);

  QHBoxLayout* m_pLayout;
  ezQtAssetLineEdit* m_pWidget;
  QToolButton* m_pButton;
  ezUInt32 m_uiThumbnailID;
  ezUuid m_AssetGuid;
};
