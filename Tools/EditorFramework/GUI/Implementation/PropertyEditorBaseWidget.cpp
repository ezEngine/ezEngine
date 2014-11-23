#include <PCH.h>
#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qlayout.h>
#include <QLineEdit>
#include <QLabel>
#include <QKeyEvent>
#include <QSpinBox>
#include <QPushButton>
#include <QColor>
#include <EditorFramework/GUI/QtHelpers.h>
#include <EditorFramework/EditorGUI.moc.h>

/// *** BASE ***

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


/// *** CHECKBOX ***

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

  EZ_VERIFY(connect(m_pWidget, SIGNAL(stateChanged(int)), this, SLOT(on_StateChanged_triggered(int))) != nullptr, "signal/slot connection failed");
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


/// *** DOUBLE SPINBOX ***

ezPropertyEditorDoubleSpinboxWidget::ezPropertyEditorDoubleSpinboxWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent, ezInt8 iNumComponents) : ezPropertyEditorBaseWidget(path, szName, pParent)
{
  m_iNumComponents= iNumComponents;

  m_pWidget[0] = nullptr;
  m_pWidget[1] = nullptr;
  m_pWidget[2] = nullptr;
  m_pWidget[3] = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pLabel = new QLabel(this);
  m_pLabel->setText(QString::fromUtf8(szName));
  m_pLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

  QSizePolicy policy = m_pLabel->sizePolicy();
  policy.setHorizontalStretch(m_iNumComponents);
  m_pLabel->setSizePolicy(policy);
  m_pLayout->addWidget(m_pLabel);

  for (ezInt32 c = 0; c < m_iNumComponents; ++c)
  {
    m_pWidget[c] = new QDoubleSpinBox(this);
    m_pWidget[c]->setMinimum(-ezMath::BasicType<double>::GetInfinity());
    m_pWidget[c]->setMaximum( ezMath::BasicType<double>::GetInfinity());
    m_pWidget[c]->setSingleStep(1.0);
    m_pWidget[c]->setAccelerated(true);
    m_pWidget[c]->setDecimals(3);

    policy.setHorizontalStretch(2);
    m_pWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
  }
}

void ezPropertyEditorDoubleSpinboxWidget::InternalSetValue(const ezVariant& value)
{
  ezQtBlockSignals b0 (m_pWidget[0]);
  ezQtBlockSignals b1 (m_pWidget[1]);
  ezQtBlockSignals b2 (m_pWidget[2]);
  ezQtBlockSignals b3 (m_pWidget[3]);

  switch (m_iNumComponents)
  {
  case 1:
    m_pWidget[0]->setValue(value.ConvertTo<double>());
    break;
  case 2:
    m_pWidget[0]->setValue(value.ConvertTo<ezVec2>().x);
    m_pWidget[1]->setValue(value.ConvertTo<ezVec2>().y);
    break;
  case 3:
    m_pWidget[0]->setValue(value.ConvertTo<ezVec3>().x);
    m_pWidget[1]->setValue(value.ConvertTo<ezVec3>().y);
    m_pWidget[2]->setValue(value.ConvertTo<ezVec3>().z);
    break;
  case 4:
    m_pWidget[0]->setValue(value.ConvertTo<ezVec4>().x);
    m_pWidget[1]->setValue(value.ConvertTo<ezVec4>().y);
    m_pWidget[2]->setValue(value.ConvertTo<ezVec4>().z);
    m_pWidget[3]->setValue(value.ConvertTo<ezVec4>().w);
    break;
  }
}

void ezPropertyEditorDoubleSpinboxWidget::on_EditingFinished_triggered()
{
  switch (m_iNumComponents)
  {
  case 1:
    BroadcastValueChanged(m_pWidget[0]->value());
    break;
  case 2:
    BroadcastValueChanged(ezVec2(m_pWidget[0]->value(), m_pWidget[1]->value()));
    break;
  case 3:
    BroadcastValueChanged(ezVec3(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value()));
    break;
  case 4:
    BroadcastValueChanged(ezVec4(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value(), m_pWidget[3]->value()));
    break;
  }
}

/// *** INT SPINBOX ***

