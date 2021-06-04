#include <EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/EditDynamicEnumsDlg.moc.h>
#include <EditorFramework/PropertyGrid/DynamicStringEnumPropertyWidget.moc.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

ezQtDynamicStringEnumPropertyWidget::ezQtDynamicStringEnumPropertyWidget()
  : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new QComboBox(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);

  EZ_VERIFY(connect(m_pWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(on_CurrentEnum_changed(int))) != nullptr, "connection failed");
}

void ezQtDynamicStringEnumPropertyWidget::OnInit()
{
  EZ_ASSERT_DEV(m_pProp->GetAttributeByType<ezDynamicStringEnumAttribute>() != nullptr,
    "ezQtDynamicStringEnumPropertyWidget was created without a ezDynamicStringEnumAttribute!");

  const ezDynamicStringEnumAttribute* pAttr = m_pProp->GetAttributeByType<ezDynamicStringEnumAttribute>();

  m_pEnum = &ezDynamicStringEnum::GetDynamicEnum(pAttr->GetDynamicEnumName());
  const auto& AllValues = m_pEnum->GetAllValidValues();

  ezQtScopedBlockSignals bs(m_pWidget);
  m_pWidget->clear();

  for (const auto& val : AllValues)
  {
    m_pWidget->addItem(QString::fromUtf8(val.GetData()));
  }

  if (!ezStringUtils::IsNullOrEmpty(m_pEnum->GetStorageFile()))
  {
    m_pWidget->addItem("< Edit Values... >", QString("<edit>"));
  }
}

void ezQtDynamicStringEnumPropertyWidget::InternalSetValue(const ezVariant& value)
{
  ezQtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    m_iLastIndex = m_pWidget->findText(value.ConvertTo<ezString>().GetData());
  }
  else
  {
    m_iLastIndex = -1;
  }

  m_pWidget->setCurrentIndex(m_iLastIndex);
}

void ezQtDynamicStringEnumPropertyWidget::on_CurrentEnum_changed(int iEnum)
{
  if (m_pWidget->currentData() == QString("<edit>"))
  {
    ezQtEditDynamicEnumsDlg dlg(m_pEnum, this);
    if (dlg.exec() == QDialog::Accepted)
    {
      iEnum = dlg.GetSelectedItem();
      OnInit();
    }
    else
    {
      iEnum = m_iLastIndex;
    }

    m_pWidget->setCurrentIndex(iEnum);
    return;
  }

  m_iLastIndex = iEnum;
  QString sValue = m_pWidget->itemText(iEnum);
  BroadcastValueChanged(sValue.toUtf8().data());
}
