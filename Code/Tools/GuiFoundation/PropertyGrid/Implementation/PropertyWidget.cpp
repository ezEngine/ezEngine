#include <GuiFoundation/PCH.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qlayout.h>
#include <QLineEdit>
#include <QLabel>
#include <QKeyEvent>
#include <QSpinBox>
#include <QPushButton>
#include <QColor>
#include <QComboBox>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QMenu>
#include <QWidgetAction>
#include <QToolButton>
#include <QFileDialog>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Strings/TranslationLookup.h>

/// *** CHECKBOX ***

ezQtPropertyEditorCheckboxWidget::ezQtPropertyEditorCheckboxWidget()
  : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new QCheckBox(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);

  EZ_VERIFY(connect(m_pWidget, SIGNAL(stateChanged(int)), this, SLOT(on_StateChanged_triggered(int))) != nullptr, "signal/slot connection failed");
}

void ezQtPropertyEditorCheckboxWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    m_pWidget->setTristate(false);
    m_pWidget->setChecked(value.ConvertTo<bool>() ? Qt::Checked : Qt::Unchecked);
  }
  else
  {
    m_pWidget->setTristate(true);
    m_pWidget->setCheckState(Qt::CheckState::PartiallyChecked);
  }
}

void ezQtPropertyEditorCheckboxWidget::mousePressEvent(QMouseEvent* ev)
{
  QWidget::mousePressEvent(ev);

  m_pWidget->toggle();
}

void ezQtPropertyEditorCheckboxWidget::on_StateChanged_triggered(int state)
{
  if (state == Qt::PartiallyChecked)
  {
    QtScopedBlockSignals b(m_pWidget);

    m_pWidget->setCheckState(Qt::Checked);
    m_pWidget->setTristate(false);
  }

  BroadcastValueChanged((state != Qt::Unchecked) ? true : false);
}


/// *** DOUBLE SPINBOX ***

