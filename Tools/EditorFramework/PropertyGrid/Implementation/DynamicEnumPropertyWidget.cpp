#include <PCH.h>
#include <EditorFramework/PropertyGrid/DynamicEnumPropertyWidget.moc.h>
#include <QBoxLayout>
#include <QComboBox>
#include <GuiFoundation/UIServices/DynamicEnums.h>

ezQtDynamicEnumPropertyWidget::ezQtDynamicEnumPropertyWidget() : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new QComboBox(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);

  EZ_VERIFY(connect(m_pWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(on_CurrentEnum_changed(int))) != nullptr, "connection failed");
}

void ezQtDynamicEnumPropertyWidget::OnInit()
{
  EZ_ASSERT_DEV(m_pProp->GetAttributeByType<ezDynamicEnumAttribute>() != nullptr, "ezQtDynamicEnumPropertyWidget was created without a ezDynamicEnumAttribute!");

  const ezDynamicEnumAttribute* pAttr = m_pProp->GetAttributeByType<ezDynamicEnumAttribute>();

  const auto& denum = ezDynamicEnum::GetDynamicEnum(pAttr->GetDynamicEnumName());
  const auto& AllValues = denum.GetAllValidValues();

  QtScopedBlockSignals bs(m_pWidget);

  for (auto it = AllValues.GetIterator(); it.IsValid(); ++it)
  {
    m_pWidget->addItem(QString::fromUtf8(it.Value().GetData()), it.Key());
  }
}

void ezQtDynamicEnumPropertyWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    ezInt32 iIndex = m_pWidget->findData(value.ConvertTo<ezInt64>());
    //EZ_ASSERT_DEV(iIndex != -1, "Enum widget is set to an invalid value!"); // 'invalid value'
    m_pWidget->setCurrentIndex(iIndex);
  }
  else
  {
    m_pWidget->setCurrentIndex(-1);
  }
}

void ezQtDynamicEnumPropertyWidget::on_CurrentEnum_changed(int iEnum)
{
  ezInt64 iValue = m_pWidget->itemData(iEnum).toLongLong();
  BroadcastValueChanged(iValue);
}
