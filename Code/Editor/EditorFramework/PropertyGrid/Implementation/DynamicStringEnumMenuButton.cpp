#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/EditDynamicEnumsDlg.moc.h>
#include <EditorFramework/PropertyGrid/DynamicStringEnumMenuButton.moc.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <ToolsFoundation/Document/Document.h>

ezMap<ezString, QString> ezQtDynamicStringEnumMenuButton::s_LastSearch;

ezQtDynamicStringEnumMenuButton::ezQtDynamicStringEnumMenuButton(QWidget* pParent)
  : QPushButton(pParent)
{
  setText("Select");
  setStyleSheet("QPushButton { text-align:left; padding-left:5px; padding-top:3px; padding-bottom:3px; }");

  m_pMenu = new QMenu(this);
  m_pMenu->setToolTipsVisible(false);
  connect(m_pMenu, &QMenu::aboutToShow, this, &ezQtDynamicStringEnumMenuButton::onMenuAboutToShow);
  setMenu(m_pMenu);
}

void ezQtDynamicStringEnumMenuButton::SetEnum(ezStringView sEnumName)
{
  m_sEnumName = sEnumName;
  m_pEnum = &ezDynamicStringEnum::GetDynamicEnum(m_sEnumName);
}

void ezQtDynamicStringEnumMenuButton::SetCurrentValue(ezStringView sValue)
{
  setText(ezMakeQString(sValue));
}

void ezQtDynamicStringEnumMenuButton::onMenuAboutToShow()
{
  if (m_pEnum == nullptr)
    return;

  m_pMenu->clear();

  m_pSearchableMenu = new ezQtSearchableMenu(m_pMenu);

  connect(m_pSearchableMenu, &ezQtSearchableMenu::MenuItemTriggered, m_pMenu, [this](const QString& sName, const QVariant& variant)
    {
      if (variant.toString() == "<item>")
      {
        Q_EMIT ValueSelected(sName);
      }
      else if (variant.toString() == "<edit>")
      {
        ezQtEditDynamicEnumsDlg dlg(m_pEnum, this);
        if (dlg.exec() == QDialog::Accepted)
        {
          ezInt32 iEnum = dlg.GetSelectedItem();
          if (iEnum >= 0)
          {
            Q_EMIT ValueSelected(ezMakeQString(m_pEnum->GetAllValidValues()[iEnum]));
          }
        }
      }
      else if (variant.toString() == "<cmd>")
      {
        ezActionManager::ExecuteAction({}, m_pEnum->GetEditCommand(), ezActionContext(const_cast<ezDocument*>(m_pDocument)), m_pEnum->GetEditCommandValue()).AssertSuccess();
      }

      m_pMenu->close(); });

  connect(m_pSearchableMenu, &ezQtSearchableMenu::SearchTextChanged, m_pMenu,
    [this](const QString& sText)
    { s_LastSearch[m_sEnumName] = sText; });

  {
    ezDynamicStringEnum::RefreshValuesEvent e;
    e.m_sEnumName = m_sEnumName;
    e.m_pDocument = m_pDocument;
    e.m_pEnum = m_pEnum;
    ezDynamicStringEnum::s_RefreshValuesEvent.Broadcast(e);
  }

  for (const auto& val : m_pEnum->GetAllValidValues())
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
  m_pSearchableMenu->Finalize(s_LastSearch[m_sEnumName]);
}
