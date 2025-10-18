#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/EditDynamicEnumsDlg.moc.h>
#include <EditorFramework/PropertyGrid/DynamicStringEnumPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>

ezMap<ezString, QString> ezQtDynamicStringEnumPropertyWidget::s_LastSearch;

ezQtDynamicStringEnumPropertyWidget::ezQtDynamicStringEnumPropertyWidget()
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

void ezQtDynamicStringEnumPropertyWidget::OnInit()
{
  EZ_ASSERT_DEV(m_pProp->GetAttributeByType<ezDynamicStringEnumAttribute>() != nullptr,
    "ezQtDynamicStringEnumPropertyWidget was created without a ezDynamicStringEnumAttribute!");

  const ezDynamicStringEnumAttribute* pAttr = m_pProp->GetAttributeByType<ezDynamicStringEnumAttribute>();

  m_sEnumAttribute = pAttr->GetDynamicEnumName();

  m_pEnum = &ezDynamicStringEnum::GetDynamicEnum(m_sEnumAttribute);

  if (auto pDefaultValueAttr = m_pProp->GetAttributeByType<ezDefaultValueAttribute>())
  {
    m_pEnum->AddValidValue(pDefaultValueAttr->GetValue().ConvertTo<ezString>(), true);
  }

  m_pMenu = new QMenu(m_pButton);
  m_pMenu->setToolTipsVisible(false);
  connect(m_pMenu, &QMenu::aboutToShow, this, &ezQtDynamicStringEnumPropertyWidget::onMenuAboutToShow);
  m_pButton->setMenu(m_pMenu);
}

void ezQtDynamicStringEnumPropertyWidget::InternalSetValue(const ezVariant& value)
{
  m_pButton->setText(value.ConvertTo<ezString>().GetData());
}

void ezQtDynamicStringEnumPropertyWidget::onMenuAboutToShow()
{
  m_pMenu->clear();

  m_pSearchableMenu = new ezQtSearchableMenu(m_pMenu);

  connect(m_pSearchableMenu, &ezQtSearchableMenu::MenuItemTriggered, m_pMenu, [this](const QString& sName, const QVariant& variant)
    {
      if (variant.toString() == "<item>")
      {
        InternalSetValue(sName.toUtf8().data());
        BroadcastValueChanged(sName.toUtf8().data());
      }
      else if (variant.toString() == "<edit>")
      {

        ezQtEditDynamicEnumsDlg dlg(m_pEnum, this);
        if (dlg.exec() == QDialog::Accepted)
        {
          ezInt32 iEnum = dlg.GetSelectedItem();
          if (iEnum >= 0)
          {
            InternalSetValue(m_pEnum->GetAllValidValues()[iEnum]);
            BroadcastValueChanged(m_pEnum->GetAllValidValues()[iEnum]);
          }
        }
      }
      else if (variant.toString() == "<cmd>")
      {
        ezActionManager::ExecuteAction({}, m_pEnum->GetEditCommand(), ezActionContext(const_cast<ezDocument*>(m_pGrid->GetDocument())), m_pEnum->GetEditCommandValue()).AssertSuccess();
      }

      m_pMenu->close();
      //
    });

  connect(m_pSearchableMenu, &ezQtSearchableMenu::SearchTextChanged, m_pMenu,
    [this](const QString& sText)
    { s_LastSearch[m_sEnumAttribute] = sText; });

  const auto& AllValues = m_pEnum->GetAllValidValues();

  for (const auto& val : AllValues)
  {
    m_pSearchableMenu->AddItem(val, "", QString("<item>"));
  }

  if (!m_pEnum->GetEditCommand().IsEmpty())
  {
    m_pSearchableMenu->AddItem("< Edit Values... >", "", QString("<cmd>"), QIcon(":/GuiFoundation/Icons/Edit.svg"));
  }
  else if (!m_pEnum->GetStorageFile().IsEmpty())
  {
    m_pSearchableMenu->AddItem("< Edit Values... >", "", QString("<edit>"), QIcon(":/GuiFoundation/Icons/Edit.svg"));
  }

  m_pMenu->addAction(m_pSearchableMenu);

  // important to do this last to make sure the search bar gets focus
  m_pSearchableMenu->Finalize(s_LastSearch[m_sEnumAttribute]);
}
