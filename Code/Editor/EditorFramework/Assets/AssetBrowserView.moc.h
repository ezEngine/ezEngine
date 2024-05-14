#pragma once

#include <EditorFramework/Assets/AssetBrowserFolderView.moc.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/Widgets/ItemView.moc.h>
#include <QItemDelegate>
#include <QListView>

class ezQtIconViewDelegate;

class ezQtAssetBrowserView : public ezQtItemView<QListView>
{
  Q_OBJECT

public:
  ezQtAssetBrowserView(QWidget* pParent);
  void SetDialogMode(bool bDialogMode);

  void SetIconMode(bool bIconMode);
  void SetIconScale(ezInt32 iIconSizePercentage);
  ezInt32 GetIconScale() const;

  void dragEnterEvent(QDragEnterEvent* event) override;
  void dragMoveEvent(QDragMoveEvent* event) override;
  void dragLeaveEvent(QDragLeaveEvent* event) override;
  void dropEvent(QDropEvent* event) override;

Q_SIGNALS:
  void ViewZoomed(ezInt32 iIconSizePercentage);

protected:
  virtual void wheelEvent(QWheelEvent* pEvent) override;

private:
  bool m_bDialogMode;
  ezQtIconViewDelegate* m_pDelegate;
  ezInt32 m_iIconSizePercentage;
};


class ezQtIconViewDelegate : public ezQtItemDelegate
{
  Q_OBJECT

public:
  ezQtIconViewDelegate(ezQtAssetBrowserView* pParent = nullptr);

  void SetDrawTransformState(bool b) { m_bDrawTransformState = b; }

  void SetIconScale(ezInt32 iIconSizePercentage);

  virtual bool mousePressEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual bool mouseReleaseEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;


public:
  virtual void paint(QPainter* pPainter, const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual QSize sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual QWidget* createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
  virtual void setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const override;
  virtual void updateEditorGeometry(QWidget* pEditor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
  QSize ItemSize() const;
  QFont GetFont() const;
  ezUInt32 ThumbnailSize() const;
  bool IsInIconMode() const;

private:
  enum
  {
    MaxSize = ezThumbnailSize,
    HighlightBorderWidth = 3,
    ItemSideMargin = 5,
    TextSpacing = 5
  };

  bool m_bDrawTransformState;
  ezInt32 m_iIconSizePercentage;
  ezQtAssetBrowserView* m_pView;
};
