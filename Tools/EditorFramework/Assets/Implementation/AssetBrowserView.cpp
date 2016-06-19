#include <PCH.h>
#include <EditorFramework/Assets/AssetBrowserView.moc.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Logging/Log.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

#include <QWheelEvent>
#include <QApplication>
#include <QPainter>

ezAssetBrowserView::ezAssetBrowserView(QWidget* parent) : QListView(parent)
{
  m_iIconSizePercentage = 100;
  m_pDelegate = new QtIconViewDelegate(this);

  SetDialogMode(false);

  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
  setViewMode(QListView::ViewMode::IconMode);
  setUniformItemSizes(true);
  setResizeMode(QListView::ResizeMode::Adjust);
  
  setItemDelegate(m_pDelegate);
  SetIconScale(m_iIconSizePercentage);
}

void ezAssetBrowserView::SetDialogMode(bool bDialogMode)
{
  m_bDialogMode = bDialogMode;

  if (m_bDialogMode)
  {
    m_pDelegate->SetDrawTransformState(false);
    setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
    setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  }
  else
  {
    m_pDelegate->SetDrawTransformState(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  }
}

void ezAssetBrowserView::SetIconMode(bool bIconMode)
{
  if (bIconMode)
  {
    setViewMode(QListView::ViewMode::IconMode);
    SetIconScale(m_iIconSizePercentage);
  }
  else
  {
    setViewMode(QListView::ViewMode::ListMode);
    setGridSize(QSize());
  }
}

void ezAssetBrowserView::SetIconScale(ezInt32 iIconSizePercentage)
{
  m_iIconSizePercentage = ezMath::Clamp(iIconSizePercentage, 10, 100);
  m_pDelegate->SetIconScale(m_iIconSizePercentage);

  if (viewMode() != QListView::ViewMode::IconMode)
    return;

  setGridSize(m_pDelegate->sizeHint(QStyleOptionViewItem(), QModelIndex()));
}

ezInt32 ezAssetBrowserView::GetIconScale() const
{
  return m_iIconSizePercentage;
}


void ezAssetBrowserView::wheelEvent(QWheelEvent* pEvent)
{
  if(pEvent->modifiers() == Qt::CTRL)
  {
    ezInt32 iDelta = pEvent->delta() > 0 ? 5 : -5;
    SetIconScale(m_iIconSizePercentage + iDelta);
    emit ViewZoomed(m_iIconSizePercentage);
    return;
  }

  QListView::wheelEvent(pEvent);
}

QtIconViewDelegate::QtIconViewDelegate(ezAssetBrowserView* pParent) : QItemDelegate(pParent)
{
  m_bDrawTransformState = true;
  m_iIconSizePercentage = 100;
  m_pView = pParent;
}

void QtIconViewDelegate::SetIconScale(ezInt32 iIconSizePercentage)
{
  m_iIconSizePercentage = iIconSizePercentage;
}

void QtIconViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  if (!IsInIconMode())
  {
    QItemDelegate::paint(painter, opt, index);
    return;
  }

  const ezUInt32 uiThumbnailSize = ThumbnailSize();

  // Prepare painter.
  {
    painter->save();
    if (hasClipping())
      painter->setClipRect(opt.rect);

    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
  }

  // Draw highlight background (copy of QItemDelegate::drawBackground)
  {
    QRect highlightRect = opt.rect.adjusted(ItemSideMargin - HighlightBorderWidth, ItemSideMargin - HighlightBorderWidth, 0, 0);
    highlightRect.setHeight(uiThumbnailSize + 2 * HighlightBorderWidth);
    highlightRect.setWidth(uiThumbnailSize + 2 * HighlightBorderWidth);

    if ((opt.state & QStyle::State_Selected)) 
    {
      QPalette::ColorGroup cg = opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
      if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active))
        cg = QPalette::Inactive;

      painter->fillRect(highlightRect, opt.palette.brush(cg, QPalette::Highlight));
    } 
    else 
    {
      QVariant value = index.data(Qt::BackgroundRole);
      if (value.canConvert<QBrush>()) 
      {
        QPointF oldBO = painter->brushOrigin();
        painter->setBrushOrigin(highlightRect.topLeft());
        painter->fillRect(highlightRect, qvariant_cast<QBrush>(value));
        painter->setBrushOrigin(oldBO);
      }
    }
  }

  // Draw thumbnail.
  {
    QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin, 0, 0);
    thumbnailRect.setSize(QSize(uiThumbnailSize, uiThumbnailSize));
    QPixmap pixmap = qvariant_cast<QPixmap>(index.data(Qt::DecorationRole));
    painter->drawPixmap(thumbnailRect, pixmap);
  }

  // Draw icon.
  {
    QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin - 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
    thumbnailRect.setSize(QSize(16, 16));
    QPixmap pixmap = qvariant_cast<QPixmap>(index.data(ezAssetBrowserModel::UserRoles::AssetIconPath));
    painter->drawPixmap(thumbnailRect, pixmap);
  }

  // Draw Transform State Icon
  if (m_bDrawTransformState)
  {
    QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin + uiThumbnailSize - 16 + 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
    thumbnailRect.setSize(QSize(16, 16));

    ezAssetInfo::TransformState state = (ezAssetInfo::TransformState)index.data(ezAssetBrowserModel::UserRoles::TransformState).toInt();

    switch (state)
    {
    case ezAssetInfo::TransformState::Unknown:
      ezUIServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetUnknown16.png").paint(painter, thumbnailRect);
      break;
    case ezAssetInfo::TransformState::NeedsThumbnail:
      ezUIServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetNeedsThumbnail16.png").paint(painter, thumbnailRect);
      break;
    case ezAssetInfo::TransformState::NeedsTransform:
      ezUIServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetNeedsTransform16.png").paint(painter, thumbnailRect);
      break;
    case ezAssetInfo::TransformState::UpToDate:
      ezUIServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetOk16.png").paint(painter, thumbnailRect);
      break;
    }
  }

  // Draw caption.
  {
    painter->setFont(GetFont());
    QRect textRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin + uiThumbnailSize + TextSpacing,
                                      -ItemSideMargin, -ItemSideMargin - TextSpacing);

    QString caption = qvariant_cast<QString>(index.data(Qt::DisplayRole));
    painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWrapAnywhere, caption);
  }


  painter->restore();
}

QSize	QtIconViewDelegate::sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  if (IsInIconMode())
  {
    return ItemSize();
  }
  else
  {
    return QItemDelegate::sizeHint(opt, index);
  }
}

QSize QtIconViewDelegate::ItemSize() const
{
  QFont font = GetFont();
  QFontMetrics fm(font);

  ezUInt32 iThumbnail = ThumbnailSize();
  const ezUInt32 iItemWidth = iThumbnail + 2 * ItemSideMargin;
  const ezUInt32 iItemHeight = iThumbnail + 2 * (ItemSideMargin + fm.height() + TextSpacing);

  return QSize(iItemWidth, iItemHeight);
}

QFont QtIconViewDelegate::GetFont() const
{
  QFont font = QApplication::font();

  float fScaleFactor = ezMath::Clamp((1.0f + (m_iIconSizePercentage / 100.0f)) * 0.75f, 0.75f, 1.25f);

  font.setPointSizeF(font.pointSizeF() * fScaleFactor);
  return font;
}

ezUInt32 QtIconViewDelegate::ThumbnailSize() const
{
  return MaxSize * (float)m_iIconSizePercentage / 100.0f;
}

bool QtIconViewDelegate::IsInIconMode() const
{
  return m_pView->viewMode() == QListView::ViewMode::IconMode;
}
