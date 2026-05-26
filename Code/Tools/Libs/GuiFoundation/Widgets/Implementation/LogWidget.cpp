#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Models/LogModel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/LogWidget.moc.h>
#include <QClipboard>
#include <QKeyEvent>
#include <QRegularExpression>

//////////////////////////////////////////////////////////////////////////


ezQtLogWidgetItemDelegate::ezQtLogWidgetItemDelegate(QObject* pParent)
  : ezQtItemDelegate(pParent)
{
}

void ezQtLogWidgetItemDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  const QVariant link = index.data(ezQtLogModel::UserRoles::Link);

  if (!link.isValid())
  {
    ezQtItemDelegate::paint(pPainter, option, index);
    return;
  }

  const QVariant linkText = index.data(ezQtLogModel::UserRoles::LinkText);

  const QString sLink = link.toString();
  const QString sLinkText = linkText.isValid() ? linkText.toString() : QString();
  const QString sLogText = index.data(Qt::DisplayRole).toString();

  drawBackground(pPainter, option, index);

  const bool bSelected = option.state & QStyle::State_Selected;
  const QColor textColor = bSelected ? option.palette.highlightedText().color() : option.palette.text().color();
  const QString sLinkColor = bSelected ? option.palette.link().color().name() : option.palette.linkVisited().color().name();

  QString sHtml = sLogText.toHtmlEscaped();
  const bool bHovered = (index == m_HoveredIndex);
  const QString sDecoration = bHovered ? QStringLiteral("; text-decoration:underline;") : QString();
  sHtml.replace(sLink.toHtmlEscaped(), QStringLiteral("<span style=\"color:") + sLinkColor + sDecoration + QStringLiteral(";\">") + sLinkText.toHtmlEscaped() + QString("</span>"));

  m_Doc.setDefaultFont(option.font);
  m_Doc.setDefaultStyleSheet(QStringLiteral("body { color: ") + textColor.name() + QStringLiteral("; }"));
  m_Doc.setHtml(QStringLiteral("<body>") + sHtml + QStringLiteral("</body>"));
  m_Doc.setDocumentMargin(0);

  pPainter->save();
  pPainter->translate(option.rect.left(), option.rect.top());
  m_Doc.drawContents(pPainter, QRectF(0, 0, option.rect.width(), option.rect.height()));
  pPainter->restore();
}

bool ezQtLogWidgetItemDelegate::mouseHoverEvent(QHoverEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  if (pEvent->type() == QEvent::HoverEnter)
  {
    m_HoveredIndex = index;
  }
  else if (pEvent->type() == QEvent::HoverLeave)
  {
    m_HoveredIndex = QModelIndex();
  }

  return false;
}

bool ezQtLogWidgetItemDelegate::mouseDoubleClickEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  QVariant linkTarget = index.data(ezQtLogModel::UserRoles::LinkTarget);
  if (!linkTarget.isValid())
  {
    return false;
  }

  ezQtUiServices::GotoLinkTarget(qtToEzString(linkTarget.toString()));
  return true;
}


bool ezQtLogWidgetItemDelegate::mousePressEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  if (pEvent->button() == Qt::MouseButton::MiddleButton)
  {
    // necessary, so that we get mouseReleaseEvent
    return true;
  }

  return false;
}

bool ezQtLogWidgetItemDelegate::mouseReleaseEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  if (pEvent->button() == Qt::MouseButton::MiddleButton)
  {
    return mouseDoubleClickEvent(pEvent, option, index);
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////

ezMap<ezString, ezQtLogWidget::LogItemContextActionCallback> ezQtLogWidget::s_LogCallbacks;

ezQtLogWidget::ezQtLogWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setupUi(this);

  m_pDelegate = new ezQtLogWidgetItemDelegate(this);

  m_pLog = new ezQtLogModel(this);
  ListViewLog->setModel(m_pLog);
  ListViewLog->setUniformItemSizes(true);
  ListViewLog->installEventFilter(this);
  ListViewLog->setItemDelegate(m_pDelegate);
  connect(m_pLog, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex& parent, int iFirst, int iLast)
    { ScrollToBottomIfAtEnd(iFirst); });
  connect(ListViewLog, &QAbstractItemView::doubleClicked, this, &ezQtLogWidget::OnItemDoubleClicked);

  const int logIndex = ((int)ezLogMsgType::All - (int)ezLogMsgType::InfoMsg);
  ComboFilter->setCurrentIndex(logIndex);
}