ezQtPropertyEditorDoubleSpinboxWidget::ezQtPropertyEditorDoubleSpinboxWidget(ezInt8 iNumComponents) : ezQtStandardPropertyWidget()
{
  m_iNumComponents = iNumComponents;
  m_bTemporaryCommand = false;

  m_pWidget[0] = nullptr;
  m_pWidget[1] = nullptr;
  m_pWidget[2] = nullptr;
  m_pWidget[3] = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  for (ezInt32 c = 0; c < m_iNumComponents; ++c)
  {
    m_pWidget[c] = new ezQtDoubleSpinBox(this);
    m_pWidget[c]->setMinimum(-ezMath::BasicType<double>::GetInfinity());
    m_pWidget[c]->setMaximum(ezMath::BasicType<double>::GetInfinity());
    m_pWidget[c]->setSingleStep(0.1f);
    m_pWidget[c]->setAccelerated(true);
    m_pWidget[c]->setDecimals(3);

    policy.setHorizontalStretch(2);
    m_pWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void ezQtPropertyEditorDoubleSpinboxWidget::OnInit()
{
  const ezClampValueAttribute* pClamp = m_pProp->GetAttributeByType<ezClampValueAttribute>();
  const ezDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<ezDefaultValueAttribute>();
  const ezSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<ezSuffixAttribute>();

  if (pClamp)
  {
    switch (m_iNumComponents)
    {
    case 1:
      {
        QtScopedBlockSignals bs(m_pWidget[0]);

        if (pClamp->GetMinValue().CanConvertTo<double>())
        {
          m_pWidget[0]->setMinimum(pClamp->GetMinValue().ConvertTo<double>());
        }
        if (pClamp->GetMaxValue().CanConvertTo<double>())
        {
          m_pWidget[0]->setMaximum(pClamp->GetMaxValue().ConvertTo<double>());
        }
        break;
      }
    case 2:
      {
        QtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pClamp->GetMinValue().CanConvertTo<ezVec2>())
        {
          ezVec2 value = pClamp->GetMinValue().ConvertTo<ezVec2>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
        }
        if (pClamp->GetMaxValue().CanConvertTo<ezVec2>())
        {
          ezVec2 value = pClamp->GetMaxValue().ConvertTo<ezVec2>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
        }
        break;
      }
    case 3:
      {
        QtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pClamp->GetMinValue().CanConvertTo<ezVec3>())
        {
          ezVec3 value = pClamp->GetMinValue().ConvertTo<ezVec3>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
        }
        if (pClamp->GetMaxValue().CanConvertTo<ezVec3>())
        {
          ezVec3 value = pClamp->GetMaxValue().ConvertTo<ezVec3>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
        }
        break;
      }
    case 4:
      {
        QtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pClamp->GetMinValue().CanConvertTo<ezVec4>())
        {
          ezVec4 value = pClamp->GetMinValue().ConvertTo<ezVec4>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
          m_pWidget[3]->setMinimum(value.w);
        }
        if (pClamp->GetMaxValue().CanConvertTo<ezVec4>())
        {
          ezVec4 value = pClamp->GetMaxValue().ConvertTo<ezVec4>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
          m_pWidget[3]->setMaximum(value.w);
        }
        break;
      }
    }
  }

  if (pDefault)
  {
    switch (m_iNumComponents)
    {
    case 1:
      {
        QtScopedBlockSignals bs(m_pWidget[0]);

        if (pDefault->GetValue().CanConvertTo<double>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<double>());
        }
        break;
      }
    case 2:
      {
        QtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pDefault->GetValue().CanConvertTo<ezVec2>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<ezVec2>().x);
          m_pWidget[1]->setDefaultValue(pDefault->GetValue().ConvertTo<ezVec2>().y);
        }
        break;
      }
    case 3:
      {
        QtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pDefault->GetValue().CanConvertTo<ezVec3>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<ezVec3>().x);
          m_pWidget[1]->setDefaultValue(pDefault->GetValue().ConvertTo<ezVec3>().y);
          m_pWidget[2]->setDefaultValue(pDefault->GetValue().ConvertTo<ezVec3>().z);
        }
        break;
      }
    case 4:
      {
        QtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pDefault->GetValue().CanConvertTo<ezVec4>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<ezVec4>().x);
          m_pWidget[1]->setDefaultValue(pDefault->GetValue().ConvertTo<ezVec4>().y);
          m_pWidget[2]->setDefaultValue(pDefault->GetValue().ConvertTo<ezVec4>().z);
          m_pWidget[3]->setDefaultValue(pDefault->GetValue().ConvertTo<ezVec4>().w);
        }
        break;
      }
    }
  }

  if (pSuffix)
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setDisplaySuffix(pSuffix->GetSuffix());
    }
  }

  const ezMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<ezMinValueTextAttribute>();
  if (pMinValueText)
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setSpecialValueText(pMinValueText->GetText());
    }
  }
}

void ezQtPropertyEditorDoubleSpinboxWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b0(m_pWidget[0]);
  QtScopedBlockSignals b1(m_pWidget[1]);
  QtScopedBlockSignals b2(m_pWidget[2]);
  QtScopedBlockSignals b3(m_pWidget[3]);

  if (value.IsValid())
  {
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
  else
  {
    switch (m_iNumComponents)
    {
    case 1:
      m_pWidget[0]->setValueInvalid();
      break;
    case 2:
      m_pWidget[0]->setValueInvalid();
      m_pWidget[1]->setValueInvalid();
      break;
    case 3:
      m_pWidget[0]->setValueInvalid();
      m_pWidget[1]->setValueInvalid();
      m_pWidget[2]->setValueInvalid();
      break;
    case 4:
      m_pWidget[0]->setValueInvalid();
      m_pWidget[1]->setValueInvalid();
      m_pWidget[2]->setValueInvalid();
      m_pWidget[3]->setValueInvalid();
      break;
    }
  }

}

