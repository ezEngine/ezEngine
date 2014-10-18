#include <PCH.h>
#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qlayout.h>
#include <QLineEdit>
#include <QLabel>
#include <EditorFramework/GUI/QtHelpers.h>

ezPropertyEditorBaseWidget::ezPropertyEditorBaseWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent) : QWidget(pParent)
{
  m_szDisplayName = szName;
  m_PropertyPath = path;
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

void ezPropertyEditorCheckboxWidget::SetValue(const ezVariant& value)
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
  EventData ed;
  ed.m_pPropertyPath = &m_PropertyPath;
  ed.m_Value = (state == Qt::Checked) ? true : false;

  m_ValueChanged.Broadcast(ed);
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

  QSizePolicy policy = m_pLabel->sizePolicy();
  policy.setHorizontalStretch(1);
  m_pLabel->setSizePolicy(policy);
  policy.setHorizontalStretch(2);
  m_pWidget->setSizePolicy(policy);

  m_pLayout->addWidget(m_pLabel);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL(valueChanged(double)), this, SLOT(on_ValueChanged_triggered(double)));
}

void ezPropertyEditorDoubleSpinboxWidget::SetValue(const ezVariant& value)
{
  ezQtBlockSignals b (m_pWidget);
  m_pWidget->setValue(value.ConvertTo<double>());
}

void ezPropertyEditorDoubleSpinboxWidget::on_ValueChanged_triggered(double value)
{
  EventData ed;
  ed.m_pPropertyPath = &m_PropertyPath;
  ed.m_Value = value;

  m_ValueChanged.Broadcast(ed);
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

  connect(m_pWidget, SIGNAL(textChanged(const QString& value)), this, SLOT(on_TextChanged_triggered(const QString& value)));
}

void ezPropertyEditorLineEditWidget::SetValue(const ezVariant& value)
{
  ezQtBlockSignals b (m_pWidget);
  m_pWidget->setText(QString::fromUtf8(value.ConvertTo<ezString>().GetData()));
}

void ezPropertyEditorLineEditWidget::on_TextChanged_triggered(const QString& value)
{
  EventData ed;
  ed.m_pPropertyPath = &m_PropertyPath;
  ed.m_Value = value.toUtf8().data();

  m_ValueChanged.Broadcast(ed);
}

