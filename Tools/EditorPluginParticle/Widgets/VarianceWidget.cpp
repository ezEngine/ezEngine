#include <PCH.h>
#include <EditorPluginParticle/Widgets/VarianceWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <QBoxLayout>
#include <CoreUtils/Localization/TranslationLookup.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

ezQtVarianceTypeWidget::ezQtVarianceTypeWidget()
{
  m_bTemporaryCommand = false;

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
  m_pVarianceWidget->setSingleStep(5);
  m_pVarianceWidget->setAccelerated(true);
  m_pVarianceWidget->setDecimals(0);

  m_pLayout->addWidget(m_pValueWidget);
  m_pLayout->addWidget(m_pVarianceWidget);

  connect(m_pValueWidget, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
  connect(m_pValueWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  connect(m_pVarianceWidget, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
  connect(m_pVarianceWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
}

void ezQtVarianceTypeWidget::SetSelection(const ezHybridArray<Selection, 8>& items)
{
  QtScopedBlockSignals _1(m_pValueWidget);
  QtScopedBlockSignals _2(m_pVarianceWidget);

  ezQtPropertyWidget::SetSelection(items);
  ezObjectAccessorBase* pObjectAccessor = m_pGrid->GetObjectAccessor();

  ezVariant vVal, vVar;

  // check if we have multiple values
  for (const auto& item : items)
  {
    ezUuid ObjectGuid = pObjectAccessor->Get<ezUuid>(item.m_pObject, m_pProp, item.m_Index);
    const ezDocumentObject* pObject = pObjectAccessor->GetObject(ObjectGuid);

    if (pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezVarianceType>())
    {
      ezVariant val = pObject->GetTypeAccessor().GetValue("Value");
      ezVariant var = pObject->GetTypeAccessor().GetValue("Variance");

      if (!vVal.IsValid())
      {
        vVal = val;
        vVar = var;
        continue;
      }

      if (vVal != val || vVar != var)
      {
        vVal = ezVariant();
        vVar = ezVariant();
        break;
      }
    }
  }

  if (vVal.IsValid() && vVar.IsValid())
  {
    m_pValueWidget->setValue(vVal.ConvertTo<double>());
    m_pVarianceWidget->setValue(vVar.ConvertTo<double>());
  }
  else
  {
    m_pValueWidget->setValueInvalid();
    m_pVarianceWidget->setValueInvalid();
  }
}

void ezQtVarianceTypeWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(ezQtPropertyWidget::Event::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void ezQtVarianceTypeWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(ezQtPropertyWidget::Event::Type::BeginTemporary);

  m_bTemporaryCommand = true;


  ezStringBuilder sTemp;
  sTemp.Format("Change Property '{0}'", ezTranslate(m_pProp->GetPropertyName()));

  ezObjectAccessorBase* pObjectAccessor = m_pGrid->GetObjectAccessor();
  pObjectAccessor->StartTransaction(sTemp);

  const ezVariant var = m_pVarianceWidget->value();
  const ezVariant val = m_pValueWidget->value();

  ezStatus res;
  for (const auto& item : GetSelection())
  {
    ezUuid ObjectGuid = pObjectAccessor->Get<ezUuid>(item.m_pObject, m_pProp, item.m_Index);
    const ezDocumentObject* pObject = pObjectAccessor->GetObject(ObjectGuid);

    if (pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezVarianceType>())
    {
      res = pObjectAccessor->SetValue(pObject, "Value", val);

      if (res.Failed())
        break;

      res = pObjectAccessor->SetValue(pObject, "Variance", var);

      if (res.Failed())
        break;
    }
  }

  if (res.m_Result.Failed())
    pObjectAccessor->CancelTransaction();
  else
    pObjectAccessor->FinishTransaction();

  ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Changing the property failed.");
}
