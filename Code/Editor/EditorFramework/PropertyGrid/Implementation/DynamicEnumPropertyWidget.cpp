#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/DynamicEnumPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>

ezMap<ezString, QString> ezQtDynamicEnumPropertyWidget::s_LastSearch;

ezQtDynamicEnumPropertyWidget::ezQtDynamicEnumPropertyWidget()
  : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pButton = new QPushButton(this);
  m_pButton->setText("Select");
  m_pButton->setStyleSheet("QPushButton { text-align:left; padding-left:5px; padding-top:3px; padding-bottom:3px; }");

  QSizePolicy policy = m_pButton->sizePolicy();
  policy.setHorizontalStretch(0);
  m_pButton->setSizePolicy(policy);

  m_pLayout->addWidget(m_pButton);
}

void ezQtDynamicEnumPropertyWidget::OnInit()
{
  EZ_ASSERT_DEV(
    m_pProp->GetAttributeByType<ezDynamicEnumAttribute>() != nullptr, "ezQtDynamicEnumPropertyWidget was created without a ezDynamicEnumAttribute!");

  ezVariantType::Enum type = m_pProp->GetSpecificType()->GetVariantType();
  EZ_IGNORE_UNUSED(type);
  EZ_ASSERT_DEV(type != ezVariantType::String && type != ezVariantType::HashedString && type != ezVariantType::StringView, "ezDynamicEnumAttribute can't be used with string types");

  const ezDynamicEnumAttribute* pAttr = m_pProp->GetAttributeByType<ezDynamicEnumAttribute>();

  m_sEnumAttribute = pAttr->GetDynamicEnumName();

  m_pEnum = &ezDynamicEnum::GetDynamicEnum(m_sEnumAttribute);



  m_pMenu = new QMenu(m_pButton);
  m_pMenu->setToolTipsVisible(false);
  connect(m_pMenu, &QMenu::aboutToShow, this, &ezQtDynamicEnumPropertyWidget::onMenuAboutToShow);
  m_pButton->setMenu(m_pMenu);
}

void ezQtDynamicEnumPropertyWidget::InternalSetValue(const ezVariant& value)
{

  m_pButton->setText(ezMakeQString(m_pEnum->GetValueName(value.ConvertTo<ezInt64>())));
}

void ezQtDynamicEnumPropertyWidget::onMenuAboutToShow()
{
  m_pMenu->clear();

  m_pSearchableMenu = new ezQtSearchableMenu(m_pMenu);

  connect(m_pSearchableMenu, &ezQtSearchableMenu::MenuItemTriggered, m_pMenu, [this](const QString& sName, const QVariant& variant)
    {
      if (variant.typeId() == QMetaType::QString)
      {
        if (variant.toString() == "<cmd>")
        {
          ezActionManager::ExecuteAction({}, m_pEnum->GetEditCommand(), ezActionContext(const_cast<ezDocument*>(m_pGrid->GetDocument())), m_pEnum->GetEditCommandValue()).AssertSuccess();
        }
      }
      else
      {
        InternalSetValue(variant.toLongLong());
        BroadcastValueChanged(variant.toLongLong());
      }

      m_pMenu->close();
      //
    });

  connect(m_pSearchableMenu, &ezQtSearchableMenu::SearchTextChanged, m_pMenu,
    [this](const QString& sText)
    { s_LastSearch[m_sEnumAttribute] = sText; });

  if (!m_pEnum->GetEditCommand().IsEmpty())
  {
    m_pSearchableMenu->AddItem("< Edit Values... >", "", QString("<cmd>"), QIcon(":/GuiFoundation/Icons/Edit.svg"));
  }

  const auto& AllValues = m_pEnum->GetAllValidValues();

  for (auto it = AllValues.GetIterator(); it.IsValid(); ++it)
  {
    m_pSearchableMenu->AddItem(it.Value(), "", it.Key());
  }

  m_pMenu->addAction(m_pSearchableMenu);

  // important to do this last to make sure the search bar gets focus
  m_pSearchableMenu->Finalize(s_LastSearch[m_sEnumAttribute]);
}
