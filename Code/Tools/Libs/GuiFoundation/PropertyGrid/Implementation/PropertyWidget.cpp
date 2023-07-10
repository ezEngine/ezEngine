#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Dialogs/CurveEditDlg.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <QComboBox>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QWidgetAction>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <qcheckbox.h>
#include <qlayout.h>


/// *** CHECKBOX ***

ezQtPropertyEditorCheckboxWidget::ezQtPropertyEditorCheckboxWidget()
  : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QCheckBox(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);

  EZ_VERIFY(connect(m_pWidget, SIGNAL(stateChanged(int)), this, SLOT(on_StateChanged_triggered(int))) != nullptr, "signal/slot connection failed");
}

void ezQtPropertyEditorCheckboxWidget::InternalSetValue(const ezVariant& value)
{
  ezQtScopedBlockSignals b(m_pWidget);

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

void ezQtPropertyEditorCheckboxWidget::mousePressEvent(QMouseEvent* pEv)
{
  QWidget::mousePressEvent(pEv);

  m_pWidget->toggle();
}

void ezQtPropertyEditorCheckboxWidget::on_StateChanged_triggered(int state)
{
  if (state == Qt::PartiallyChecked)
  {
    ezQtScopedBlockSignals b(m_pWidget);

    m_pWidget->setCheckState(Qt::Checked);
    m_pWidget->setTristate(false);
  }

  BroadcastValueChanged((state != Qt::Unchecked) ? true : false);
}


/// *** DOUBLE SPINBOX ***

ezQtPropertyEditorDoubleSpinboxWidget::ezQtPropertyEditorDoubleSpinboxWidget(ezInt8 iNumComponents)
  : ezQtStandardPropertyWidget()
{
  EZ_ASSERT_DEBUG(iNumComponents <= 4, "Only up to 4 components are supported");

  m_iNumComponents = iNumComponents;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  for (ezInt32 c = 0; c < m_iNumComponents; ++c)
  {
    m_pWidget[c] = new ezQtDoubleSpinBox(this);
    m_pWidget[c]->installEventFilter(this);
    m_pWidget[c]->setMinimum(-ezMath::Infinity<double>());
    m_pWidget[c]->setMaximum(ezMath::Infinity<double>());
    m_pWidget[c]->setSingleStep(0.1f);
    m_pWidget[c]->setAccelerated(true);

    policy.setHorizontalStretch(2);
    m_pWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void ezQtPropertyEditorDoubleSpinboxWidget::OnInit()
{
  auto pNoTemporaryTransactions = m_pProp->GetAttributeByType<ezNoTemporaryTransactionsAttribute>();
  m_bUseTemporaryTransaction = (pNoTemporaryTransactions == nullptr);

  if (const ezClampValueAttribute* pClamp = m_pProp->GetAttributeByType<ezClampValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        ezQtScopedBlockSignals bs(m_pWidget[0]);
        m_pWidget[0]->setMinimum(pClamp->GetMinValue());
        m_pWidget[0]->setMaximum(pClamp->GetMaxValue());
        break;
      }
      case 2:
      {
        ezQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

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
        ezQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

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
        ezQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

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

  if (const ezDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<ezDefaultValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        ezQtScopedBlockSignals bs(m_pWidget[0]);

        if (pDefault->GetValue().CanConvertTo<double>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<double>());
        }
        break;
      }
      case 2:
      {
        ezQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pDefault->GetValue().CanConvertTo<ezVec2>())
        {
          ezVec2 value = pDefault->GetValue().ConvertTo<ezVec2>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
        }
        break;
      }
      case 3:
      {
        ezQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pDefault->GetValue().CanConvertTo<ezVec3>())
        {
          ezVec3 value = pDefault->GetValue().ConvertTo<ezVec3>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
        }
        break;
      }
      case 4:
      {
        ezQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pDefault->GetValue().CanConvertTo<ezVec4>())
        {
          ezVec4 value = pDefault->GetValue().ConvertTo<ezVec4>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
          m_pWidget[3]->setDefaultValue(value.w);
        }
        break;
      }
    }
  }

  if (const ezSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<ezSuffixAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setDisplaySuffix(pSuffix->GetSuffix());
    }
  }

  if (const ezMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<ezMinValueTextAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setSpecialValueText(pMinValueText->GetText());
    }
  }
}

