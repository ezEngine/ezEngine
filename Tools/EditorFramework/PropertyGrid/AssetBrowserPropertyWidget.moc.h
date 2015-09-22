#pragma once

#include <EditorFramework/Plugin.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <QModelIndex>
#include <QLineEdit>

class EZ_EDITORFRAMEWORK_DLL ezAssetLineEdit : public QLineEdit
{
  Q_OBJECT

public:

  explicit ezAssetLineEdit(QWidget* parent = nullptr);
  virtual void dragMoveEvent(QDragMoveEvent *e) override;
  virtual void dragEnterEvent(QDragEnterEvent * e) override;
  virtual void dropEvent(QDropEvent* e) override;
};

/// *** Asset Browser ***

class EZ_EDITORFRAMEWORK_DLL ezAssetBrowserPropertyWidget : public ezStandardPropertyBaseWidget
{
  Q_OBJECT

public:
  ezAssetBrowserPropertyWidget();

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

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

protected:
  void UpdateThumbnail(const ezUuid& guid, const char* szThumbnailPath);

  QHBoxLayout* m_pLayout;
  ezAssetLineEdit* m_pWidget;
  QToolButton* m_pButton;
  ezUInt32 m_uiThumbnailID;
  ezUuid m_AssetGuid;
};
