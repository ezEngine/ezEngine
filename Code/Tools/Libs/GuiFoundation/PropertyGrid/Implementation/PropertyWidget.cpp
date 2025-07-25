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

  const char* szLabels[] = {"X",
    "Y",
    "Z",
    "W"};

  const ezColorGammaUB labelColors[] = {ezColorScheme::LightUI(ezColorScheme::Red),
    ezColorScheme::LightUI(ezColorScheme::Green),
    ezColorScheme::LightUI(ezColorScheme::Blue),
    ezColorScheme::LightUI(ezColorScheme::Gray)};

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

    if (m_iNumComponents > 1)
    {
      QLabel* pLabel = new QLabel(szLabels[c]);
      QPalette palette = pLabel->palette();
      palette.setColor(pLabel->foregroundRole(), QColor(labelColors[c].r, labelColors[c].g, labelColors[c].b));
      pLabel->setPalette(palette);
      m_pLayout->addWidget(pLabel);
    }

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

  m_OriginalType = GetProperty()->GetSpecificType()->GetVariantType();
  if (m_OriginalType == ezVariantType::Invalid)
  {
    m_OriginalType = value.GetType();
  }

  if (m_OriginalType == ezVariantType::Invalid)
  {
    m_OriginalType = ezVariantType::Double;
  }

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
  {
    Broadcast(ezPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }

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
  {
    Broadcast(ezPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }

  BroadcastValueChanged(ezTime::MakeFromSeconds(m_pWidget->value()));
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
  {
    Broadcast(ezPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }

  BroadcastValueChanged(ezAngle::MakeFromDegree(m_pWidget->value()));
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

void ezQtPropertyEditorIntSpinboxWidget::SetReadOnly(bool bReadOnly /*= true*/)
{
  for (ezUInt32 i = 0; i < 4; ++i)
  {
    if (m_pWidget[i])
      m_pWidget[i]->setReadOnly(bReadOnly);
  }

  if (m_pSlider)
  {
    m_pSlider->setDisabled(bReadOnly);
  }
}

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

          connect(m_pSlider, SIGNAL(sliderPressed()), this, SLOT(onBeginTemporary()));
          connect(m_pSlider, SIGNAL(sliderReleased()), this, SLOT(onEndTemporary()));
          connect(m_pSlider, SIGNAL(valueChanged(int)), this, SLOT(SlotSliderValueChanged(int)));
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

  auto prop = GetProperty();
  const ezRTTI* type = prop->GetSpecificType();
  m_OriginalType = type->GetVariantType();
  if (m_OriginalType == ezVariantType::Invalid)
  {
    m_OriginalType = value.GetType();
  }

  if (m_OriginalType == ezVariantType::Invalid)
  {
    m_OriginalType = ezVariantType::Int32;
  }

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
  {
    Broadcast(ezPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }

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

void ezQtPropertyEditorIntSpinboxWidget::onBeginTemporary()
{
  if (m_bUseTemporaryTransaction && !m_bTemporaryCommand)
  {
    Broadcast(ezPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }
}

void ezQtPropertyEditorIntSpinboxWidget::onEndTemporary()
{
  if (m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void ezQtPropertyEditorIntSpinboxWidget::SlotSliderValueChanged(int value)
{
  {
    ezQtScopedBlockSignals b0(m_pWidget[0]);
    m_pWidget[0]->setValue(value);
  }

  BroadcastValueChanged(ezVariant(m_pSlider->value()).ConvertTo(m_OriginalType));
}

void ezQtPropertyEditorIntSpinboxWidget::on_EditingFinished_triggered()
{
  onEndTemporary();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezMap<ezString, ezQtImageSliderWidget::ImageGeneratorFunc> ezQtImageSliderWidget::s_ImageGenerators;

ezQtImageSliderWidget::ezQtImageSliderWidget(ImageGeneratorFunc generator, double fMinValue, double fMaxValue, QWidget* pParent)
  : QWidget(pParent)
{
  m_Generator = generator;
  m_fMinValue = fMinValue;
  m_fMaxValue = fMaxValue;

  setAutoFillBackground(false);
}

void ezQtImageSliderWidget::SetValue(double fValue)
{
  if (m_fValue == fValue)
    return;

  m_fValue = fValue;
  update();
}

void ezQtImageSliderWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::RenderHint::Antialiasing);

  const QRect area = rect();

  if (area.width() != m_Image.width())
    UpdateImage();

  painter.drawTiledPixmap(area, QPixmap::fromImage(m_Image));

  const float factor = ezMath::Unlerp(m_fMinValue, m_fMaxValue, m_fValue);

  const double pos = (int)(factor * area.width()) + 0.5f;

  const double top = area.top() + 0.5;
  const double bot = area.bottom() + 0.5;
  const double len = 5.0;
  const double wid = 2.0;

  const QColor col = qRgb(80, 80, 80);

  painter.setPen(col);
  painter.setBrush(col);

  {
    QPainterPath path;
    path.moveTo(QPointF(pos - wid, top));
    path.lineTo(QPointF(pos, top + len));
    path.lineTo(QPointF(pos + wid, top));
    path.closeSubpath();

    painter.drawPath(path);
  }

  {
    QPainterPath path;
    path.moveTo(QPointF(pos - wid, bot));
    path.lineTo(QPointF(pos, bot - len));
    path.lineTo(QPointF(pos + wid, bot));
    path.closeSubpath();

    painter.drawPath(path);
  }
}

void ezQtImageSliderWidget::UpdateImage()
{
  const int width = rect().width();

  if (m_Generator)
  {
    m_Image = m_Generator(rect().width(), rect().height(), m_fMinValue, m_fMaxValue);
  }
  else
  {
    m_Image = QImage(width, 1, QImage::Format::Format_RGB32);

    ezColorGammaUB cg = ezColor::HotPink;
    for (int x = 0; x < width; ++x)
    {
      m_Image.setPixel(x, 0, qRgb(cg.r, cg.g, cg.b));
    }
  }
}

void ezQtImageSliderWidget::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons().testFlag(Qt::LeftButton))
  {
    const int width = rect().width();
    const int height = rect().height();

    QPoint coord = event->pos();
    const int x = ezMath::Clamp(coord.x(), 0, width - 1);

    const double fx = (double)x / (width - 1);
    const double val = ezMath::Lerp(m_fMinValue, m_fMaxValue, fx);

    valueChanged(val);
  }

  event->accept();
}

void ezQtImageSliderWidget::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
  {
    Q_EMIT sliderPressed();
  }

  mouseMoveEvent(event);

  event->accept();
}

void ezQtImageSliderWidget::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
  {
    Q_EMIT sliderReleased();
  }
  event->accept();
}

/// *** SLIDER ***

ezQtPropertyEditorSliderWidget::ezQtPropertyEditorSliderWidget()
  : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);
}