void ezQtPropertyEditorDoubleSpinboxWidget::InternalSetValue(const ezVariant& value)
{
  ezQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

  m_OriginalType = value.GetType();

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
  if (m_bUseTemporaryTransaction && m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void ezQtPropertyEditorDoubleSpinboxWidget::SlotValueChanged()
{
  if (m_bUseTemporaryTransaction && !m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  switch (m_iNumComponents)
  {
    case 1:
      BroadcastValueChanged(ezVariant(m_pWidget[0]->value()).ConvertTo(m_OriginalType));
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

ezQtPropertyEditorTimeWidget::ezQtPropertyEditorTimeWidget()
  : ezQtStandardPropertyWidget()
{
  m_bTemporaryCommand = false;

  m_pWidget = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  {
    m_pWidget = new ezQtDoubleSpinBox(this);
    m_pWidget->installEventFilter(this);
    m_pWidget->setDisplaySuffix(" sec");
    m_pWidget->setMinimum(-ezMath::Infinity<double>());
    m_pWidget->setMaximum(ezMath::Infinity<double>());
    m_pWidget->setSingleStep(0.1f);
    m_pWidget->setAccelerated(true);

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
    ezQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setMinimum(pClamp->GetMinValue());
    m_pWidget->setMaximum(pClamp->GetMaxValue());
  }

  const ezDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<ezDefaultValueAttribute>();
  if (pDefault)
  {
    ezQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setDefaultValue(pDefault->GetValue());
  }
}

void ezQtPropertyEditorTimeWidget::InternalSetValue(const ezVariant& value)
{
  ezQtScopedBlockSignals b0(m_pWidget);
  m_pWidget->setValue(value);
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

ezQtPropertyEditorAngleWidget::ezQtPropertyEditorAngleWidget()
  : ezQtStandardPropertyWidget()
{
  m_bTemporaryCommand = false;

  m_pWidget = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  {
    m_pWidget = new ezQtDoubleSpinBox(this);
    m_pWidget->installEventFilter(this);
    m_pWidget->setDisplaySuffix(ezStringUtf8(L"\u00B0").GetData());
    m_pWidget->setMinimum(-ezMath::Infinity<double>());
    m_pWidget->setMaximum(ezMath::Infinity<double>());
    m_pWidget->setSingleStep(0.1f);
    m_pWidget->setAccelerated(true);
    m_pWidget->setDecimals(1);

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
    ezQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setMinimum(pClamp->GetMinValue());
    m_pWidget->setMaximum(pClamp->GetMaxValue());
  }

  const ezDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<ezDefaultValueAttribute>();
  if (pDefault)
  {
    ezQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setDefaultValue(pDefault->GetValue());
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
  ezQtScopedBlockSignals b0(m_pWidget);
  m_pWidget->setValue(value);
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


ezQtPropertyEditorIntSpinboxWidget::ezQtPropertyEditorIntSpinboxWidget(ezInt8 iNumComponents, ezInt32 iMinValue, ezInt32 iMaxValue)
  : ezQtStandardPropertyWidget()
{
  m_iNumComponents = iNumComponents;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();
  policy.setHorizontalStretch(2);

  for (ezInt32 c = 0; c < m_iNumComponents; ++c)
  {
    m_pWidget[c] = new ezQtDoubleSpinBox(this, true);
    m_pWidget[c]->installEventFilter(this);
    m_pWidget[c]->setMinimum(iMinValue);
    m_pWidget[c]->setMaximum(iMaxValue);
    m_pWidget[c]->setSingleStep(1);
    m_pWidget[c]->setAccelerated(true);

    m_pWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}


ezQtPropertyEditorIntSpinboxWidget::~ezQtPropertyEditorIntSpinboxWidget() = default;

void ezQtPropertyEditorIntSpinboxWidget::OnInit()
{
  auto pNoTemporaryTransactions = m_pProp->GetAttributeByType<ezNoTemporaryTransactionsAttribute>();
  m_bUseTemporaryTransaction = (pNoTemporaryTransactions == nullptr);

  if (const ezClampValueAttribute* pClamp = m_pProp->GetAttributeByType<ezClampValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        const ezInt32 iMinValue = pClamp->GetMinValue().ConvertTo<ezInt32>();
        const ezInt32 iMaxValue = pClamp->GetMaxValue().ConvertTo<ezInt32>();

        ezQtScopedBlockSignals bs(m_pWidget[0]);
        m_pWidget[0]->setMinimum(pClamp->GetMinValue());
        m_pWidget[0]->setMaximum(pClamp->GetMaxValue());

        if (pClamp->GetMinValue().IsValid() && pClamp->GetMaxValue().IsValid() && (iMaxValue - iMinValue) < 256 && m_bUseTemporaryTransaction)
        {
          ezQtScopedBlockSignals bs2(m_pSlider);

          // we have to create the slider here, because in the constructor we don't know the real
          // min and max values from the ezClampValueAttribute (only the rough type ranges)
          m_pSlider = new QSlider(this);
          m_pSlider->installEventFilter(this);
          m_pSlider->setOrientation(Qt::Orientation::Horizontal);
          m_pSlider->setMinimum(iMinValue);
          m_pSlider->setMaximum(iMaxValue);

          m_pLayout->insertWidget(0, m_pSlider, 5); // make it take up most of the space
          connect(m_pSlider, SIGNAL(valueChanged(int)), this, SLOT(SlotSliderValueChanged(int)));
          connect(m_pSlider, SIGNAL(sliderReleased()), this, SLOT(on_EditingFinished_triggered()));
        }

        break;
      }
      case 2:
      {
        ezQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pClamp->GetMinValue().CanConvertTo<ezVec2I32>())
        {
          ezVec2I32 value = pClamp->GetMinValue().ConvertTo<ezVec2I32>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
        }
        if (pClamp->GetMaxValue().CanConvertTo<ezVec2I32>())
        {
          ezVec2I32 value = pClamp->GetMaxValue().ConvertTo<ezVec2I32>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
        }
        break;
      }
      case 3:
      {
        ezQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pClamp->GetMinValue().CanConvertTo<ezVec3I32>())
        {
          ezVec3I32 value = pClamp->GetMinValue().ConvertTo<ezVec3I32>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
        }
        if (pClamp->GetMaxValue().CanConvertTo<ezVec3I32>())
        {
          ezVec3I32 value = pClamp->GetMaxValue().ConvertTo<ezVec3I32>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
        }
        break;
      }
      case 4:
      {
        ezQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pClamp->GetMinValue().CanConvertTo<ezVec4I32>())
        {
          ezVec4I32 value = pClamp->GetMinValue().ConvertTo<ezVec4I32>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
          m_pWidget[3]->setMinimum(value.w);
        }
        if (pClamp->GetMaxValue().CanConvertTo<ezVec4I32>())
        {
          ezVec4I32 value = pClamp->GetMaxValue().ConvertTo<ezVec4I32>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
          m_pWidget[3]->setMaximum(value.w);
        }
        break;
      }
    }
  }

  if (const ezDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<ezDefaultValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        ezQtScopedBlockSignals bs(m_pWidget[0], m_pSlider);

        if (pDefault->GetValue().CanConvertTo<ezInt32>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<ezInt32>());

          if (m_pSlider)
          {
            m_pSlider->setValue(pDefault->GetValue().ConvertTo<ezInt32>());
          }
        }
        break;
      }
      case 2:
      {
        ezQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pDefault->GetValue().CanConvertTo<ezVec2I32>())
        {
          ezVec2I32 value = pDefault->GetValue().ConvertTo<ezVec2I32>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
        }
        break;
      }
      case 3:
      {
        ezQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pDefault->GetValue().CanConvertTo<ezVec3I32>())
        {
          ezVec3I32 value = pDefault->GetValue().ConvertTo<ezVec3I32>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
        }
        break;
      }
      case 4:
      {
        ezQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pDefault->GetValue().CanConvertTo<ezVec4I32>())
        {
          ezVec4I32 value = pDefault->GetValue().ConvertTo<ezVec4I32>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
          m_pWidget[3]->setDefaultValue(value.w);
        }
        break;
      }
    }
  }

  if (const ezSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<ezSuffixAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setDisplaySuffix(pSuffix->GetSuffix());
    }
  }

  if (const ezMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<ezMinValueTextAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setSpecialValueText(pMinValueText->GetText());
    }
  }
}