void ezQtPropertyEditorDoubleSpinboxWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void ezQtPropertyEditorDoubleSpinboxWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

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


/// *** TIME SPINBOX ***

ezQtPropertyEditorTimeWidget::ezQtPropertyEditorTimeWidget() : ezQtStandardPropertyWidget()
{
  m_bTemporaryCommand = false;

  m_pWidget = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  {
    m_pWidget = new ezQtDoubleSpinBox(this);
    m_pWidget->setDisplaySuffix(" (sec)");
    m_pWidget->setMinimum(-ezMath::BasicType<double>::GetInfinity());
    m_pWidget->setMaximum(ezMath::BasicType<double>::GetInfinity());
    m_pWidget->setSingleStep(0.1f);
    m_pWidget->setAccelerated(true);
    m_pWidget->setDecimals(3);

    policy.setHorizontalStretch(2);
    m_pWidget->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget);

    connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void ezQtPropertyEditorTimeWidget::OnInit()
{
  const ezClampValueAttribute* pClamp = m_pProp->GetAttributeByType<ezClampValueAttribute>();
  if (pClamp)
  {
    QtScopedBlockSignals bs(m_pWidget);

    if (pClamp->GetMinValue().CanConvertTo<ezTime>())
    {
      m_pWidget->setMinimum(pClamp->GetMinValue().ConvertTo<ezTime>().GetSeconds());
    }
    if (pClamp->GetMaxValue().CanConvertTo<ezTime>())
    {
      m_pWidget->setMaximum(pClamp->GetMaxValue().ConvertTo<ezTime>().GetSeconds());
    }
  }

  const ezDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<ezDefaultValueAttribute>();
  if (pDefault)
  {
    QtScopedBlockSignals bs(m_pWidget);

    if (pDefault->GetValue().CanConvertTo<ezTime>())
    {
      m_pWidget->setDefaultValue(pDefault->GetValue().ConvertTo<ezTime>().GetSeconds());
    }
  }
}

void ezQtPropertyEditorTimeWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b0(m_pWidget);

  if (value.IsValid())
  {
    m_pWidget->setValue(value.ConvertTo<ezTime>().GetSeconds());
  }
  else
  {
    m_pWidget->setValueInvalid();
  }
}

void ezQtPropertyEditorTimeWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void ezQtPropertyEditorTimeWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  BroadcastValueChanged(ezTime::Seconds(m_pWidget->value()));
}


/// *** ANGLE SPINBOX ***

ezQtPropertyEditorAngleWidget::ezQtPropertyEditorAngleWidget() : ezQtStandardPropertyWidget()
{
  m_bTemporaryCommand = false;

  m_pWidget = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  {
    m_pWidget = new ezQtDoubleSpinBox(this);
    m_pWidget->setDisplaySuffix(ezStringUtf8(L"\u00B0").GetData());
    m_pWidget->setMinimum(-ezMath::BasicType<double>::GetInfinity());
    m_pWidget->setMaximum(ezMath::BasicType<double>::GetInfinity());
    m_pWidget->setSingleStep(0.1f);
    m_pWidget->setAccelerated(true);
    m_pWidget->setDecimals(2);

    policy.setHorizontalStretch(2);
    m_pWidget->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget);

    connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void ezQtPropertyEditorAngleWidget::OnInit()
{
  const ezClampValueAttribute* pClamp = m_pProp->GetAttributeByType<ezClampValueAttribute>();
  if (pClamp)
  {
    QtScopedBlockSignals bs(m_pWidget);

    if (pClamp->GetMinValue().CanConvertTo<ezAngle>())
    {
      m_pWidget->setMinimum(pClamp->GetMinValue().ConvertTo<ezAngle>().GetDegree());
    }
    if (pClamp->GetMaxValue().CanConvertTo<ezAngle>())
    {
      m_pWidget->setMaximum(pClamp->GetMaxValue().ConvertTo<ezAngle>().GetDegree());
    }
  }

  const ezDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<ezDefaultValueAttribute>();
  if (pDefault)
  {
    QtScopedBlockSignals bs(m_pWidget);

    if (pDefault->GetValue().CanConvertTo<ezAngle>())
    {
      m_pWidget->setDefaultValue(pDefault->GetValue().ConvertTo<ezAngle>().GetDegree());
    }
  }

  const ezSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<ezSuffixAttribute>();
  if (pSuffix)
  {
    m_pWidget->setDisplaySuffix(pSuffix->GetSuffix());
  }

  const ezMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<ezMinValueTextAttribute>();
  if (pMinValueText)
  {
    m_pWidget->setSpecialValueText(pMinValueText->GetText());
  }
}

void ezQtPropertyEditorAngleWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b0(m_pWidget);

  if (value.IsValid())
  {
    m_pWidget->setValue(value.ConvertTo<ezAngle>().GetDegree());
  }
  else
  {
    m_pWidget->setValueInvalid();
  }
}

void ezQtPropertyEditorAngleWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void ezQtPropertyEditorAngleWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  BroadcastValueChanged(ezAngle::Degree(m_pWidget->value()));
}

/// *** INT SPINBOX ***


ezQtPropertyEditorIntSpinboxWidget::ezQtPropertyEditorIntSpinboxWidget(ezInt32 iMinValue, ezInt32 iMaxValue) : ezQtStandardPropertyWidget()
{
  m_bTemporaryCommand = false;
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new ezQtDoubleSpinBox(this, true);
  m_pWidget->setMinimum(iMinValue);
  m_pWidget->setMaximum(iMaxValue);
  m_pWidget->setSingleStep(1);
  m_pWidget->setAccelerated(true);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
  connect(m_pWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
}

void ezQtPropertyEditorIntSpinboxWidget::OnInit()
{
  const ezClampValueAttribute* pClamp = m_pProp->GetAttributeByType<ezClampValueAttribute>();
  if (pClamp)
  {
    QtScopedBlockSignals bs(m_pWidget);

    if (pClamp->GetMinValue().CanConvertTo<ezInt32>())
    {
      m_pWidget->setMinimum(pClamp->GetMinValue().ConvertTo<ezInt32>());
    }
    if (pClamp->GetMaxValue().CanConvertTo<ezInt32>())
    {
      m_pWidget->setMaximum(pClamp->GetMaxValue().ConvertTo<ezInt32>());
    }
  }

  const ezDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<ezDefaultValueAttribute>();
  if (pDefault)
  {
    QtScopedBlockSignals bs(m_pWidget);

    if (pDefault->GetValue().CanConvertTo<ezInt32>())
    {
      m_pWidget->setDefaultValue(pDefault->GetValue().ConvertTo<ezInt32>());
    }
  }

  const ezSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<ezSuffixAttribute>();
  if (pSuffix)
  {
    m_pWidget->setDisplaySuffix(pSuffix->GetSuffix());
  }

  const ezMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<ezMinValueTextAttribute>();
  if (pMinValueText)
  {
    m_pWidget->setSpecialValueText(pMinValueText->GetText());
  }
}

void ezQtPropertyEditorIntSpinboxWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b(m_pWidget);
  m_pWidget->setValue(value.ConvertTo<ezInt32>());
}

void ezQtPropertyEditorIntSpinboxWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  BroadcastValueChanged((ezInt32)m_pWidget->value());
}

void ezQtPropertyEditorIntSpinboxWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}


/// *** QUATERNION ***

