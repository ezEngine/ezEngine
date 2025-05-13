#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/DynamicEnumPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>

ezQtDynamicEnumPropertyWidget::ezQtDynamicEnumPropertyWidget()
  : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QComboBox(this);
  m_pWidget->installEventFilter(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);

  EZ_VERIFY(connect(m_pWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(on_CurrentEnum_changed(int))) != nullptr, "connection failed");
}

void ezQtDynamicEnumPropertyWidget::OnInit()
{
  EZ_ASSERT_DEV(
    m_pProp->GetAttributeByType<ezDynamicEnumAttribute>() != nullptr, "ezQtDynamicEnumPropertyWidget was created without a ezDynamicEnumAttribute!");

  const ezDynamicEnumAttribute* pAttr = m_pProp->GetAttributeByType<ezDynamicEnumAttribute>();

  m_pEnum = &ezDynamicEnum::GetDynamicEnum(pAttr->GetDynamicEnumName());
  const auto& AllValues = m_pEnum->GetAllValidValues();

  ezQtScopedBlockSignals bs(m_pWidget);

  for (auto it = AllValues.GetIterator(); it.IsValid(); ++it)
  {
    m_pWidget->addItem(QString::fromUtf8(it.Value().GetData()), it.Key());
  }

  if (!m_pEnum->GetEditCommand().IsEmpty())
  {
    m_pWidget->addItem("< Edit Values... >", QString("<cmd>"));
  }
}

void ezQtDynamicEnumPropertyWidget::InternalSetValue(const ezVariant& value)
{
  ezQtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    m_iLastIndex = m_pWidget->findData(value.ConvertTo<ezInt64>());
  }
  else
  {
    m_iLastIndex = -1;
  }

  m_pWidget->setCurrentIndex(m_iLastIndex);
}

void ezQtDynamicEnumPropertyWidget::on_CurrentEnum_changed(int iEnum)
{
  if (m_pWidget->currentData() == QString("<cmd>"))
  {
    iEnum = m_iLastIndex;
    m_pWidget->setCurrentIndex(iEnum);

    ezActionManager::ExecuteAction({}, m_pEnum->GetEditCommand(), ezActionContext(const_cast<ezDocument*>(m_pGrid->GetDocument())), m_pEnum->GetEditCommandValue()).AssertSuccess();

    return;
  }

  m_iLastIndex = m_pWidget->currentIndex();
  ezInt64 iValue = m_pWidget->itemData(iEnum).toLongLong();
  BroadcastValueChanged(iValue);
}
