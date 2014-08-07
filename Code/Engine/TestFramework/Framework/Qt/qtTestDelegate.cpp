#include <TestFramework/PCH.h>
#ifdef EZ_USE_QT

#include <TestFramework/Framework/Qt/qtTestFramework.h>
#include <TestFramework/Framework/Qt/qtTestDelegate.h>
#include <TestFramework/Framework/Qt/qtTestModel.h>
#include <QPainter>
#include <QApplication>
#include <QStyleOptionViewItem>

////////////////////////////////////////////////////////////////////////
// ezQtTestDelegate public functions
////////////////////////////////////////////////////////////////////////

ezQtTestDelegate::ezQtTestDelegate(QObject* pParent) : QStyledItemDelegate(pParent)
{
}

ezQtTestDelegate::~ezQtTestDelegate()
{
}

void ezQtTestDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  if (index.column() == ezQtTestModel::Columns::Duration)
  {
    // We need to draw the alternate background color here because setting it via the model would
    // overwrite our duration bar.
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(option.palette.alternateBase());
    painter->drawRect(option.rect);
    painter->restore();

    bool bSuccess = false;
    float fProgress = index.data(ezQtTestModel::UserRoles::Duration).toFloat(&bSuccess);
    
    // If we got a valid float from the model we can draw a small duration bar on top of the background.
    if (bSuccess)
    {
      QColor DurationColor = index.data(ezQtTestModel::UserRoles::DurationColor).value<QColor>();
      QStyleOptionViewItem option2 = option;
      option2.palette.setBrush(QPalette::Base, QBrush(DurationColor));
      option2.rect.setWidth((int)((float)option2.rect.width() * fProgress));
      QApplication::style()->drawControl(QStyle::CE_ProgressBarGroove, &option2, painter);
    }
  }

  QStyledItemDelegate::paint(painter, option, index);
}

#endif

EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_Qt_qtTestDelegate);