ezQtPropertyEditorSliderWidget::~ezQtPropertyEditorSliderWidget() = default;

void ezQtPropertyEditorSliderWidget::OnInit()
{
  const ezImageSliderUiAttribute* pSliderAttr = m_pProp->GetAttributeByType<ezImageSliderUiAttribute>();
  const ezClampValueAttribute* pRange = m_pProp->GetAttributeByType<ezClampValueAttribute>();
  EZ_ASSERT_DEV(pRange != nullptr, "ezImageSliderUiAttribute always has to be compined with ezClampValueAttribute to specify the valid range.");
  EZ_ASSERT_DEV(pRange->GetMinValue().IsValid() && pRange->GetMaxValue().IsValid(), "The min and max values used with ezImageSliderUiAttribute both have to be valid.");

  m_fMinValue = pRange->GetMinValue().ConvertTo<double>();
  m_fMaxValue = pRange->GetMaxValue().ConvertTo<double>();

  m_pSlider = new ezQtImageSliderWidget(ezQtImageSliderWidget::s_ImageGenerators[pSliderAttr->m_sImageGenerator], m_fMinValue, m_fMaxValue, this);

  m_pLayout->insertWidget(0, m_pSlider);
  connect(m_pSlider, SIGNAL(sliderPressed()), this, SLOT(onBeginTemporary()));
  connect(m_pSlider, SIGNAL(sliderReleased()), this, SLOT(onEndTemporary()));
  connect(m_pSlider, SIGNAL(valueChanged(double)), this, SLOT(SlotSliderValueChanged(double)));

  if (const ezDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<ezDefaultValueAttribute>())
  {
    ezQtScopedBlockSignals bs(m_pSlider);

    if (pDefault->GetValue().CanConvertTo<double>())
    {
      m_pSlider->SetValue(pDefault->GetValue().ConvertTo<double>());
    }
  }
}

