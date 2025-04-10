#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/PropertyGrid/Implementation/VarianceWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QBoxLayout>
#include <QSlider>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezQtVarianceTypeWidget::ezQtVarianceTypeWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pValueWidget = new ezQtDoubleSpinBox(this);
  m_pValueWidget->installEventFilter(m_pValueWidget);
  m_pValueWidget->setMinimum(-ezMath::Infinity<double>());
  m_pValueWidget->setMaximum(ezMath::Infinity<double>());
  m_pValueWidget->setSingleStep(0.1f);
  m_pValueWidget->setAccelerated(true);
  m_pValueWidget->setDecimals(2);

  m_pVarianceWidget = new QSlider(this);
  m_pVarianceWidget->setOrientation(Qt::Orientation::Horizontal);
  m_pVarianceWidget->setMinimum(0);
  m_pVarianceWidget->setMaximum(100);
  m_pVarianceWidget->setSingleStep(1);

  QLabel* pText = new QLabel("Variance:");
  pText->setToolTip("Random deviation of base value:\nSlider to the left -> 0 variance, no randomness at all.\nSlider in the middle -> 0.5 variance, value will be in range [0.5 * base ... 1.5 * base]\nSlider to the right -> full variance, value will be in range [0 ... 2 * base]\n\nNote that values deviate from base using a Bell curve, meaning that values close to 'base' are more likely.");

  m_pLayout->addWidget(m_pValueWidget);
  m_pLayout->addWidget(pText);
  m_pLayout->addWidget(m_pVarianceWidget);

  connect(m_pValueWidget, SIGNAL(editingFinished()), this, SLOT(onEndTemporary()));
  connect(m_pValueWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  connect(m_pVarianceWidget, SIGNAL(sliderPressed()), this, SLOT(onBeginTemporary()));
  connect(m_pVarianceWidget, SIGNAL(sliderReleased()), this, SLOT(onEndTemporary()));
  connect(m_pVarianceWidget, SIGNAL(valueChanged(int)), this, SLOT(SlotVarianceChanged()));
}

void ezQtVarianceTypeWidget::SetSelection(const ezHybridArray<ezPropertySelection, 8>& items)
{
  ezQtStandardPropertyWidget::SetSelection(items);
  EZ_ASSERT_DEBUG(m_pProp->GetSpecificType()->IsDerivedFrom<ezVarianceTypeBase>(), "Selection does not match ezVarianceType.");
}

void ezQtVarianceTypeWidget::onBeginTemporary()
{
  if (!m_bTemporaryCommand)
  {
    Broadcast(ezPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }
}

void ezQtVarianceTypeWidget::onEndTemporary()
{
  if (m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void ezQtVarianceTypeWidget::SlotValueChanged()
{
  onBeginTemporary();

  ezVariant value;
  ezToolsReflectionUtils::GetVariantFromFloat(m_pValueWidget->value(), m_pValueProp->GetSpecificType()->GetVariantType(), value);

  auto obj = m_OldValue.Get<ezTypedObject>();
  void* pCopy = ezReflectionSerializer::Clone(obj.m_pObject, obj.m_pType);
  ezReflectionUtils::SetMemberPropertyValue(m_pValueProp, pCopy, value);
  ezVariant newValue;
  newValue.MoveTypedObject(pCopy, obj.m_pType);

  BroadcastValueChanged(newValue);
}


void ezQtVarianceTypeWidget::SlotVarianceChanged()
{
  double variance = ezMath::Clamp<double>(m_pVarianceWidget->value() / 100.0, 0, 1);

  ezVariant newValue = m_OldValue;
  ezTypedPointer ptr = newValue.GetWriteAccess();
  ezReflectionUtils::SetMemberPropertyValue(m_pVarianceProp, ptr.m_pObject, variance);

  BroadcastValueChanged(newValue);
}

void ezQtVarianceTypeWidget::OnInit()
{
  m_pValueProp = static_cast<const ezAbstractMemberProperty*>(GetProperty()->GetSpecificType()->FindPropertyByName("Value"));
  m_pVarianceProp = static_cast<const ezAbstractMemberProperty*>(GetProperty()->GetSpecificType()->FindPropertyByName("Variance"));

  // Property type adjustments
  ezQtScopedBlockSignals bs(m_pValueWidget);
  const ezRTTI* pValueType = m_pValueProp->GetSpecificType();
  if (pValueType == ezGetStaticRTTI<ezTime>())
  {
    m_pValueWidget->setDisplaySuffix(" sec");
  }
  else if (pValueType == ezGetStaticRTTI<ezAngle>())
  {
    m_pValueWidget->setDisplaySuffix(ezStringUtf8(L"\u00B0").GetData());
  }

  // Handle attributes
  if (const ezSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<ezSuffixAttribute>())
  {
    m_pValueWidget->setDisplaySuffix(pSuffix->GetSuffix());
  }
  if (const ezClampValueAttribute* pClamp = m_pProp->GetAttributeByType<ezClampValueAttribute>())
  {
    if (pClamp->GetMinValue().CanConvertTo<double>() || pClamp->GetMinValue().IsA<ezTime>() || pClamp->GetMinValue().IsA<ezAngle>())
    {
      m_pValueWidget->setMinimum(pClamp->GetMinValue());
    }
    else if (const ezRTTI* pType = pClamp->GetMinValue().GetReflectedType(); pType && pType->IsDerivedFrom<ezVarianceTypeBase>())
    {
      m_pValueWidget->setMinimum(pClamp->GetMinValue()["Value"]);
      m_pVarianceWidget->setMinimum(static_cast<ezInt32>(pClamp->GetMinValue()["Variance"].ConvertTo<double>() * 100.0));
    }
    if (pClamp->GetMaxValue().CanConvertTo<double>())
    {
      m_pValueWidget->setMaximum(pClamp->GetMaxValue());
    }
    else if (const ezRTTI* pType = pClamp->GetMaxValue().GetReflectedType(); pType && pType->IsDerivedFrom<ezVarianceTypeBase>())
    {
      m_pValueWidget->setMaximum(pClamp->GetMaxValue()["Value"]);
      m_pVarianceWidget->setMaximum(static_cast<ezInt32>(pClamp->GetMaxValue()["Variance"].ConvertTo<double>() * 100.0));
    }
  }
  if (const ezDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<ezDefaultValueAttribute>())
  {
    if (pDefault->GetValue().CanConvertTo<double>() || pDefault->GetValue().IsA<ezTime>() || pDefault->GetValue().IsA<ezAngle>())
    {
      m_pValueWidget->setDefaultValue(pDefault->GetValue());
    }
    else if (const ezRTTI* pType = pDefault->GetValue().GetReflectedType(); pType && pType->IsDerivedFrom<ezVarianceTypeBase>())
    {
      m_pValueWidget->setDefaultValue(pDefault->GetValue()["Value"]);
    }
  }
}

void ezQtVarianceTypeWidget::InternalSetValue(const ezVariant& value)
{
  ezQtScopedBlockSignals bs(m_pValueWidget, m_pVarianceWidget);
  if (value.IsValid())
  {
    m_pValueWidget->setValue(value["Value"]);
    m_pVarianceWidget->setValue(value["Variance"].ConvertTo<double>() * 100.0);
  }
  else
  {
    m_pValueWidget->setValueInvalid();
    m_pVarianceWidget->setValue(50);
  }
}