ezQtLogWidget::~ezQtLogWidget() = default;

void ezQtLogWidget::ShowControls(bool bShow)
{
  ButtonClearLog->setVisible(bShow);
  ComboFilter->setVisible(bShow);
  Search->setVisible(bShow);
}

ezQtLogModel* ezQtLogWidget::GetLog()
{
  return m_pLog;
}

ezQtSearchWidget* ezQtLogWidget::GetSearchWidget()
{
  return Search;
}

void ezQtLogWidget::SetLogLevel(ezLogMsgType::Enum logLevel)
{
  EZ_ASSERT_DEBUG(logLevel >= (int)ezLogMsgType::ErrorMsg && logLevel <= ezLogMsgType::All, "Invalid log level set.");
  ComboFilter->setCurrentIndex((int)ezLogMsgType::All - (int)logLevel);
}

ezLogMsgType::Enum ezQtLogWidget::GetLogLevel() const
{
  int index = ComboFilter->currentIndex();
  return (ezLogMsgType::Enum)((int)ezLogMsgType::All - index);
}

bool ezQtLogWidget::eventFilter(QObject* pObject, QEvent* pEvent)
{
  if (pObject == ListViewLog)
  {
    if (pEvent->type() == QEvent::ShortcutOverride)
    {
      // Intercept copy
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
      if (keyEvent->matches(QKeySequence::StandardKey::Copy))
      {
        keyEvent->accept();
        return true;
      }
    }
    else if (pEvent->type() == QEvent::KeyPress)
    {
      // Copy entire selection
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
      if (keyEvent->matches(QKeySequence::StandardKey::Copy))
      {
        QModelIndexList selection = ListViewLog->selectionModel()->selectedRows(0);
        std::sort(selection.begin(), selection.end());

        QStringList sTemp;
        sTemp.reserve(selection.count());
        for (const QModelIndex& index : selection)
        {
          QString sLine = m_pLog->data(index, Qt::DisplayRole).toString();
          sTemp.push_back(sLine);
        }

        QString sFullText = sTemp.join(QStringLiteral("\n"));
        QApplication::clipboard()->setText(sFullText);
        keyEvent->accept();
        return true;
      }
    }
  }

  return false;
}

bool ezQtLogWidget::AddLogItemContextActionCallback(const ezStringView& sName, const LogItemContextActionCallback& logCallback)
{
  if (sName.IsEmpty())
    return false;

  if (s_LogCallbacks.Contains(sName))
    return false;

  s_LogCallbacks[sName] = logCallback;
  return true;
}

bool ezQtLogWidget::RemoveLogItemContextActionCallback(const ezStringView& sName)
{
  if (sName.IsEmpty())
    return false;

  return s_LogCallbacks.Remove(sName);
}

void ezQtLogWidget::ScrollToBottomIfAtEnd(int iFirstNewRow)
{
  if (ListViewLog->selectionModel()->hasSelection())
  {
    if (ListViewLog->selectionModel()->selectedIndexes()[0].row() + 1 < iFirstNewRow)
      return;

    ListViewLog->selectionModel()->clearSelection();
  }
  ListViewLog->scrollToBottom();
}

void ezQtLogWidget::on_ButtonClearLog_clicked()
{
  m_pLog->Clear();
}

void ezQtLogWidget::on_Search_textChanged(const QString& text)
{
  m_pLog->SetSearchText(text.toUtf8().data());
}

void ezQtLogWidget::on_ComboFilter_currentIndexChanged(int index)
{
  const ezLogMsgType::Enum LogLevel = (ezLogMsgType::Enum)((int)ezLogMsgType::All - index);
  m_pLog->SetLogLevel(LogLevel);
}

void ezQtLogWidget::OnItemDoubleClicked(QModelIndex idx)
{
  const ezString sLine(m_pLog->data(idx, Qt::DisplayRole).toString().toUtf8().data());

  for (auto const& callback : s_LogCallbacks)
  {
    callback.Value()(sLine);
  }
}