void ezQtPropertyEditorSliderWidget::InternalSetValue(const ezVariant& value)
{
  ezQtScopedBlockSignals bs(m_pSlider);

  m_OriginalType = GetProperty()->GetSpecificType()->GetVariantType();

  if (m_OriginalType == ezVariantType::Invalid)
  {
    m_OriginalType = value.GetType();
  }

  if (m_OriginalType == ezVariantType::Invalid)
  {
    m_OriginalType = ezVariantType::Double;
  }

  m_pSlider->SetValue(value.ConvertTo<double>());
}

void ezQtPropertyEditorSliderWidget::SlotSliderValueChanged(double fValue)
{
  if (!m_bTemporaryCommand)
  {
    Broadcast(ezPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }

  BroadcastValueChanged(ezVariant(fValue).ConvertTo(m_OriginalType));

  m_pSlider->SetValue(fValue);
}

void ezQtPropertyEditorSliderWidget::on_EditingFinished_triggered()
{
  onEndTemporary();
}

void ezQtPropertyEditorSliderWidget::onBeginTemporary()
{
  if (!m_bTemporaryCommand)
  {
    Broadcast(ezPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }
}

void ezQtPropertyEditorSliderWidget::onEndTemporary()
{
  if (m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

/// *** QUATERNION ***

ezQtPropertyEditorQuaternionWidget::ezQtPropertyEditorQuaternionWidget()
  : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  const char* szLabels[] = {"R",
    "P",
    "Y"};
  const char* szTooltip[] = {"Roll (Rotation around the forward axis)",
    "Pitch (Rotation around the side axis)",
    "Yaw (Rotation around the up axis)"};

  const ezColorGammaUB labelColors[] = {ezColorScheme::LightUI(ezColorScheme::Red),
    ezColorScheme::LightUI(ezColorScheme::Green),
    ezColorScheme::LightUI(ezColorScheme::Blue)};

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

    QLabel* pLabel = new QLabel(szLabels[c]);
    QPalette palette = pLabel->palette();
    palette.setColor(pLabel->foregroundRole(), QColor(labelColors[c].r, labelColors[c].g, labelColors[c].b));
    pLabel->setPalette(palette);
    pLabel->setToolTip(szTooltip[c]);

    m_pLayout->addWidget(pLabel);
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
  {
    Broadcast(ezPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }

  ezAngle x = ezAngle::MakeFromDegree(m_pWidget[0]->value());
  ezAngle y = ezAngle::MakeFromDegree(m_pWidget[1]->value());
  ezAngle z = ezAngle::MakeFromDegree(m_pWidget[2]->value());

  ezQuat qRot = ezQuat::MakeFromEulerAngles(x, y, z);

  BroadcastValueChanged(qRot);
}

/// *** TRANSFORM ***

ezQtPropertyEditorTransformWidget::ezQtPropertyEditorTransformWidget()
  : ezQtStandardPropertyWidget()
{
  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  m_pLayout->setSpacing(1);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  const char* szXYZLabels[] = {"X",
    "Y",
    "Z"};
  const char* szRotLabels[] = {"R",
    "P",
    "Y"};
  const char* szRotTooltip[] = {"Roll (Rotation around the forward axis)",
    "Pitch (Rotation around the side axis)",
    "Yaw (Rotation around the up axis)"};

  const ezColorGammaUB labelColors[] = {ezColorScheme::LightUI(ezColorScheme::Red),
    ezColorScheme::LightUI(ezColorScheme::Green),
    ezColorScheme::LightUI(ezColorScheme::Blue)};

  ezUInt32 uiCurrentWidget = 0;
  auto AddWidget = [&](QLayout* layout, double fStep, const char* szLabel, const char* szTooltip, ezColorGammaUB color, const char* szDisplaySuffix)
  {
    auto pSpinBox = new ezQtDoubleSpinBox(this);
    pSpinBox->installEventFilter(this);
    pSpinBox->setMinimum(-ezMath::Infinity<double>());
    pSpinBox->setMaximum(ezMath::Infinity<double>());
    pSpinBox->setSingleStep(fStep);
    pSpinBox->setAccelerated(true);
    pSpinBox->setDisplaySuffix(szDisplaySuffix);

    policy.setHorizontalStretch(2);
    pSpinBox->setSizePolicy(policy);

    QLabel* pLabel = new QLabel(szLabel);
    QPalette palette = pLabel->palette();
    palette.setColor(pLabel->foregroundRole(), QColor(color.r, color.g, color.b));
    pLabel->setPalette(palette);
    pLabel->setToolTip(szTooltip);

    layout->addWidget(pLabel);
    layout->addWidget(pSpinBox);

    connect(pSpinBox, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(pSpinBox, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));

    m_pWidget[uiCurrentWidget] = pSpinBox;
    ++uiCurrentWidget;
  };

  // Position
  {
    auto pSubLayout = new QHBoxLayout(this);
    pSubLayout->setSpacing(6);
    m_pLayout->addLayout(pSubLayout);

    for (ezUInt32 c = 0; c < 3; ++c)
    {
      AddWidget(pSubLayout, 0.1, szXYZLabels[c], "", labelColors[c], "");
    }
  }

  // Rotation
  {
    auto pSubLayout = new QHBoxLayout(this);
    pSubLayout->setSpacing(6);
    m_pLayout->addLayout(pSubLayout);

    for (ezUInt32 c = 0; c < 3; ++c)
    {
      AddWidget(pSubLayout, 1.0, szRotLabels[c], szRotTooltip[c], labelColors[c], "\xC2\xB0");
    }
  }

  // Scale
  {
    auto pSubLayout = new QHBoxLayout(this);
    pSubLayout->setSpacing(6);
    m_pLayout->addLayout(pSubLayout);

    for (ezUInt32 c = 0; c < 3; ++c)
    {
      AddWidget(pSubLayout, 0.1, szXYZLabels[c], "", labelColors[c], "");
    }
  }
}

void ezQtPropertyEditorTransformWidget::OnInit() {}

void ezQtPropertyEditorTransformWidget::InternalSetValue(const ezVariant& value)
{
  if (m_bTemporaryCommand)
    return;

  ezQtScopedBlockSignals b0(m_pWidget[0]);
  ezQtScopedBlockSignals b1(m_pWidget[1]);
  ezQtScopedBlockSignals b2(m_pWidget[2]);
  ezQtScopedBlockSignals b3(m_pWidget[3]);
  ezQtScopedBlockSignals b4(m_pWidget[4]);
  ezQtScopedBlockSignals b5(m_pWidget[5]);
  ezQtScopedBlockSignals b6(m_pWidget[6]);
  ezQtScopedBlockSignals b7(m_pWidget[7]);
  ezQtScopedBlockSignals b8(m_pWidget[8]);

  if (value.IsValid())
  {
    const ezTransform t = value.ConvertTo<ezTransform>();

    m_pWidget[0]->setValue(t.m_vPosition.x);
    m_pWidget[1]->setValue(t.m_vPosition.y);
    m_pWidget[2]->setValue(t.m_vPosition.z);

    ezAngle x, y, z;
    t.m_qRotation.GetAsEulerAngles(x, y, z);
    m_pWidget[3]->setValue(x.GetDegree());
    m_pWidget[4]->setValue(y.GetDegree());
    m_pWidget[5]->setValue(z.GetDegree());

    m_pWidget[6]->setValue(t.m_vScale.x);
    m_pWidget[7]->setValue(t.m_vScale.y);
    m_pWidget[8]->setValue(t.m_vScale.z);
  }
  else
  {
    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_pWidget); ++i)
    {
      m_pWidget[i]->setValueInvalid();
    }
  }
}

void ezQtPropertyEditorTransformWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void ezQtPropertyEditorTransformWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
  {
    Broadcast(ezPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }

  ezTransform t;

  t.m_vPosition.x = m_pWidget[0]->value();
  t.m_vPosition.y = m_pWidget[1]->value();
  t.m_vPosition.z = m_pWidget[2]->value();

  ezAngle x = ezAngle::MakeFromDegree(m_pWidget[3]->value());
  ezAngle y = ezAngle::MakeFromDegree(m_pWidget[4]->value());
  ezAngle z = ezAngle::MakeFromDegree(m_pWidget[5]->value());
  t.m_qRotation = ezQuat::MakeFromEulerAngles(x, y, z);

  t.m_vScale.x = m_pWidget[6]->value();
  t.m_vScale.y = m_pWidget[7]->value();
  t.m_vScale.z = m_pWidget[8]->value();

  BroadcastValueChanged(t);
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

void ezQtPropertyEditorLineEditWidget::SetReadOnly(bool bReadOnly /*= true*/)
{
  m_pWidget->setReadOnly(bReadOnly);
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

  m_OriginalType = GetProperty()->GetSpecificType()->GetVariantType();

  if (m_OriginalType == ezVariantType::Invalid)
  {
    m_OriginalType = value.GetType();
  }

  if (m_OriginalType == ezVariantType::Invalid)
  {
    m_OriginalType = ezVariantType::String;
  }

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
    const ezColorGammaUB col = ezColor::LightGrey;

    QColor qol;
    qol.setRgb(col.r, col.g, col.b, col.a);

    m_Pal.setBrush(QPalette::Window, QBrush(qol, Qt::DiagCrossPattern));
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
  if (GetProperty() && GetProperty()->GetSpecificType() == ezGetStaticRTTI<ezColor>())
  {
    m_bIsHDR = true;
  }

  Broadcast(ezPropertyEvent::Type::BeginTemporary);

  ezColor temp = ezColor::White;
  if (m_OriginalValue.IsValid())
  {
    temp = m_OriginalValue.ConvertTo<ezColor>();
  }

  ezQtUiServices::GetSingleton()->ShowColorDialog(temp, m_bExposeAlpha, m_bIsHDR, this, SLOT(on_CurrentColor_changed(const ezColor&)), SLOT(on_Color_accepted()), SLOT(on_Color_reset()));
}

void ezQtPropertyEditorColorWidget::on_CurrentColor_changed(const ezColor& color)
{
  ezVariant col;

  if (!m_bIsHDR)
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
}

void ezQtPropertyEditorEnumWidget::OnInit()
{
  const ezRTTI* pType = m_pProp->GetSpecificType();

  const ezUInt32 uiCount = pType->GetProperties().GetCount();

  ezHybridArray<const ezAbstractProperty*, 16> props;

  // Start at 1 to skip default value.
  for (ezUInt32 i = 1; i < uiCount; ++i)
  {
    auto pProp = pType->GetProperties()[i];

    if (pProp->GetCategory() != ezPropertyCategory::Constant)
      continue;

    props.PushBack(pProp);
  }

  // this code path implements using multiple buttons in a row instead of a combobox, for small number of entries
  // it works for 2 elements, but often already looks bad with 3 elements
  // but even with 2 elements, it just adds visual clutter (unused values are now visible)
  // so I'm not going to enable it, but keep it in, in case we want to try it again in the future
  constexpr bool bUseButtons = false;

  if (bUseButtons && props.GetCount() <= EZ_ARRAY_SIZE(m_pButtons))
  {
    for (ezUInt32 i = 0; i < props.GetCount(); ++i)
    {
      auto pProp = props[i];

      const ezAbstractConstantProperty* pConstant = static_cast<const ezAbstractConstantProperty*>(pProp);

      m_pButtons[i] = new QPushButton(this);
      m_pButtons[i]->setText(ezMakeQString(ezTranslate(pConstant->GetPropertyName())));
      m_pButtons[i]->setCheckable(true);
      m_pButtons[i]->setProperty("value", pConstant->GetConstant().ConvertTo<ezInt64>());

      connect(m_pButtons[i], SIGNAL(clicked(bool)), this, SLOT(on_ButtonClicked_changed(bool)));

      m_pLayout->addWidget(m_pButtons[i]);
    }
  }
  else
  {
    m_pWidget = new QComboBox(this);
    m_pWidget->installEventFilter(this);
    m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_pLayout->addWidget(m_pWidget);

    connect(m_pWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(on_CurrentEnum_changed(int)));

    ezQtScopedBlockSignals bs(m_pWidget);

    for (ezUInt32 i = 0; i < props.GetCount(); ++i)
    {
      auto pProp = props[i];

      const ezAbstractConstantProperty* pConstant = static_cast<const ezAbstractConstantProperty*>(pProp);

      m_pWidget->addItem(ezMakeQString(ezTranslate(pConstant->GetPropertyName())), pConstant->GetConstant().ConvertTo<ezInt64>());
    }
  }
}

void ezQtPropertyEditorEnumWidget::InternalSetValue(const ezVariant& value)
{

  if (m_pWidget)
  {
    ezInt32 iIndex = -1;
    if (value.IsValid())
    {
      iIndex = m_pWidget->findData(value.ConvertTo<ezInt64>());
      EZ_ASSERT_DEV(iIndex != -1, "Enum widget is set to an invalid value!");
    }

    ezQtScopedBlockSignals b(m_pWidget);
    m_pWidget->setCurrentIndex(iIndex);
  }
  else
  {
    const ezInt64 iValue = value.ConvertTo<ezInt64>();

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_pButtons); ++i)
    {
      if (m_pButtons[i])
      {
        const ezInt64 iButtonValue = m_pButtons[i]->property("value").toLongLong();

        ezQtScopedBlockSignals b(m_pButtons[i]);
        m_pButtons[i]->setChecked(iButtonValue == iValue);
      }
    }
  }
}

void ezQtPropertyEditorEnumWidget::on_CurrentEnum_changed(int iEnum)
{
  const ezInt64 iValue = m_pWidget->itemData(iEnum).toLongLong();
  BroadcastValueChanged(iValue);
}

void ezQtPropertyEditorEnumWidget::on_ButtonClicked_changed(bool checked)
{
  if (QPushButton* pButton = qobject_cast<QPushButton*>(sender()))
  {
    const ezInt64 iValue = pButton->property("value").toLongLong();
    BroadcastValueChanged(iValue);
  }
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
  m_pMenu = new QMenu(m_pWidget);
  m_pWidget->setMenu(m_pMenu);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pMenu, SIGNAL(aboutToShow()), this, SLOT(on_Menu_aboutToShow()));
  connect(m_pMenu, SIGNAL(aboutToHide()), this, SLOT(on_Menu_aboutToHide()));
}

