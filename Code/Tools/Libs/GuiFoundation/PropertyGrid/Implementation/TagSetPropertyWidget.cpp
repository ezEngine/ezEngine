#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/PropertyGrid/Implementation/TagSetPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

/// *** Tag Set ***

ezQtPropertyEditorTagSetWidget::ezQtPropertyEditorTagSetWidget()
  : ezQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QPushButton(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pMenu = nullptr;
  m_pMenu = new QMenu(m_pWidget);
  m_pMenu->setToolTipsVisible(true);
  m_pWidget->setMenu(m_pMenu);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pMenu, SIGNAL(aboutToShow()), this, SLOT(on_Menu_aboutToShow()));
}

ezQtPropertyEditorTagSetWidget::~ezQtPropertyEditorTagSetWidget()
{
  m_Tags.Clear();
  m_pWidget->setMenu(nullptr);

  delete m_pMenu;
  m_pMenu = nullptr;
}

void ezQtPropertyEditorTagSetWidget::SetSelection(const ezArrayPtr<ezPropertySelection>& items)
{
  ezQtPropertyWidget::SetSelection(items);
  InternalUpdateValue();
}

void ezQtPropertyEditorTagSetWidget::OnInit()
{
  EZ_ASSERT_DEV(m_pProp->GetCategory() == ezPropertyCategory::Set && m_pProp->GetSpecificType() == ezGetStaticRTTI<ezConstCharPtr>(),
    "ezQtPropertyEditorTagSetWidget only works with ezTagSet.");

  // Retrieve tag categories.
  const ezTagSetWidgetAttribute* pAssetAttribute = m_pProp->GetAttributeByType<ezTagSetWidgetAttribute>();
  EZ_ASSERT_DEV(pAssetAttribute != nullptr, "ezQtPropertyEditorTagSetWidget needs ezTagSetWidgetAttribute to be set.");
  ezStringBuilder sTagFilter = pAssetAttribute->GetTagFilter();
  m_sTagFilter = sTagFilter;
  ezTempHybridArray<ezStringView, 4> categories;
  sTagFilter.Split(false, categories, ";");

  // Get tags by categories.
  ezTempHybridArray<const ezToolsTag*, 16> tags;
  ezToolsTagRegistry::GetTagsByCategory(categories, tags);

  const char* szCurrentCategory = "";

  // Add valid tags to menu.
  for (const ezToolsTag* pTag : tags)
  {
    if (!pTag->m_sCategory.IsEqual(szCurrentCategory))
    {
      /*QAction* pCategory = */ m_pMenu->addSection(ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Tag.svg"),
        QLatin1String("[") + QString(pTag->m_sCategory.GetData()) + QLatin1String("]"));

      szCurrentCategory = pTag->m_sCategory;

      // remove category from list, as it was added once

      /// \todo ezStringView is POD? -> array<stringview>::Remove(stringview) fails, because of memcmp
      // categories.Remove(szCurrentCategory);

      for (ezUInt32 i = 0; i < categories.GetCount(); ++i)
      {
        if (categories[i] == szCurrentCategory)
        {
          categories.RemoveAtAndCopy(i);
          break;
        }
      }
    }

    QWidgetAction* pAction = new QWidgetAction(m_pMenu);
    QCheckBox* pCheckBox = new QCheckBox(pTag->m_sName.GetData(), m_pMenu);
    pCheckBox->setCheckable(true);
    pCheckBox->setCheckState(Qt::Unchecked);
    pCheckBox->setProperty("Tag", pTag->m_sName.GetData());
    connect(pCheckBox, &QCheckBox::clicked, this, &ezQtPropertyEditorTagSetWidget::onCheckBoxClicked);
    pAction->setDefaultWidget(pCheckBox);

    m_Tags.PushBack(pCheckBox);
    m_pMenu->addAction(pAction);
  }

  ezStringBuilder tmp;

  // if a tag category is empty, it will never show up in the menu, thus the user doesn't know the name of the valid category
  // therefore, for every empty category, add an entry
  for (const auto& catname : categories)
  {
    /*QAction* pCategory = */ m_pMenu->addSection(ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Tag.svg"),
      QLatin1String("[") + QString(catname.GetData(tmp)) + QLatin1String("]"));
  }

  m_pInvalidTagsAnchor = m_pMenu->addSeparator();

  QAction* pEditAction = new QAction(ezQtUiServices::GetSingleton()->GetCachedIconResource(":/GuiFoundation/Icons/Edit.svg"), "Edit Tags...", m_pMenu);
  connect(pEditAction, &QAction::triggered, this, [this]()
    { ezActionManager::ExecuteAction({}, "Engine.Tags", ezActionContext(const_cast<ezDocument*>(m_pGrid->GetDocument())), ezVariant(m_sTagFilter)).AssertSuccess(); });
  m_pMenu->addAction(pEditAction);
}

