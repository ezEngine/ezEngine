#include <PCH.h>
#include <EditorFramework/PropertyGrid/DynamicStringEnumPropertyWidget.moc.h>
#include <QBoxLayout>
#include <QComboBox>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

ezQtDynamicStringEnumPropertyWidget::ezQtDynamicStringEnumPropertyWidget() : ezQtStandardPropertyWidget()
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
  EZ_ASSERT_DEV(m_pProp->GetAttributeByType<ezDynamicStringEnumAttribute>() != nullptr, "ezQtDynamicStringEnumPropertyWidget was created without a ezDynamicStringEnumAttribute!");

  const ezDynamicStringEnumAttribute* pAttr = m_pProp->GetAttributeByType<ezDynamicStringEnumAttribute>();

  const auto& denum = ezDynamicStringEnum::GetDynamicEnum(pAttr->GetDynamicEnumName());
  const auto& AllValues = denum.GetAllValidValues();

  QtScopedBlockSignals bs(m_pWidget);

  for (const auto& val : AllValues)
  {
    m_pWidget->addItem(QString::fromUtf8(val.GetData()));
  }
}

void ezQtDynamicStringEnumPropertyWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    ezInt32 iIndex = m_pWidget->findText(value.ConvertTo<ezString>().GetData());
    //EZ_ASSERT_DEV(iIndex != -1, "Enum widget is set to an invalid value!"); // 'invalid value'
    m_pWidget->setCurrentIndex(iIndex);
  }
  else
  {
    m_pWidget->setCurrentIndex(-1);
  }
}

void ezQtDynamicStringEnumPropertyWidget::on_CurrentEnum_changed(int iEnum)
{
  QString sValue = m_pWidget->itemText(iEnum);
  BroadcastValueChanged(sValue.toUtf8().data());
}