ezQtPropertyEditorQuaternionWidget::ezQtPropertyEditorQuaternionWidget() : ezQtStandardPropertyWidget()
{
  m_bTemporaryCommand = false;

  m_pWidget[0] = nullptr;
  m_pWidget[1] = nullptr;
  m_pWidget[2] = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  for (ezInt32 c = 0; c < 3; ++c)
  {
    m_pWidget[c] = new ezQtDoubleSpinBox(this);
    m_pWidget[c]->setMinimum(-ezMath::BasicType<double>::GetInfinity());
    m_pWidget[c]->setMaximum(ezMath::BasicType<double>::GetInfinity());
    m_pWidget[c]->setSingleStep(1.0);
    m_pWidget[c]->setAccelerated(true);
    m_pWidget[c]->setDecimals(3);
    m_pWidget[c]->setDisplaySuffix("\xC2\xB0");

    policy.setHorizontalStretch(2);
    m_pWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void ezQtPropertyEditorQuaternionWidget::OnInit()
{
}

void ezQtPropertyEditorQuaternionWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b0(m_pWidget[0]);
  QtScopedBlockSignals b1(m_pWidget[1]);
  QtScopedBlockSignals b2(m_pWidget[2]);

  if (value.IsValid())
  {
    const ezQuat qRot = value.ConvertTo<ezQuat>();
    ezAngle x, y, z;
    qRot.GetAsEulerAngles(x, y, z);

    m_pWidget[0]->setValue(x.GetDegree());
    m_pWidget[1]->setValue(y.GetDegree());
    m_pWidget[2]->setValue(z.GetDegree());
  }
  else
  {
    m_pWidget[0]->setValueInvalid();
    m_pWidget[1]->setValueInvalid();
    m_pWidget[2]->setValueInvalid();
  }
}

void ezQtPropertyEditorQuaternionWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void ezQtPropertyEditorQuaternionWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  ezAngle x = ezAngle::Degree(m_pWidget[0]->value());
  ezAngle y = ezAngle::Degree(m_pWidget[1]->value());
  ezAngle z = ezAngle::Degree(m_pWidget[2]->value());

  ezQuat qRot;
  qRot.SetFromEulerAngles(x, y, z);

  BroadcastValueChanged(qRot);
}

/// *** LINEEDIT ***

ezQtPropertyEditorLineEditWidget::ezQtPropertyEditorLineEditWidget() : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new QLineEdit(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  setFocusProxy(m_pWidget);

  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered()));
}

void ezQtPropertyEditorLineEditWidget::OnInit()
{
  if (m_pProp->GetAttributeByType<ezReadOnlyAttribute>() != nullptr || m_pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
  {
    setEnabled(true);

    QtScopedBlockSignals bs(m_pWidget);

    m_pWidget->setReadOnly(true);
    auto palette = m_pWidget->palette();
    palette.setColor(QPalette::Base, QColor(0, 0, 0, 0));
    m_pWidget->setPalette(palette);
  }
}

void ezQtPropertyEditorLineEditWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b(m_pWidget);

  if (!value.IsValid())
  {
    m_pWidget->setPlaceholderText(QStringLiteral("<Multiple Values>"));
  }
  else
  {
    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(QString::fromUtf8(value.ConvertTo<ezString>().GetData()));
  }
}

void ezQtPropertyEditorLineEditWidget::on_TextChanged_triggered(const QString& value)
{
  BroadcastValueChanged(value.toUtf8().data());
}

void ezQtPropertyEditorLineEditWidget::on_TextFinished_triggered()
{
  BroadcastValueChanged(m_pWidget->text().toUtf8().data());
}


/// *** COLOR ***

ezQtColorButtonWidget::ezQtColorButtonWidget(QWidget* parent) : QFrame(parent)
{
  setAutoFillBackground(true);
}

void ezQtColorButtonWidget::SetColor(const ezVariant& color)
{
  if (color.IsValid())
  {
    ezColor col0 = color.ConvertTo<ezColor>();
    col0.NormalizeToLdrRange();

    const ezColorGammaUB col = col0;

    QColor qol;
    qol.setRgb(col.r, col.g, col.b, col.a);

    QPalette pal = palette();
    pal.setBrush(QPalette::Window, QBrush(qol, Qt::SolidPattern));

    setPalette(pal);
  }
  else
  {
    QPalette pal = palette();
    pal.setBrush(QPalette::Window, QBrush(pal.foreground().color(), Qt::DiagCrossPattern));

    setPalette(pal);
  }
}

