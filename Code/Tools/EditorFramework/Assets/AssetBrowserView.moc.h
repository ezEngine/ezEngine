#pragma once

#include <EditorFramework/Plugin.h>
#include <QListView>
#include <QItemDelegate>
#include <GuiFoundation/Widgets/ItemView.moc.h>

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

  virtual bool mousePressEvent(QMouseEvent* event, const QStyleOptionViewItem& option, const QModelIndex& index);
  virtual bool mouseReleaseEvent(QMouseEvent* event, const QStyleOptionViewItem& option, const QModelIndex& index);

public:
  virtual void paint(QPainter* painter, const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual QSize	sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const override;

private:
  QSize ItemSize() const;
  QFont GetFont() const;
  ezUInt32 ThumbnailSize() const;
  bool IsInIconMode() const;

private:
  enum
  {
    MaxSize = 256,
    HighlightBorderWidth = 3,
    ItemSideMargin = 5,
    TextSpacing = 5
  };

  bool m_bDrawTransformState;
  ezInt32 m_iIconSizePercentage;
  ezQtAssetBrowserView* m_pView;
};