void ezQtPropertyEditorIntSpinboxWidget::InternalSetValue(const ezVariant& value)
{
  ezQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3], m_pSlider);

  m_OriginalType = value.GetType();

  switch (m_iNumComponents)
  {
    case 1:
      m_pWidget[0]->setValue(value.ConvertTo<ezInt32>());

      if (m_pSlider)
      {
        m_pSlider->setValue(value.ConvertTo<ezInt32>());
      }

      break;
    case 2:
      m_pWidget[0]->setValue(value.ConvertTo<ezVec2I32>().x);
      m_pWidget[1]->setValue(value.ConvertTo<ezVec2I32>().y);
      break;
    case 3:
      m_pWidget[0]->setValue(value.ConvertTo<ezVec3I32>().x);
      m_pWidget[1]->setValue(value.ConvertTo<ezVec3I32>().y);
      m_pWidget[2]->setValue(value.ConvertTo<ezVec3I32>().z);
      break;
    case 4:
      m_pWidget[0]->setValue(value.ConvertTo<ezVec4I32>().x);
      m_pWidget[1]->setValue(value.ConvertTo<ezVec4I32>().y);
      m_pWidget[2]->setValue(value.ConvertTo<ezVec4I32>().z);
      m_pWidget[3]->setValue(value.ConvertTo<ezVec4I32>().w);
      break;
  }
}