void ezQtColorButtonWidget::mouseReleaseEvent(QMouseEvent* event)
{
  emit clicked();
}

ezQtPropertyEditorColorWidget::ezQtPropertyEditorColorWidget() : ezQtStandardPropertyWidget()
{
  m_bExposeAlpha = false;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new ezQtColorButtonWidget(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  m_pLayout->addWidget(m_pWidget);

  EZ_VERIFY(connect(m_pWidget, SIGNAL(clicked()), this, SLOT(on_Button_triggered())) != nullptr, "signal/slot connection failed");
}

void ezQtPropertyEditorColorWidget::OnInit()
{
  m_bExposeAlpha = (m_pProp->GetAttributeByType<ezExposeColorAlphaAttribute>() != nullptr);
}

void ezQtPropertyEditorColorWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b(m_pWidget);

  m_OriginalValue = GetOldValue();
  m_pWidget->SetColor(value);
}

void ezQtPropertyEditorColorWidget::on_Button_triggered()
{
  Broadcast(ezPropertyEvent::Type::BeginTemporary);

  bool bShowHDR = false;

  ezColor temp = ezColor::White;
  if (m_OriginalValue.IsValid())
  {
    bShowHDR = m_OriginalValue.IsA<ezColor>();

    temp = m_OriginalValue.ConvertTo<ezColor>();
  }

  ezQtUiServices::GetSingleton()->ShowColorDialog(temp, m_bExposeAlpha, bShowHDR, this, SLOT(on_CurrentColor_changed(const ezColor&)), SLOT(on_Color_accepted()), SLOT(on_Color_reset()));
}

void ezQtPropertyEditorColorWidget::on_CurrentColor_changed(const ezColor& color)
{
  ezVariant col;

  if (m_OriginalValue.IsA<ezColorGammaUB>())
  {
    // ezVariant does not down-cast to ezColorGammaUB automatically
    col = ezColorGammaUB(color);
  }
  else
  {
    col = color;
  }

  m_pWidget->SetColor(col);
  BroadcastValueChanged(col);
}

void ezQtPropertyEditorColorWidget::on_Color_reset()
{
  m_pWidget->SetColor(m_OriginalValue);
  Broadcast(ezPropertyEvent::Type::CancelTemporary);
}

void ezQtPropertyEditorColorWidget::on_Color_accepted()
{
  m_OriginalValue = GetOldValue();
  Broadcast(ezPropertyEvent::Type::EndTemporary);
}


/// *** ENUM COMBOBOX ***

ezQtPropertyEditorEnumWidget::ezQtPropertyEditorEnumWidget() : ezQtStandardPropertyWidget()
{

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new QComboBox(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(on_CurrentEnum_changed(int)));
}

void ezQtPropertyEditorEnumWidget::OnInit()
{
  const ezRTTI* enumType = m_pProp->GetSpecificType();

  QtScopedBlockSignals bs(m_pWidget);

  ezStringBuilder sTemp;
  const ezRTTI* pType = enumType;
  ezUInt32 uiCount = pType->GetProperties().GetCount();
  // Start at 1 to skip default value.
  for (ezUInt32 i = 1; i < uiCount; ++i)
  {
    auto pProp = pType->GetProperties()[i];

    if (pProp->GetCategory() != ezPropertyCategory::Constant)
      continue;

    const ezAbstractConstantProperty* pConstant = static_cast<const ezAbstractConstantProperty*>(pProp);

    m_pWidget->addItem(QString::fromUtf8(ezTranslate(pConstant->GetPropertyName())), pConstant->GetConstant().ConvertTo<ezInt64>());
  }
}

void ezQtPropertyEditorEnumWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    ezInt32 iIndex = m_pWidget->findData(value.ConvertTo<ezInt64>());
    EZ_ASSERT_DEV(iIndex != -1, "Enum widget is set to an invalid value!");
    m_pWidget->setCurrentIndex(iIndex);
  }
  else
  {
    m_pWidget->setCurrentIndex(-1);
  }
}

void ezQtPropertyEditorEnumWidget::on_CurrentEnum_changed(int iEnum)
{
  ezInt64 iValue = m_pWidget->itemData(iEnum).toLongLong();
  BroadcastValueChanged(iValue);
}


/// *** BITFLAGS COMBOBOX ***

ezQtPropertyEditorBitflagsWidget::ezQtPropertyEditorBitflagsWidget() : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new QPushButton(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pMenu = nullptr;
  m_pMenu = new QMenu(m_pWidget);
  m_pWidget->setMenu(m_pMenu);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pMenu, SIGNAL(aboutToShow()), this, SLOT(on_Menu_aboutToShow()));
  connect(m_pMenu, SIGNAL(aboutToHide()), this, SLOT(on_Menu_aboutToHide()));
}

ezQtPropertyEditorBitflagsWidget::~ezQtPropertyEditorBitflagsWidget()
{
  m_Constants.Clear();
  m_pWidget->setMenu(nullptr);

  delete m_pMenu;
  m_pMenu = nullptr;
}

void ezQtPropertyEditorBitflagsWidget::OnInit()
{
  const ezRTTI* enumType = m_pProp->GetSpecificType();

  const ezRTTI* pType = enumType;
  ezUInt32 uiCount = pType->GetProperties().GetCount();

  // Start at 1 to skip default value.
  for (ezUInt32 i = 1; i < uiCount; ++i)
  {
    auto pProp = pType->GetProperties()[i];

    if (pProp->GetCategory() != ezPropertyCategory::Constant)
      continue;

    const ezAbstractConstantProperty* pConstant = static_cast<const ezAbstractConstantProperty*>(pProp);

    QWidgetAction* pAction = new QWidgetAction(m_pMenu);
    QCheckBox* pCheckBox = new QCheckBox(QString::fromUtf8(pConstant->GetPropertyName()), m_pMenu);
    pCheckBox->setCheckable(true);
    pCheckBox->setCheckState(Qt::Unchecked);
    pAction->setDefaultWidget(pCheckBox);

    m_Constants[pConstant->GetConstant().ConvertTo<ezInt64>()] = pCheckBox;
    m_pMenu->addAction(pAction);
  }
}

void ezQtPropertyEditorBitflagsWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b(m_pWidget);
  m_iCurrentBitflags = value.ConvertTo<ezInt64>();

  QString sText;
  for (auto it = m_Constants.GetIterator(); it.IsValid(); ++it)
  {
    bool bChecked = (it.Key() & m_iCurrentBitflags) != 0;
    QString sName = it.Value()->text();
    if (bChecked)
    {
      sText += sName + "|";
    }
    it.Value()->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);
  }
  if (!sText.isEmpty())
    sText = sText.left(sText.size() - 1);

  m_pWidget->setText(sText);
}

void ezQtPropertyEditorBitflagsWidget::on_Menu_aboutToShow()
{
  m_pMenu->setMinimumWidth(m_pWidget->geometry().width());
}

void ezQtPropertyEditorBitflagsWidget::on_Menu_aboutToHide()
{
  ezInt64 iValue = 0;
  QString sText;
  for (auto it = m_Constants.GetIterator(); it.IsValid(); ++it)
  {
    bool bChecked = it.Value()->checkState() == Qt::Checked;
    QString sName = it.Value()->text();
    if (bChecked)
    {
      sText += sName + "|";
      iValue |= it.Key();
    }
  }
  if (!sText.isEmpty())
    sText = sText.left(sText.size() - 1);

  m_pWidget->setText(sText);

  if (m_iCurrentBitflags != iValue)
  {
    m_iCurrentBitflags = iValue;
    BroadcastValueChanged(m_iCurrentBitflags);
  }
}

