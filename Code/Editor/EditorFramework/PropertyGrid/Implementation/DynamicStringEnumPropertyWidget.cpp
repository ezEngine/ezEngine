#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/DynamicStringEnumMenuButton.moc.h>
#include <EditorFramework/PropertyGrid/DynamicStringEnumPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

ezQtDynamicStringEnumPropertyWidget::ezQtDynamicStringEnumPropertyWidget()
  : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pButton = new ezQtDynamicStringEnumMenuButton(this);

  QSizePolicy policy = m_pButton->sizePolicy();
  policy.setHorizontalStretch(0);
  m_pButton->setSizePolicy(policy);

  connect(m_pButton, &ezQtDynamicStringEnumMenuButton::ValueSelected, this,
    [this](const QString& sValue)
    { SetNewValue(sValue.toUtf8().data()); });

  m_pLayout->addWidget(m_pButton);
}

void ezQtDynamicStringEnumPropertyWidget::OnInit()
{
  EZ_ASSERT_DEV(m_pProp->GetAttributeByType<ezDynamicStringEnumAttribute>() != nullptr,
    "ezQtDynamicStringEnumPropertyWidget was created without a ezDynamicStringEnumAttribute!");
  ezVariantType::Enum type = m_pProp->GetSpecificType()->GetVariantType();
  EZ_IGNORE_UNUSED(type);
  EZ_ASSERT_DEV(type == ezVariantType::String || type == ezVariantType::HashedString || type == ezVariantType::StringView, "ezDynamicStringEnumAttribute can only be used with string types");

  const ezDynamicStringEnumAttribute* pAttr = m_pProp->GetAttributeByType<ezDynamicStringEnumAttribute>();

  m_pButton->SetEnum(pAttr->GetDynamicEnumName());
  m_pButton->SetDocument(m_pGrid->GetDocument());

  if (auto pDefaultValueAttr = m_pProp->GetAttributeByType<ezDefaultValueAttribute>())
  {
    m_pButton->GetEnum()->AddValidValue(pDefaultValueAttr->GetValue().ConvertTo<ezString>(), true);
  }
}

void ezQtDynamicStringEnumPropertyWidget::InternalSetValue(const ezVariant& value)
{
  m_pButton->SetCurrentValue(value.ConvertTo<ezString>());
}

void ezQtDynamicStringEnumPropertyWidget::SetNewValue(ezStringView sNewValue)
{
  ezVariant v;
  ezVariantType::Enum type = m_pProp->GetSpecificType()->GetVariantType();
  if (type == ezVariantType::String || type == ezVariantType::StringView)
  {
    v = ezVariant(sNewValue);
  }
  else if (type == ezVariantType::HashedString)
  {
    ezHashedString s;
    s.Assign(sNewValue);
    v = s;
  }
  else
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  InternalSetValue(v);
  BroadcastValueChanged(v);
}
