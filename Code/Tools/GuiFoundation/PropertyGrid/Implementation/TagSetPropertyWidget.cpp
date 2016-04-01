#include <GuiFoundation/PCH.h>
#include <GuiFoundation/PropertyGrid/Implementation/TagSetPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>
#include <CoreUtils/Localization/TranslationLookup.h>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QAction>
#include <QWidgetAction>
#include <QCheckBox>

/// *** Tag Set ***

ezPropertyEditorTagSetWidget::ezPropertyEditorTagSetWidget() : ezQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new QPushButton(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pMenu = nullptr;
  m_pMenu = new QMenu(m_pWidget);
  m_pWidget->setMenu(m_pMenu);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pMenu, SIGNAL(aboutToShow()), this, SLOT(on_Menu_aboutToShow()));
}

ezPropertyEditorTagSetWidget::~ezPropertyEditorTagSetWidget()
{
  m_Tags.Clear();
  m_pWidget->setMenu(nullptr);

  delete m_pMenu;
  m_pMenu = nullptr;
}

void ezPropertyEditorTagSetWidget::SetSelection(const ezHybridArray<Selection, 8>& items)
{
  ezQtPropertyWidget::SetSelection(items);
  InternalUpdateValue();
}

void ezPropertyEditorTagSetWidget::OnInit()
{
  EZ_ASSERT_DEV(m_pProp->GetCategory() == ezPropertyCategory::Set && m_pProp->GetSpecificType() == ezGetStaticRTTI<ezConstCharPtr>()
    , "ezPropertyEditorTagSetWidget only works with ezTagSet.");

  // Retrieve tag categories.
  const ezTagSetWidgetAttribute* pAssetAttribute = m_pProp->GetAttributeByType<ezTagSetWidgetAttribute>();
  EZ_ASSERT_DEV(pAssetAttribute != nullptr, "ezPropertyEditorTagSetWidget needs ezTagSetWidgetAttribute to be set.");
  ezStringBuilder sTagFilter = pAssetAttribute->GetTagFilter();
  ezHybridArray<ezStringView, 4> categories;
  sTagFilter.Split(false, categories, ";");

  // Get tags by categories.
  ezHybridArray<const ezToolsTag*, 16> tags;
  ezToolsTagRegistry::GetTagsByCategory(categories, tags);
  
  const char* szCurrentCategory = "";

  // Add valid tags to menu.
  for (const ezToolsTag* pTag : tags)
  {
    if (!pTag->m_sCategory.IsEqual(szCurrentCategory))
    {
      QAction* pCategory = m_pMenu->addAction(QLatin1String("[") + QString(pTag->m_sCategory.GetData()) + QLatin1String("]"));
      pCategory->setEnabled(false);
      pCategory->setIcon(ezUIServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Tag16.png"));
      szCurrentCategory = pTag->m_sCategory;

      // remove category from list, as it was added once
      
      /// \todo ezStringView is POD? -> array<stringview>::Remove(stringview) fails, because of memcmp
      // categories.Remove(szCurrentCategory);

      for (ezUInt32 i = 0; i < categories.GetCount(); ++i)
      {
        if (categories[i] == szCurrentCategory)
        {
          categories.RemoveAt(i);
          break;
        }
      }
    }

    QWidgetAction* pAction = new QWidgetAction(m_pMenu);
    QCheckBox* pCheckBox = new QCheckBox(pTag->m_sName.GetData(), m_pMenu);
    pCheckBox->setCheckable(true);
    pCheckBox->setCheckState(Qt::Unchecked);
    pCheckBox->setProperty("Tag", pTag->m_sName.GetData());
    connect(pCheckBox, &QCheckBox::clicked, this, &ezPropertyEditorTagSetWidget::onCheckBoxClicked);
    pAction->setDefaultWidget(pCheckBox);

    m_Tags.PushBack(pCheckBox);
    m_pMenu->addAction(pAction);
  }

  // if a tag category is empty, it will never show up in the menu, thus the user doesn't know the name of the valid category
  // therefore, for every empty category, add an entry
  for (const auto& catname : categories)
  {
    QAction* pCategory = m_pMenu->addAction(QLatin1String("[") + QString(catname.GetData()) + QLatin1String("]"));
    pCategory->setEnabled(false);
    pCategory->setIcon(ezUIServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Tag16.png"));
  }
}

void ezPropertyEditorTagSetWidget::InternalUpdateValue()
{
  ezMap<ezString, ezUInt32> tags;

  // Count used tags of each object in the selection.
  for (auto& item : m_Items)
  {
    ezHybridArray<ezVariant, 16> currentSetValues;
    bool bRes = item.m_pObject->GetTypeAccessor().GetValues(m_PropertyPath, currentSetValues);
    EZ_ASSERT_DEV(bRes, "Failed to get tag keys!");
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

    QtScopedBlockSignals b(pCheckBox);
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


  QtScopedBlockSignals b(m_pWidget);
  if (!sText.isEmpty())
    sText = sText.left(sText.size() - 1);
  else
    sText = "<none>";

  //m_pWidget->setIcon(ezUIServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Tag16.png"));
  m_pWidget->setText(sText);
}

void ezPropertyEditorTagSetWidget::on_Menu_aboutToShow()
{
  m_pMenu->setMinimumWidth(m_pWidget->geometry().width());
}

void ezPropertyEditorTagSetWidget::onCheckBoxClicked(bool bChecked)
{
  QCheckBox* pCheckBox = qobject_cast<QCheckBox*>(sender());
  ezVariant value = pCheckBox->property("Tag").toString().toUtf8().data();

  ezCommandHistory* history = m_pGrid->GetCommandHistory();
  history->StartTransaction();

  if (pCheckBox->isChecked())
  {
    ezInsertObjectPropertyCommand cmd;
    cmd.SetPropertyPath(m_PropertyPath.GetPathString());

    // Add tag to all objects in selection that don't have it yet.
    for (auto& item : m_Items)
    {
      ezHybridArray<ezVariant, 16> currentSetValues;
      bool bRes = item.m_pObject->GetTypeAccessor().GetValues(m_PropertyPath, currentSetValues);
      EZ_ASSERT_DEV(bRes, "Failed to get tag keys!");
      if (!currentSetValues.Contains(value))
      {
        cmd.m_Object = item.m_pObject->GetGuid();
        cmd.m_NewValue = value;
        cmd.m_Index = -1;

        auto res = history->AddCommand(cmd);
        if (res.m_Result.Failed())
        {
          EZ_REPORT_FAILURE("Failed to add '%s' tag to tag set", value.Get<ezString>().GetData());
        }
      }
    }
  }
  else
  {
    ezRemoveObjectPropertyCommand cmd;
    cmd.SetPropertyPath(m_PropertyPath.GetPathString());

    // Remove tag from all objects in selection that have it.
    for (auto& item : m_Items)
    {
      ezHybridArray<ezVariant, 16> currentSetValues;
      bool bRes = item.m_pObject->GetTypeAccessor().GetValues(m_PropertyPath, currentSetValues);
      EZ_ASSERT_DEV(bRes, "Failed to get tag keys!");
      ezUInt32 uiIndex = currentSetValues.IndexOf(value);
      if (uiIndex != -1)
      {
        cmd.m_Object = item.m_pObject->GetGuid();
        cmd.m_Index = uiIndex;

        auto res = history->AddCommand(cmd);
        if (res.m_Result.Failed())
        {
          EZ_REPORT_FAILURE("Failed to remove '%s' tag from tag set", value.Get<ezString>().GetData());
        }
      }
    }
  }

  history->FinishTransaction();
}