void ezQtPropertyEditorIntSpinboxWidget::SlotValueChanged()
{
  if (m_bUseTemporaryTransaction && !m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  ezVariant newValue;
  switch (m_iNumComponents)
  {
    case 1:
      newValue = m_pWidget[0]->value();

      if (m_pSlider)
      {
        ezQtScopedBlockSignals b0(m_pSlider);
        m_pSlider->setValue((ezInt32)m_pWidget[0]->value());
      }

      break;
    case 2:
      newValue = ezVec2I32(m_pWidget[0]->value(), m_pWidget[1]->value());
      break;
    case 3:
      newValue = ezVec3I32(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value());
      break;
    case 4:
      newValue = ezVec4I32(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value(), m_pWidget[3]->value());
      break;
  }

  BroadcastValueChanged(newValue.ConvertTo(m_OriginalType));
}

void ezQtPropertyEditorIntSpinboxWidget::SlotSliderValueChanged(int value)
{
  if (m_bUseTemporaryTransaction && !m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  {
    ezQtScopedBlockSignals b0(m_pWidget[0]);
    m_pWidget[0]->setValue(value);
  }

  BroadcastValueChanged(ezVariant(m_pSlider->value()).ConvertTo(m_OriginalType));
}

void ezQtPropertyEditorIntSpinboxWidget::on_EditingFinished_triggered()
{
  if (m_bUseTemporaryTransaction && m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}


/// *** QUATERNION ***

ezQtPropertyEditorQuaternionWidget::ezQtPropertyEditorQuaternionWidget()
  : ezQtStandardPropertyWidget()
{
  m_bTemporaryCommand = false;

  m_pWidget[0] = nullptr;
  m_pWidget[1] = nullptr;
  m_pWidget[2] = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  for (ezInt32 c = 0; c < 3; ++c)
  {
    m_pWidget[c] = new ezQtDoubleSpinBox(this);
    m_pWidget[c]->installEventFilter(this);
    m_pWidget[c]->setMinimum(-ezMath::Infinity<double>());
    m_pWidget[c]->setMaximum(ezMath::Infinity<double>());
    m_pWidget[c]->setSingleStep(1.0);
    m_pWidget[c]->setAccelerated(true);
    m_pWidget[c]->setDisplaySuffix("\xC2\xB0");

    policy.setHorizontalStretch(2);
    m_pWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void ezQtPropertyEditorQuaternionWidget::OnInit() {}

void ezQtPropertyEditorQuaternionWidget::InternalSetValue(const ezVariant& value)
{
  if (m_bTemporaryCommand)
    return;

  ezQtScopedBlockSignals b0(m_pWidget[0]);
  ezQtScopedBlockSignals b1(m_pWidget[1]);
  ezQtScopedBlockSignals b2(m_pWidget[2]);

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

ezQtPropertyEditorLineEditWidget::ezQtPropertyEditorLineEditWidget()
  : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QLineEdit(this);
  m_pWidget->installEventFilter(this);
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

    ezQtScopedBlockSignals bs(m_pWidget);

    m_pWidget->setReadOnly(true);
    QPalette palette = m_pWidget->palette();
    palette.setColor(QPalette::Base, QColor(0, 0, 0, 0));
    m_pWidget->setPalette(palette);
  }
}

void ezQtPropertyEditorLineEditWidget::InternalSetValue(const ezVariant& value)
{
  ezQtScopedBlockSignals b(m_pWidget);

  m_OriginalType = value.GetType();

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
  BroadcastValueChanged(ezVariant(value.toUtf8().data()).ConvertTo(m_OriginalType));
}

void ezQtPropertyEditorLineEditWidget::on_TextFinished_triggered()
{
  BroadcastValueChanged(ezVariant(m_pWidget->text().toUtf8().data()).ConvertTo(m_OriginalType));
}


/// *** COLOR ***

ezQtColorButtonWidget::ezQtColorButtonWidget(QWidget* pParent)
  : QFrame(pParent)
{
  setAutoFillBackground(true);
  setCursor(Qt::PointingHandCursor);
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

    m_Pal.setBrush(QPalette::Window, QBrush(qol, Qt::SolidPattern));
    setPalette(m_Pal);
  }
  else
  {
    setPalette(m_Pal);
  }
}

void ezQtColorButtonWidget::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set palette.
  setPalette(m_Pal);
  QFrame::showEvent(event);
}

void ezQtColorButtonWidget::mouseReleaseEvent(QMouseEvent* event)
{
  Q_EMIT clicked();
}

QSize ezQtColorButtonWidget::sizeHint() const
{
  return minimumSizeHint();
}

QSize ezQtColorButtonWidget::minimumSizeHint() const
{
  QFontMetrics fm(font());

  QStyleOptionFrame opt;
  initStyleOption(&opt);
  return style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(20, fm.height()), this);
}

ezQtPropertyEditorColorWidget::ezQtPropertyEditorColorWidget()
  : ezQtStandardPropertyWidget()
{
  m_bExposeAlpha = false;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
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
  ezQtScopedBlockSignals b(m_pWidget);

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

  ezQtUiServices::GetSingleton()->ShowColorDialog(
    temp, m_bExposeAlpha, bShowHDR, this, SLOT(on_CurrentColor_changed(const ezColor&)), SLOT(on_Color_accepted()), SLOT(on_Color_reset()));
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

ezQtPropertyEditorEnumWidget::ezQtPropertyEditorEnumWidget()
  : ezQtStandardPropertyWidget()
{

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QComboBox(this);
  m_pWidget->installEventFilter(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(on_CurrentEnum_changed(int)));
}

void ezQtPropertyEditorEnumWidget::OnInit()
{
  const ezRTTI* pType = m_pProp->GetSpecificType();

  ezQtScopedBlockSignals bs(m_pWidget);

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
  ezQtScopedBlockSignals b(m_pWidget);

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

ezQtPropertyEditorBitflagsWidget::ezQtPropertyEditorBitflagsWidget()
  : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
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
    QCheckBox* pCheckBox = new QCheckBox(QString::fromUtf8(ezTranslate(pConstant->GetPropertyName())), m_pMenu);
    pCheckBox->setCheckable(true);
    pCheckBox->setCheckState(Qt::Unchecked);
    pAction->setDefaultWidget(pCheckBox);

    m_Constants[pConstant->GetConstant().ConvertTo<ezInt64>()] = pCheckBox;
    m_pMenu->addAction(pAction);
  }
}

void ezQtPropertyEditorBitflagsWidget::InternalSetValue(const ezVariant& value)
{
  ezQtScopedBlockSignals b(m_pWidget);
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


/// *** CURVE1D ***

ezQtCurve1DButtonWidget::ezQtCurve1DButtonWidget(QWidget* pParent)
  : QLabel(pParent)
{
  setAutoFillBackground(true);
  setCursor(Qt::PointingHandCursor);
  setScaledContents(true);
}

void ezQtCurve1DButtonWidget::UpdatePreview(ezObjectAccessorBase* pObjectAccessor, const ezDocumentObject* pCurveObject, QColor color, double fLowerExtents, bool bLowerFixed, double fUpperExtents, bool bUpperFixed, double fDefaultValue, double fLowerRange, double fUpperRange)
{
  ezInt32 iNumPoints = 0;
  pObjectAccessor->GetCount(pCurveObject, "ControlPoints", iNumPoints).AssertSuccess();

  ezVariant v;
  ezHybridArray<ezVec2d, 32> points;
  points.Reserve(iNumPoints);

  double minX = fLowerExtents * 4800.0;
  double maxX = fUpperExtents * 4800.0;

  double minY = fLowerRange;
  double maxY = fUpperRange;

  for (ezInt32 i = 0; i < iNumPoints; ++i)
  {
    const ezDocumentObject* pPoint = pObjectAccessor->GetChildObject(pCurveObject, "ControlPoints", i);

    ezVec2d p;

    pObjectAccessor->GetValue(pPoint, "Tick", v).AssertSuccess();
    p.x = v.ConvertTo<double>();

    pObjectAccessor->GetValue(pPoint, "Value", v).AssertSuccess();
    p.y = v.ConvertTo<double>();

    points.PushBack(p);

    if (!bLowerFixed)
      minX = ezMath::Min(minX, p.x);

    if (!bUpperFixed)
      maxX = ezMath::Max(maxX, p.x);

    minY = ezMath::Min(minY, p.y);
    maxY = ezMath::Max(maxY, p.y);
  }

  const double pW = ezMath::Max(10, size().width());
  const double pH = ezMath::Clamp(size().height(), 5, 24);

  QPixmap pixmap((int)pW, (int)pH);
  pixmap.fill(palette().base().color());

  QPainter pt(&pixmap);
  pt.setPen(color);
  pt.setRenderHint(QPainter::RenderHint::Antialiasing);

  if (!points.IsEmpty())
  {
    points.Sort([](const ezVec2d& lhs, const ezVec2d& rhs) -> bool { return lhs.x < rhs.x; });

    const double normX = 1.0 / (maxX - minX);
    const double normY = 1.0 / (maxY - minY);

    QPainterPath path;

    {
      double startX = ezMath::Min(minX, points[0].x);
      double startY = points[0].y;

      startX = (startX - minX) * normX;
      startY = 1.0 - ((startY - minY) * normY);

      path.moveTo((int)(startX * pW), (int)(startY * pH));
    }

    for (ezUInt32 i = 0; i < points.GetCount(); ++i)
    {
      auto pt0 = points[i];
      pt0.x = (pt0.x - minX) * normX;
      pt0.y = 1.0 - ((pt0.y - minY) * normY);

      path.lineTo((int)(pt0.x * pW), (int)(pt0.y * pH));
    }

    {
      double endX = ezMath::Max(maxX, points.PeekBack().x);
      double endY = points.PeekBack().y;

      endX = (endX - minX) * normX;
      endY = 1.0 - ((endY - minY) * normY);

      path.lineTo((int)(endX * pW), (int)(endY * pH));
    }

    pt.drawPath(path);
  }
  else
  {
    const double normY = 1.0 / (maxY - minY);
    double valY = 1.0 - ((fDefaultValue - minY) * normY);

    pt.drawLine(0, (int)(valY * pH), (int)pW, (int)(valY * pH));
  }

  setPixmap(pixmap);
}

void ezQtCurve1DButtonWidget::mouseReleaseEvent(QMouseEvent* event)
{
  Q_EMIT clicked();
}

ezQtPropertyEditorCurve1DWidget::ezQtPropertyEditorCurve1DWidget()
  : ezQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pButton = new ezQtCurve1DButtonWidget(this);
  m_pButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  m_pLayout->addWidget(m_pButton);

  EZ_VERIFY(connect(m_pButton, SIGNAL(clicked()), this, SLOT(on_Button_triggered())) != nullptr, "signal/slot connection failed");
}

void ezQtPropertyEditorCurve1DWidget::SetSelection(const ezHybridArray<ezPropertySelection, 8>& items)
{
  ezQtPropertyWidget::SetSelection(items);

  UpdatePreview();
}

void ezQtPropertyEditorCurve1DWidget::OnInit() {}
void ezQtPropertyEditorCurve1DWidget::DoPrepareToDie() {}

void ezQtPropertyEditorCurve1DWidget::UpdatePreview()
{
  if (m_Items.IsEmpty())
    return;

  const ezDocumentObject* pParent = m_Items[0].m_pObject;
  const ezDocumentObject* pCurve = m_pObjectAccessor->GetChildObject(pParent, m_pProp->GetPropertyName(), {});
  const ezColorAttribute* pColorAttr = m_pProp->GetAttributeByType<ezColorAttribute>();
  const ezCurveExtentsAttribute* pExtentsAttr = m_pProp->GetAttributeByType<ezCurveExtentsAttribute>();
  const ezDefaultValueAttribute* pDefAttr = m_pProp->GetAttributeByType<ezDefaultValueAttribute>();
  const ezClampValueAttribute* pClampAttr = m_pProp->GetAttributeByType<ezClampValueAttribute>();

  const bool bLowerFixed = pExtentsAttr ? pExtentsAttr->m_bLowerExtentFixed : false;
  const bool bUpperFixed = pExtentsAttr ? pExtentsAttr->m_bUpperExtentFixed : false;
  const double fLowerExt = pExtentsAttr ? pExtentsAttr->m_fLowerExtent : 0.0;
  const double fUpperExt = pExtentsAttr ? pExtentsAttr->m_fUpperExtent : 1.0;
  const ezColorGammaUB color = pColorAttr ? pColorAttr->GetColor() : ezColor::GreenYellow;
  const double fLowerRange = (pClampAttr && pClampAttr->GetMinValue().IsNumber()) ? pClampAttr->GetMinValue().ConvertTo<double>() : 0.0;
  const double fUpperRange = (pClampAttr && pClampAttr->GetMaxValue().IsNumber()) ? pClampAttr->GetMaxValue().ConvertTo<double>() : 1.0;
  const double fDefVal = (pDefAttr && pDefAttr->GetValue().IsNumber()) ? pDefAttr->GetValue().ConvertTo<double>() : 0.0;

  m_pButton->UpdatePreview(m_pObjectAccessor, pCurve, QColor(color.r, color.g, color.b), fLowerExt, bLowerFixed, fUpperExt, bUpperFixed, fDefVal, fLowerRange, fUpperRange);
}

void ezQtPropertyEditorCurve1DWidget::on_Button_triggered()
{
  const ezDocumentObject* pParent = m_Items[0].m_pObject;
  const ezDocumentObject* pCurve = m_pObjectAccessor->GetChildObject(pParent, m_pProp->GetPropertyName(), {});
  const ezColorAttribute* pColorAttr = m_pProp->GetAttributeByType<ezColorAttribute>();
  const ezCurveExtentsAttribute* pExtentsAttr = m_pProp->GetAttributeByType<ezCurveExtentsAttribute>();
  const ezClampValueAttribute* pClampAttr = m_pProp->GetAttributeByType<ezClampValueAttribute>();

  // TODO: would like to have one transaction open to finish/cancel at the end
  // but also be able to undo individual steps while editing
  // m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->StartTransaction("Edit Curve");

  ezQtCurveEditDlg* pDlg = new ezQtCurveEditDlg(m_pObjectAccessor, pCurve, this);
  pDlg->restoreGeometry(ezQtCurveEditDlg::GetLastDialogGeometry());

  if (pColorAttr)
  {
    pDlg->SetCurveColor(pColorAttr->GetColor());
  }

  if (pExtentsAttr)
  {
    pDlg->SetCurveExtents(pExtentsAttr->m_fLowerExtent, pExtentsAttr->m_bLowerExtentFixed, pExtentsAttr->m_fUpperExtent, pExtentsAttr->m_bUpperExtentFixed);
  }

  if (pClampAttr)
  {
    const double fLower = pClampAttr->GetMinValue().IsNumber() ? pClampAttr->GetMinValue().ConvertTo<double>() : -ezMath::HighValue<double>();
    const double fUpper = pClampAttr->GetMaxValue().IsNumber() ? pClampAttr->GetMaxValue().ConvertTo<double>() : ezMath::HighValue<double>();

    pDlg->SetCurveRanges(fLower, fUpper);
  }

  if (pDlg->exec() == QDialog::Accepted)
  {
    // m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->FinishTransaction();

    UpdatePreview();
  }
  else
  {
    // m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->CancelTransaction();
  }

  delete pDlg;
}