ezQtPropertyEditorBitflagsWidget::~ezQtPropertyEditorBitflagsWidget()
{
  m_pWidget->setMenu(nullptr);
  delete m_pMenu;
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
    QCheckBox* pCheckBox = new QCheckBox(ezMakeQString(ezTranslate(pConstant->GetPropertyName())), m_pMenu);
    pCheckBox->setCheckable(true);
    pCheckBox->setCheckState(Qt::Unchecked);
    pAction->setDefaultWidget(pCheckBox);

    m_Constants[pConstant->GetConstant().ConvertTo<ezInt64>()] = pCheckBox;
    m_pMenu->addAction(pAction);
  }

  // sets all bits to clear or set
  {
    QWidgetAction* pAllAction = new QWidgetAction(m_pMenu);
    m_pAllButton = new QPushButton(QString::fromUtf8("All"), m_pMenu);
    connect(m_pAllButton, &QPushButton::clicked, this, [this](bool bChecked)
      { SetAllChecked(true); });
    pAllAction->setDefaultWidget(m_pAllButton);
    m_pMenu->addAction(pAllAction);

    QWidgetAction* pClearAction = new QWidgetAction(m_pMenu);
    m_pClearButton = new QPushButton(QString::fromUtf8("Clear"), m_pMenu);
    connect(m_pClearButton, &QPushButton::clicked, this, [this](bool bChecked)
      { SetAllChecked(false); });
    pClearAction->setDefaultWidget(m_pClearButton);
    m_pMenu->addAction(pClearAction);
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

void ezQtPropertyEditorBitflagsWidget::SetAllChecked(bool bChecked)
{
  for (auto& pCheckBox : m_Constants)
  {
    pCheckBox.Value()->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);
  }
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
  pObjectAccessor->GetCountByName(pCurveObject, "ControlPoints", iNumPoints).AssertSuccess();

  ezVariant v;
  ezHybridArray<ezVec2d, 32> points;
  points.Reserve(iNumPoints);

  double minX = fLowerExtents * 4800.0;
  double maxX = fUpperExtents * 4800.0;

  double minY = fLowerRange;
  double maxY = fUpperRange;

  for (ezInt32 i = 0; i < iNumPoints; ++i)
  {
    const ezDocumentObject* pPoint = pObjectAccessor->GetChildObjectByName(pCurveObject, "ControlPoints", i);

    ezVec2d p;

    pObjectAccessor->GetValueByName(pPoint, "Tick", v).AssertSuccess();
    p.x = v.ConvertTo<double>();

    pObjectAccessor->GetValueByName(pPoint, "Value", v).AssertSuccess();
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
    points.Sort([](const ezVec2d& lhs, const ezVec2d& rhs) -> bool
      { return lhs.x < rhs.x; });

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
  const ezDocumentObject* pCurve = m_pObjectAccessor->GetChildObjectByName(pParent, m_pProp->GetPropertyName(), {});
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
  const ezDocumentObject* pCurve = m_pObjectAccessor->GetChildObjectByName(pParent, m_pProp->GetPropertyName(), {});
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
