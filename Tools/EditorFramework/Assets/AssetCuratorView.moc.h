#pragma once

#include <EditorFramework/Plugin.h>
#include <QListView>
#include <QItemDelegate>

class QtIconViewDelegate;

class ezAssetCuratorView : public QListView
{
  Q_OBJECT

public:
  ezAssetCuratorView(QWidget* pParent);

  void SetIconMode(bool bIconMode);
  void SetIconScale(ezInt32 iIconSizePercentage);
  ezInt32 GetIconScale() const;

signals:
  void ViewZoomed(ezInt32 iIconSizePercentage);

protected:
  virtual void wheelEvent(QWheelEvent* pEvent) override;

private:
  QtIconViewDelegate* m_pDelegate;
  ezInt32 m_iIconSizePercentage;
};


class QtIconViewDelegate : public QItemDelegate
{
  Q_OBJECT
public:
  QtIconViewDelegate(ezAssetCuratorView* pParent = nullptr);

  void SetIconScale(ezInt32 iIconSizePercentage);
  QSize ItemSize() const;

public:
  virtual void paint(QPainter* painter, const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual QSize	sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const override;

private:
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

  ezInt32 m_iIconSizePercentage;
  ezAssetCuratorView* m_pView;
};