void ezQtPropertyEditorTagSetWidget::InternalUpdateValue()
{
  ezMap<ezString, ezUInt32> tags;
  // Count used tags of each object in the selection.
  for (auto& item : m_Items)
  {
    ezTempHybridArray<ezVariant, 16> currentSetValues;
    ezStatus status = m_pObjectAccessor->GetValues(item.m_pObject, m_pProp, currentSetValues);
    EZ_ASSERT_DEV(status.Succeeded(), "Failed to get tag keys!");
    for (const ezVariant& key : currentSetValues)
    {
      EZ_ASSERT_DEV(key.GetType() == ezVariantType::String, "Tags are supposed to be of type string!");
      tags[key.Get<ezString>()]++;
    }
  }

  // Update checkbox state
  QString sText;
  ezUInt32 uiCount = m_Items.GetCount();
  for (QCheckBox* pCheckBox : m_Tags)
  {
    ezString value = pCheckBox->property("Tag").toString().toUtf8().data();
    ezUInt32 uiUsed = tags[value];

    // this tag is handled below in the "invalid tags" section instead
    tags.Remove(value);

    ezQtScopedBlockSignals b(pCheckBox);
    if (uiUsed == 0)
    {
      pCheckBox->setCheckState(Qt::CheckState::Unchecked);
    }
    else if (uiUsed == uiCount)
    {
      pCheckBox->setCheckState(Qt::CheckState::Checked);
      sText += value.GetData();
      sText += "|";
    }
    else
    {
      pCheckBox->setCheckState(Qt::CheckState::PartiallyChecked);
      sText = "<Multiple Values>|"; // string is shrunk by one character (see below), so | is a dummy
    }
  }

  // Any tag left over at this point is used by the selection but isn't a known, registered tag
  // (e.g. it was removed from the tags config after being set on an object, or copied in from another project).
  // Rebuild the "invalid tags" menu section to reflect these, since which tags qualify can change with every selection.
  for (QAction* pAction : m_InvalidTagActions)
  {
    m_pMenu->removeAction(pAction);
    pAction->deleteLater();
  }
  m_InvalidTagActions.Clear();

  bool bHasInvalidTags = false;
  for (auto it = tags.GetIterator(); it.IsValid(); ++it)
  {
    if (!ezToolsTagRegistry::IsTagKnown(it.Key()))
    {
      bHasInvalidTags = true;
      break;
    }
  }

  if (bHasInvalidTags)
  {
    QAction* pHeader = m_pMenu->insertSection(m_pInvalidTagsAnchor, ezQtUiServices::GetSingleton()->GetCachedIconResource(":/GuiFoundation/Icons/Warning.svg"), "Invalid Tags");
    m_InvalidTagActions.PushBack(pHeader);

    const ezString sInvalidStyle = "color: #D0342C; font-weight: bold;";

    for (auto it = tags.GetIterator(); it.IsValid(); ++it)
    {
      if (ezToolsTagRegistry::IsTagKnown(it.Key()))
        continue;

      const ezString& sValue = it.Key();
      const ezUInt32 uiUsed = it.Value();

      QWidgetAction* pAction = new QWidgetAction(m_pMenu);
      QCheckBox* pCheckBox = new QCheckBox(sValue.GetData(), m_pMenu);
      pCheckBox->setStyleSheet(sInvalidStyle.GetData());
      pCheckBox->setToolTip("This tag is set on the selected object(s) but is not registered in the project's tag configuration.");
      pCheckBox->setProperty("Tag", sValue.GetData());
      pCheckBox->setCheckState(uiUsed == uiCount ? Qt::CheckState::Checked : Qt::CheckState::PartiallyChecked);
      connect(pCheckBox, &QCheckBox::clicked, this, &ezQtPropertyEditorTagSetWidget::onCheckBoxClicked);
      pAction->setDefaultWidget(pCheckBox);

      m_pMenu->insertAction(m_pInvalidTagsAnchor, pAction);
      m_InvalidTagActions.PushBack(pAction);

      sText += sValue.GetData();
      sText += "|";
    }

    QAction* pRemoveAllAction = new QAction("Remove All Invalid Tags", m_pMenu);
    connect(pRemoveAllAction, &QAction::triggered, this, &ezQtPropertyEditorTagSetWidget::onRemoveInvalidTagsClicked);
    m_pMenu->insertAction(m_pInvalidTagsAnchor, pRemoveAllAction);
    m_InvalidTagActions.PushBack(pRemoveAllAction);
  }

  ezQtScopedBlockSignals b(m_pWidget);
  if (!sText.isEmpty())
    sText = sText.left(sText.size() - 1);
  else
    sText = "<none>";

  // indicate invalid tags via an icon rather than coloring the whole button text,
  // since only the invalid tag names themselves should read as red (see the menu entries above)
  if (bHasInvalidTags)
    m_pWidget->setIcon(ezQtUiServices::GetSingleton()->GetCachedIconResource(":/GuiFoundation/Icons/Warning.svg"));
  else
    m_pWidget->setIcon(QIcon());

  m_pWidget->setText(sText);
}

