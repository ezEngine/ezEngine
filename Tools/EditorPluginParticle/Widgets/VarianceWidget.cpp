#include <PCH.h>
#include <EditorPluginParticle/Widgets/VarianceWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <QBoxLayout>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QSlider>

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
  connect(m_pVarianceWidget, SIGNAL(valueChanged(int)), this, SLOT(SlotValueChanged()));
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
  ezObjectAccessorBase* pObjectAccessor = m_pGrid->GetObjectAccessor();
  ezStringBuilder sTemp; sTemp.Format("Change Property '{0}'", ezTranslate(m_pProp->GetPropertyName()));
  pObjectAccessor->StartTransaction(sTemp);
  {
    double variance = ezMath::Clamp<double>(m_pVarianceWidget->value() / 100.0, 0, 1);
    SetPropertyValue(m_pResolvedType->FindPropertyByName("Variance"), variance);

    if (m_pValueType == ezGetStaticRTTI<ezTime>())
    {
      SetPropertyValue(m_pResolvedType->FindPropertyByName("Value"), ezTime::Seconds(m_pValueWidget->value()));
    }
    else if (m_pValueType == ezGetStaticRTTI<ezAngle>())
    {
      SetPropertyValue(m_pResolvedType->FindPropertyByName("Value"), ezAngle::Degree(m_pValueWidget->value()));
    }
    else
    {
      SetPropertyValue(m_pResolvedType->FindPropertyByName("Value"), m_pValueWidget->value());
    }

  }
  pObjectAccessor->FinishTransaction();
}

void ezQtVarianceTypeWidget::OnInit()
{
  ezQtEmbeddedClassPropertyWidget::OnInit();
}

void ezQtVarianceTypeWidget::DoPrepareToDie()
{
  ezQtEmbeddedClassPropertyWidget::DoPrepareToDie();
}

void ezQtVarianceTypeWidget::OnPropertyChanged(const ezString& sProperty)
{
  ezObjectAccessorBase* pObjectAccessor = m_pGrid->GetObjectAccessor();

  if (sProperty == "Value")
  {
    if (m_pValueType == nullptr)
    {
      ezAbstractProperty* pProp = m_pResolvedType->FindPropertyByName("Value");

      m_pValueType = pProp->GetSpecificType();

      if (m_pValueType == ezGetStaticRTTI<ezTime>())
      {
        m_pValueWidget->setDisplaySuffix(" (sec)");
      }
      else if (m_pValueType == ezGetStaticRTTI<ezAngle>())
      {
        m_pValueWidget->setDisplaySuffix(ezStringUtf8(L"\u00B0").GetData());
      }
    }

    QtScopedBlockSignals _(m_pValueWidget);
    ezVariant vVal = GetCommonValue(m_ResolvedObjects, m_pResolvedType->FindPropertyByName("Value"));
    if (vVal.IsValid())
    {
      if (vVal.IsA<ezTime>())
      {
        m_pValueWidget->setValue(vVal.Get<ezTime>().GetSeconds());
      }
      else if (vVal.IsA<ezAngle>())
      {
        m_pValueWidget->setValue(vVal.Get<ezAngle>().GetDegree());
      }
      else
      {
        m_pValueWidget->setValue(vVal.ConvertTo<double>());
      }
    }
    else
    {
      m_pValueWidget->setValueInvalid();
    }
  }
  else if (sProperty == "Variance")
  {
    QtScopedBlockSignals _(m_pVarianceWidget);
    ezVariant vVar = GetCommonValue(m_ResolvedObjects, m_pResolvedType->FindPropertyByName("Variance"));
    if (vVar.IsValid())
    {
      m_pVarianceWidget->setValue(vVar.ConvertTo<double>() * 100.0);
    }
    else
    {
      //m_pVarianceWidget->setValueInvalid();
      m_pVarianceWidget->setValue(50);
    }
  }
}
