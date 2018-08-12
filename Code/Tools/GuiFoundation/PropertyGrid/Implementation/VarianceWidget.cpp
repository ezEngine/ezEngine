#include <PCH.h>

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
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pValueWidget = new ezQtDoubleSpinBox(this);
  m_pValueWidget->setMinimum(-ezMath::BasicType<double>::GetInfinity());
  m_pValueWidget->setMaximum(ezMath::BasicType<double>::GetInfinity());
  m_pValueWidget->setSingleStep(0.1f);
  m_pValueWidget->setAccelerated(true);
  m_pValueWidget->setDecimals(2);

  m_pVarianceWidget = new QSlider(this);
  m_pVarianceWidget->setOrientation(Qt::Orientation::Horizontal);
  m_pVarianceWidget->setMinimum(0);
  m_pVarianceWidget->setMaximum(100);
  m_pVarianceWidget->setSingleStep(1);

  m_pLayout->addWidget(m_pValueWidget);
  m_pLayout->addWidget(m_pVarianceWidget);

  connect(m_pValueWidget, SIGNAL(editingFinished()), this, SLOT(onEndTemporary()));
  connect(m_pValueWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  connect(m_pVarianceWidget, SIGNAL(sliderPressed()), this, SLOT(onBeginTemporary()));
  connect(m_pVarianceWidget, SIGNAL(sliderReleased()), this, SLOT(onEndTemporary()));
  connect(m_pVarianceWidget, SIGNAL(valueChanged(int)), this, SLOT(SlotVarianceChanged()));
}

void ezQtVarianceTypeWidget::SetSelection(const ezHybridArray<ezPropertySelection, 8>& items)
{
  ezQtEmbeddedClassPropertyWidget::SetSelection(items);
  EZ_ASSERT_DEBUG(m_pResolvedType->IsDerivedFrom<ezVarianceTypeBase>(), "Selection does not match ezVarianceType.");

  OnPropertyChanged("Value");
  OnPropertyChanged("Variance");
}

void ezQtVarianceTypeWidget::onBeginTemporary()
{
  if (!m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;
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

  ezStringBuilder sTemp;
  sTemp.Format("Change Property '{0}'", ezTranslate(m_pProp->GetPropertyName()));
  m_pObjectAccessor->StartTransaction(sTemp);
  {
    ezVariant value;
    ezToolsReflectionUtils::GetVariantFromFloat(m_pValueWidget->value(), m_pValueType->GetVariantType(), value);
    SetPropertyValue(m_pResolvedType->FindPropertyByName("Value"), value);
  }
  m_pObjectAccessor->FinishTransaction();
}


void ezQtVarianceTypeWidget::SlotVarianceChanged()
{
  ezStringBuilder sTemp;
  sTemp.Format("Change Property '{0}' Variance", ezTranslate(m_pProp->GetPropertyName()));
  m_pObjectAccessor->StartTransaction(sTemp);
  {
    double variance = ezMath::Clamp<double>(m_pVarianceWidget->value() / 100.0, 0, 1);
    SetPropertyValue(m_pResolvedType->FindPropertyByName("Variance"), variance);
  }
  m_pObjectAccessor->FinishTransaction();
}

void ezQtVarianceTypeWidget::OnInit()
{
  ezQtEmbeddedClassPropertyWidget::OnInit();
  // Property type adjustments
  ezQtScopedBlockSignals bs(m_pValueWidget);
  const ezAbstractProperty* pValueProp = GetProperty()->GetSpecificType()->FindPropertyByName("Value");
  // const ezAbstractProperty* pVarianceProp = m_pResolvedType->FindPropertyByName("Variance");
  m_pValueType = pValueProp->GetSpecificType();
  if (m_pValueType == ezGetStaticRTTI<ezTime>())
  {
    m_pValueWidget->setDisplaySuffix(" sec");
  }
  else if (m_pValueType == ezGetStaticRTTI<ezAngle>())
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
    m_pValueWidget->setMinimum(pClamp->GetMinValue());
    m_pValueWidget->setMaximum(pClamp->GetMaxValue());
  }
  if (const ezDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<ezDefaultValueAttribute>())
  {
    m_pValueWidget->setDefaultValue(pDefault->GetValue());
  }
}

void ezQtVarianceTypeWidget::DoPrepareToDie()
{
  ezQtEmbeddedClassPropertyWidget::DoPrepareToDie();
}

void ezQtVarianceTypeWidget::OnPropertyChanged(const ezString& sProperty)
{
  if (sProperty == "Value")
  {
    ezQtScopedBlockSignals _(m_pValueWidget);
    ezVariant vVal = GetCommonValue(m_ResolvedObjects, m_pResolvedType->FindPropertyByName("Value"));
    m_pValueWidget->setValue(vVal);
  }
  else if (sProperty == "Variance")
  {
    ezQtScopedBlockSignals _(m_pVarianceWidget);
    ezVariant vVar = GetCommonValue(m_ResolvedObjects, m_pResolvedType->FindPropertyByName("Variance"));
    if (vVar.IsValid())
    {
      m_pVarianceWidget->setValue(vVar.ConvertTo<double>() * 100.0);
    }
    else
    {
      // m_pVarianceWidget->setValueInvalid();
      m_pVarianceWidget->setValue(50);
    }
  }
}
