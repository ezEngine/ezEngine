#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>
#include <QPushButton>
#include <QLineEdit>
#include <QLayout>
#include <QKeyEvent>

ezQtSearchWidget::ezQtSearchWidget(QWidget* pParent)
{
  setLayout(new QHBoxLayout(this));
  setContentsMargins(0, 0, 0, 0);
  layout()->setContentsMargins(0, 0, 0, 0);


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

  emit textChanged(text);
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
        emit enterPressed();
        return true;
      }
    }
  }

  return false;
}
