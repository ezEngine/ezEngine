#include <PCH.h>
#include <EditorPluginParticle/Widgets/VarianceWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <QBoxLayout>
#include <CoreUtils/Localization/TranslationLookup.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

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

  m_pVarianceWidget = new ezQtDoubleSpinBox(this);
  m_pVarianceWidget->setMinimum(0);
  m_pVarianceWidget->setMaximum(100);
  m_pVarianceWidget->setSingleStep(1);
  m_pVarianceWidget->setAccelerated(true);
  m_pVarianceWidget->setDecimals(0);

  m_pLayout->addWidget(m_pValueWidget);
  m_pLayout->addWidget(m_pVarianceWidget);

  connect(m_pValueWidget, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
  connect(m_pValueWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  connect(m_pVarianceWidget, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
  connect(m_pVarianceWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
}

void ezQtVarianceTypeWidget::SetSelection(const ezHybridArray<ezPropertySelection, 8>& items)
{
  ezQtEmbeddedClassPropertyWidget::SetSelection(items);
  EZ_ASSERT_DEBUG(m_pResolvedType == ezGetStaticRTTI<ezVarianceType>(), "Selection does not match ezVarianceType.");

  OnPropertyChanged("Value");
  OnPropertyChanged("Variance");
}

void ezQtVarianceTypeWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void ezQtVarianceTypeWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  ezObjectAccessorBase* pObjectAccessor = m_pGrid->GetObjectAccessor();
  ezStringBuilder sTemp; sTemp.Format("Change Property '{0}'", ezTranslate(m_pProp->GetPropertyName()));
  pObjectAccessor->StartTransaction(sTemp);
  {
    SetPropertyValue(m_pResolvedType->FindPropertyByName("Value"), m_pValueWidget->value());
    SetPropertyValue(m_pResolvedType->FindPropertyByName("Variance"), m_pVarianceWidget->value());
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
    QtScopedBlockSignals _(m_pValueWidget);
    ezVariant vVal = GetCommonValue(m_ResolvedObjects, m_pResolvedType->FindPropertyByName("Value"));
    if (vVal.IsValid())
    {
      m_pValueWidget->setValue(vVal.ConvertTo<double>());
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
      m_pVarianceWidget->setValue(vVar.ConvertTo<double>());
    }
    else
    {
      m_pVarianceWidget->setValueInvalid();
    }
  }
}