void ezQtPropertyEditorTagSetWidget::onRemoveInvalidTagsClicked()
{
  m_pObjectAccessor->StartTransaction("Remove Invalid Tags");

  for (auto& item : m_Items)
  {
    ezTempHybridArray<ezVariant, 16> currentSetValues;
    ezStatus status = m_pObjectAccessor->GetValues(item.m_pObject, m_pProp, currentSetValues);
    EZ_ASSERT_DEV(status.Succeeded(), "Failed to get tag keys!");

    // remove from the back, so that indices of not-yet-removed values stay valid
    for (ezUInt32 i = currentSetValues.GetCount(); i-- > 0;)
    {
      const ezString sValue = currentSetValues[i].Get<ezString>();
      if (ezToolsTagRegistry::IsTagKnown(sValue))
        continue;

      auto res = m_pObjectAccessor->RemoveValue(item.m_pObject, m_pProp, i);
      if (res.Failed())
      {
        EZ_REPORT_FAILURE("Failed to remove invalid '{0}' tag from tag set", sValue);
      }
    }
  }

  m_pObjectAccessor->FinishTransaction();
}

void ezQtPropertyEditorTagSetWidget::on_Menu_aboutToShow()
{
  m_pMenu->setMinimumWidth(m_pWidget->geometry().width());
}

void ezQtPropertyEditorTagSetWidget::onCheckBoxClicked(bool bChecked)
{
  QCheckBox* pCheckBox = qobject_cast<QCheckBox*>(sender());
  ezVariant value = pCheckBox->property("Tag").toString().toUtf8().data();
  if (pCheckBox->isChecked())
  {
    m_pObjectAccessor->StartTransaction("Add Tag");

    // Add tag to all objects in selection that don't have it yet.
    for (auto& item : m_Items)
    {
      ezTempHybridArray<ezVariant, 16> currentSetValues;

      ezStatus status = m_pObjectAccessor->GetValues(item.m_pObject, m_pProp, currentSetValues);
      EZ_ASSERT_DEV(status.Succeeded(), "Failed to get tag keys!");
      if (!currentSetValues.Contains(value))
      {
        auto res = m_pObjectAccessor->InsertValue(item.m_pObject, m_pProp, value, -1);
        if (res.Failed())
        {
          EZ_REPORT_FAILURE("Failed to add '{0}' tag to tag set", value.Get<ezString>());
        }
      }
    }
  }
  else
  {
    m_pObjectAccessor->StartTransaction("Remove Tag");

    ezRemoveObjectPropertyCommand cmd;
    cmd.m_sProperty = m_pProp->GetPropertyName();

    // Remove tag from all objects in selection that have it.
    for (auto& item : m_Items)
    {
      ezTempHybridArray<ezVariant, 16> currentSetValues;
      ezStatus status = m_pObjectAccessor->GetValues(item.m_pObject, m_pProp, currentSetValues);
      EZ_ASSERT_DEV(status.Succeeded(), "Failed to get tag keys!");
      ezUInt32 uiIndex = currentSetValues.IndexOf(value);
      if (uiIndex != -1)
      {
        auto res = m_pObjectAccessor->RemoveValue(item.m_pObject, m_pProp, uiIndex);
        if (res.Failed())
        {
          EZ_REPORT_FAILURE("Failed to remove '{0}' tag from tag set", value.Get<ezString>());
        }
      }
    }
  }

  m_pObjectAccessor->FinishTransaction();
}