ezPropertyEditorIntSpinboxWidget::ezPropertyEditorIntSpinboxWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent, ezInt32 iMinValue, ezInt32 iMaxValue) : ezPropertyEditorBaseWidget(path, szName, pParent)
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pLabel = new QLabel(this);
  m_pLabel->setText(QString::fromUtf8(szName));
  m_pLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

  m_pWidget = new QSpinBox(this);
  m_pWidget->setMinimum(iMinValue);
  m_pWidget->setMaximum(iMaxValue);
  m_pWidget->setSingleStep(1);
  m_pWidget->setAccelerated(true);

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

void ezPropertyEditorIntSpinboxWidget::InternalSetValue(const ezVariant& value)
{
  ezQtBlockSignals b (m_pWidget);
  m_pWidget->setValue(value.ConvertTo<ezInt32>());
}

void ezPropertyEditorIntSpinboxWidget::on_ValueChanged_triggered(int value)
{
  BroadcastValueChanged(value);
}

void ezPropertyEditorIntSpinboxWidget::on_EditingFinished_triggered()
{
  BroadcastValueChanged(m_pWidget->value());
}


/// *** LINEEDIT ***

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

/// *** COLOR ***

ezPropertyEditorColorWidget::ezPropertyEditorColorWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent) : ezPropertyEditorBaseWidget(path, szName, pParent)
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pLabel = new QLabel(this);
  m_pLabel->setText(QString::fromUtf8(szName));
  m_pLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

  m_pWidget = new QPushButton(this);

  QSizePolicy policy = m_pLabel->sizePolicy();
  policy.setHorizontalStretch(1);
  m_pLabel->setSizePolicy(policy);
  policy.setHorizontalStretch(2);
  m_pWidget->setSizePolicy(policy);

  m_pLayout->addWidget(m_pLabel);
  m_pLayout->addWidget(m_pWidget);

  EZ_VERIFY(connect(m_pWidget, SIGNAL(clicked()), this, SLOT(on_Button_triggered())) != nullptr, "signal/slot connection failed");
}

void ezPropertyEditorColorWidget::InternalSetValue(const ezVariant& value)
{
  ezQtBlockSignals b (m_pWidget);
  m_CurrentColor = value.ConvertTo<ezColor>();

  QColor col;
  col.setRgbF(m_CurrentColor.r, m_CurrentColor.g, m_CurrentColor.b, m_CurrentColor.a);
  
  const QString COLOR_STYLE("QPushButton { background-color : %1 }");
  m_pWidget->setStyleSheet(COLOR_STYLE.arg(col.name()));
}

static ezColor g_LastColor;

void ezPropertyEditorColorWidget::on_Button_triggered()
{
  g_LastColor = m_CurrentColor;
  
  QColor col;
  col.setRgbF(m_CurrentColor.r, m_CurrentColor.g, m_CurrentColor.b, m_CurrentColor.a);

  ezEditorGUI::GetInstance()->ShowColorDialog(m_CurrentColor, true, this, SLOT(on_CurrentColor_changed(const QColor&)), SLOT(on_Color_accepted()), SLOT(on_Color_reset()));
}

void ezPropertyEditorColorWidget::on_CurrentColor_changed(const QColor& color)
{
  const ezColor NewCol(color.redF(), color.greenF(), color.blueF(), color.alphaF());

  const QString COLOR_STYLE("QPushButton { background-color : %1 }");
  m_pWidget->setStyleSheet(COLOR_STYLE.arg(color.name()));

  m_CurrentColor = NewCol;

  BroadcastValueChanged(NewCol);
}

void ezPropertyEditorColorWidget::on_Color_reset()
{
  m_CurrentColor = g_LastColor;
  
  QColor color;
  color.setRgbF(m_CurrentColor.r, m_CurrentColor.g, m_CurrentColor.b, m_CurrentColor.a);

  const QString COLOR_STYLE("QPushButton { background-color : %1 }");
  m_pWidget->setStyleSheet(COLOR_STYLE.arg(color.name()));

  BroadcastValueChanged(g_LastColor);
}

void ezPropertyEditorColorWidget::on_Color_accepted()
{
}
