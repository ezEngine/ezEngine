#include <GuiFoundationPCH.h>

#include <GuiFoundation/Widgets/SearchWidget.moc.h>
#include <QKeyEvent>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>

ezQtSearchWidget::ezQtSearchWidget(QWidget* pParent)
{
  setLayout(new QHBoxLayout(this));
  setContentsMargins(0, 0, 0, 0);
  layout()->setContentsMargins(0, 0, 0, 0);
  layout()->setSpacing(0);

  {
    m_pClearButton = new QPushButton(this);
    m_pClearButton->setAutoDefault(false);
    m_pClearButton->setDefault(false);
    m_pClearButton->setEnabled(false);
    m_pClearButton->setIcon(QIcon(":/GuiFoundation/Icons/Delete16.png"));
  }

  {
    m_pLineEdit = new QLineEdit(this);
    m_pLineEdit->setPlaceholderText("Search");
    m_pLineEdit->installEventFilter(this);

    setFocusProxy(m_pLineEdit);
  }

  layout()->addWidget(m_pLineEdit);
  layout()->addWidget(m_pClearButton);

  connect(m_pLineEdit, &QLineEdit::textChanged, this, &ezQtSearchWidget::onLineEditTextChanged);
  connect(m_pClearButton, &QPushButton::clicked, this, &ezQtSearchWidget::onClearButtonClicked);
}

void ezQtSearchWidget::setText(const QString& text)
{
  m_pLineEdit->setText(text);
}

QString ezQtSearchWidget::text() const
{
  return m_pLineEdit->text();
}

void ezQtSearchWidget::setPlaceholderText(const QString& text)
{
  m_pLineEdit->setPlaceholderText(text);
}

void ezQtSearchWidget::onLineEditTextChanged(const QString& text)
{
  m_pClearButton->setEnabled(!text.isEmpty());

  Q_EMIT textChanged(text);
}

void ezQtSearchWidget::onClearButtonClicked(bool checked)
{
  m_pLineEdit->setText(QString());
  m_pLineEdit->setFocus();
}

bool ezQtSearchWidget::eventFilter(QObject* obj, QEvent* e)
{
  if (obj == m_pLineEdit)
  {
    if (e->type() == QEvent::KeyPress)
    {
      QKeyEvent* pEvent = static_cast<QKeyEvent*>(e);

      if (pEvent->key() == Qt::Key_Escape && !text().isEmpty())
      {
        setText("");
        return true;
      }

      if (pEvent->key() == Qt::Key_Return || pEvent->key() == Qt::Key_Enter)
      {
        Q_EMIT enterPressed();
        return true;
      }

      if (pEvent->key() == Qt::Key_Up || pEvent->key() == Qt::Key_Down || pEvent->key() == Qt::Key_Tab || pEvent->key() == Qt::Key_Backtab)
      {
        Q_EMIT specialKeyPressed((Qt::Key)pEvent->key());
        return true;
      }
    }
  }

  return false;
}
