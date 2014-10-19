#include <PCH.h>
#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qlayout.h>
#include <QLineEdit>
#include <QLabel>
#include <QKeyEvent>
#include <EditorFramework/GUI/QtHelpers.h>

ezPropertyEditorBaseWidget::ezPropertyEditorBaseWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent) : QWidget(pParent)
{
  m_szDisplayName = szName;
  m_PropertyPath = path;
}

void ezPropertyEditorBaseWidget::SetValue(const ezVariant& value)
{
  m_OldValue = value;

  InternalSetValue(value);
}

void ezPropertyEditorBaseWidget::BroadcastValueChanged(const ezVariant& NewValue)
{
  if (NewValue == m_OldValue)
    return;

  m_OldValue = NewValue;

  EventData ed;
  ed.m_pPropertyPath = &m_PropertyPath;
  ed.m_Value = NewValue;

  m_ValueChanged.Broadcast(ed);
}

void ezPropertyEditorBaseWidget::keyPressEvent(QKeyEvent* pEvent)
{
  if (pEvent->key() == Qt::Key::Key_Escape)
  {
    SetValue(m_OldValue);
  }

}

ezPropertyEditorCheckboxWidget::ezPropertyEditorCheckboxWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent) : ezPropertyEditorBaseWidget(path, szName, pParent)
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pLabel = new QLabel(this);
  m_pLabel->setText(QString::fromUtf8(szName));
  m_pLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

  m_pWidget = new QCheckBox(this);

  QSizePolicy policy = m_pLabel->sizePolicy();
  policy.setHorizontalStretch(1);
  m_pLabel->setSizePolicy(policy);
  policy.setHorizontalStretch(2);
  m_pWidget->setSizePolicy(policy);

  m_pLayout->addWidget(m_pLabel);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL(stateChanged(int)), this, SLOT(on_StateChanged_triggered(int)));
}

void ezPropertyEditorCheckboxWidget::InternalSetValue(const ezVariant& value)
{
  ezQtBlockSignals b(m_pWidget);
  m_pWidget->setChecked(value.ConvertTo<bool>() ? Qt::Checked : Qt::Unchecked);
}

void ezPropertyEditorCheckboxWidget::mousePressEvent(QMouseEvent* ev)
{
  QWidget::mousePressEvent(ev);

  m_pWidget->toggle();
}

void ezPropertyEditorCheckboxWidget::on_StateChanged_triggered(int state)
{
  BroadcastValueChanged((state == Qt::Checked) ? true : false);
}

ezPropertyEditorDoubleSpinboxWidget::ezPropertyEditorDoubleSpinboxWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent) : ezPropertyEditorBaseWidget(path, szName, pParent)
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pLabel = new QLabel(this);
  m_pLabel->setText(QString::fromUtf8(szName));
  m_pLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

  m_pWidget = new QDoubleSpinBox(this);
  m_pWidget->setMinimum(-ezMath::BasicType<double>::GetInfinity());
  m_pWidget->setMaximum( ezMath::BasicType<double>::GetInfinity());
  m_pWidget->setSingleStep(1.0);
  m_pWidget->setAccelerated(true);
  m_pWidget->setDecimals(8);

  QSizePolicy policy = m_pLabel->sizePolicy();
  policy.setHorizontalStretch(1);
  m_pLabel->setSizePolicy(policy);
  policy.setHorizontalStretch(2);
  m_pWidget->setSizePolicy(policy);

  m_pLayout->addWidget(m_pLabel);
  m_pLayout->addWidget(m_pWidget);

  //connect(m_pWidget, SIGNAL(valueChanged(double)), this, SLOT(on_ValueChanged_triggered(double)));
  connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
}

void ezPropertyEditorDoubleSpinboxWidget::InternalSetValue(const ezVariant& value)
{
  ezQtBlockSignals b (m_pWidget);
  m_pWidget->setValue(value.ConvertTo<double>());
}

void ezPropertyEditorDoubleSpinboxWidget::on_ValueChanged_triggered(double value)
{
  BroadcastValueChanged(value);
}

void ezPropertyEditorDoubleSpinboxWidget::on_EditingFinished_triggered()
{
  BroadcastValueChanged(m_pWidget->value());
}

ezPropertyEditorLineEditWidget::ezPropertyEditorLineEditWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent) : ezPropertyEditorBaseWidget(path, szName, pParent)
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pLabel = new QLabel(this);
  m_pLabel->setText(QString::fromUtf8(szName));
  m_pLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

  m_pWidget = new QLineEdit(this);

  QSizePolicy policy = m_pLabel->sizePolicy();
  policy.setHorizontalStretch(1);
  m_pLabel->setSizePolicy(policy);
  policy.setHorizontalStretch(2);
  m_pWidget->setSizePolicy(policy);

  m_pLayout->addWidget(m_pLabel);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered()));
  //connect(m_pWidget, SIGNAL(textChanged(const QString&)), this, SLOT(on_TextChanged_triggered(const QString&)));
}

void ezPropertyEditorLineEditWidget::InternalSetValue(const ezVariant& value)
{
  ezQtBlockSignals b (m_pWidget);
  m_pWidget->setText(QString::fromUtf8(value.ConvertTo<ezString>().GetData()));
}

void ezPropertyEditorLineEditWidget::on_TextChanged_triggered(const QString& value)
{
  BroadcastValueChanged(value.toUtf8().data());
}

void ezPropertyEditorLineEditWidget::on_TextFinished_triggered()
{
  BroadcastValueChanged(m_pWidget->text().toUtf8().data());
}

