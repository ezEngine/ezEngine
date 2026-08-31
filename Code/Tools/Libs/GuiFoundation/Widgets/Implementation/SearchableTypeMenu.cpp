#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>

bool ezQtTypeMenu::s_bShowInDevelopmentFeatures = false;
ezMap<ezString, ezDynamicArray<ezString>>* ezQtSearchableMenuRecentList::s_pStorage = nullptr;

void ezQtSearchableMenuRecentList::SetStorage(ezMap<ezString, ezDynamicArray<ezString>>* pStorage)
{
  s_pStorage = pStorage;
}

void ezQtSearchableMenuRecentList::UseEntry(ezStringView sListName, ezStringView sEntry)
{
  if (s_pStorage == nullptr || sListName.IsEmpty())
    return;

  // the list is ordered most-recently-used first, which is also the order in which the menu displays it
  auto& list = (*s_pStorage)[sListName];

  const ezUInt32 uiIndex = list.IndexOf(sEntry);

  if (uiIndex != ezInvalidIndex && uiIndex != 0)
  {
    // already in the list, but not at the front, remove it
    list.RemoveAtAndCopy(uiIndex);
  }

  if (uiIndex != 0)
  {
    list.InsertAt(0, sEntry);
  }

  while (list.GetCount() > s_uiMaxEntries)
  {
    list.PopBack();
  }
}

ezArrayPtr<const ezString> ezQtSearchableMenuRecentList::GetList(ezStringView sListName)
{
  if (s_pStorage == nullptr)
    return {};

  auto it = s_pStorage->Find(sListName);

  if (!it.IsValid())
    return {};

  return it.Value();
}

struct TypeComparer
{
  EZ_FORCE_INLINE bool Less(const ezRTTI* a, const ezRTTI* b) const
  {
    const ezCategoryAttribute* pCatA = a->GetAttributeByType<ezCategoryAttribute>();
    const ezCategoryAttribute* pCatB = b->GetAttributeByType<ezCategoryAttribute>();
    if (pCatA != nullptr && pCatB == nullptr)
    {
      return true;
    }
    else if (pCatA == nullptr && pCatB != nullptr)
    {
      return false;
    }
    else if (pCatA != nullptr && pCatB != nullptr)
    {
      ezInt32 iRes = ezStringUtils::Compare(pCatA->GetCategory(), pCatB->GetCategory());
      if (iRes != 0)
      {
        return iRes < 0;
      }
    }

    return a->GetTypeName().Compare(b->GetTypeName()) < 0;
  }
};

ezString ezQtTypeMenu::s_sLastMenuSearch;

QMenu* ezQtTypeMenu::CreateCategoryMenu(const char* szCategory, ezMap<ezString, QMenu*>& existingMenus)
{
  if (ezStringUtils::IsNullOrEmpty(szCategory))
    return m_pMenu;


  auto it = existingMenus.Find(szCategory);
  if (it.IsValid())
    return it.Value();

  ezStringBuilder sPath = szCategory;
  sPath.PathParentDirectory();
  sPath.Trim("/");

  QMenu* pParentMenu = m_pMenu;

  if (!sPath.IsEmpty())
  {
    pParentMenu = CreateCategoryMenu(sPath, existingMenus);
  }

  sPath = szCategory;
  sPath = sPath.GetFileName();

  QMenu* pNewMenu = pParentMenu->addMenu(ezMakeQString(ezTranslate(sPath)));
  existingMenus[szCategory] = pNewMenu;

  return pNewMenu;
}

void ezQtTypeMenu::OnMenuAction()
{
  const ezRTTI* pRtti = static_cast<const ezRTTI*>(sender()->property("type").value<void*>());

  OnMenuAction(pRtti);
}

void ezQtTypeMenu::OnMenuAction(const ezRTTI* pRtti)
{
  m_pLastSelectedType = pRtti;

  ezQtSearchableMenuRecentList::UseEntry(m_sActiveRecentList, pRtti->GetTypeName());

  Q_EMIT TypeSelected(ezMakeQString(pRtti->GetTypeName()));
}

void ezQtTypeMenu::FillMenu(QMenu* pMenu, const ezRTTI* pBaseType, bool bDerivedTypes, bool bSimpleMenu)
{
  m_pMenu = pMenu;
  m_sActiveRecentList = m_sRecentListName.IsEmpty() ? ezString(pBaseType->GetTypeName()) : m_sRecentListName;

  m_SupportedTypes.Clear();
  m_SupportedTypes.Insert(pBaseType);

  if (bDerivedTypes)
  {
    ezReflectionUtils::GatherTypesDerivedFromClass(pBaseType, m_SupportedTypes);
  }

  // Make category-sorted array of types and skip all abstract, hidden or in development types
  ezDynamicArray<const ezRTTI*> supportedTypes;
  for (const ezRTTI* pRtti : m_SupportedTypes)
  {
    if (pRtti->GetTypeFlags().IsAnySet(ezTypeFlags::Abstract))
      continue;

    if (pRtti->GetAttributeByType<ezHiddenAttribute>() != nullptr)
      continue;

    if (!s_bShowInDevelopmentFeatures && pRtti->GetAttributeByType<ezInDevelopmentAttribute>() != nullptr)
      continue;

    supportedTypes.PushBack(pRtti);
  }
  supportedTypes.Sort(TypeComparer());

  if (!bSimpleMenu && supportedTypes.GetCount() > 10)
  {
    // only show a searchable menu when it makes some sense
    // also deactivating entries to prevent duplicates is currently not supported by the searchable menu
    m_pSearchableMenu = new ezQtSearchableMenu(m_pMenu);
  }

  ezStringBuilder sIconName;
  ezStringBuilder sCategory = "";

  ezMap<ezString, QMenu*> existingMenus;

  if (m_pSearchableMenu == nullptr)
  {
    // first round: create all sub menus
    for (const ezRTTI* pRtti : supportedTypes)
    {
      // Determine current menu
      const ezCategoryAttribute* pCatA = pRtti->GetAttributeByType<ezCategoryAttribute>();

      if (pCatA)
      {
        CreateCategoryMenu(pCatA->GetCategory(), existingMenus);
      }
    }
  }

  if (m_pSearchableMenu != nullptr)
  {
    // add recently used sub-menu
    {
      ezStringBuilder sInternalPath, sDisplayName;

      ezInt32 iToAdd = 8;

      auto lruList = ezQtSearchableMenuRecentList::GetList(m_sActiveRecentList);
      for (const auto& sTypeName : lruList)
      {
        const ezRTTI* pRtti = ezRTTI::FindTypeByName(sTypeName);

        if (pRtti == nullptr)
          continue;

        if (!pRtti->IsDerivedFrom(pBaseType))
          continue;

        sIconName.Set(":/TypeIcons/", pRtti->GetTypeName(), ".svg");

        sInternalPath.Set(" *** RECENT ***/", pRtti->GetTypeName());

        sDisplayName = ezTranslate(pRtti->GetTypeName());

        const ezCategoryAttribute* pCatA = pRtti->GetAttributeByType<ezCategoryAttribute>();
        const ezColorAttribute* pColA = pRtti->GetAttributeByType<ezColorAttribute>();

        ezColor iconColor = ezColor::MakeZero();

        if (pColA)
        {
          iconColor = pColA->GetColor();
        }
        else if (pCatA && iconColor == ezColor::MakeZero())
        {
          iconColor = ezColorScheme::GetCategoryColor(pCatA->GetCategory(), ezColorScheme::CategoryColorUsage::MenuEntryIcon);
        }

        const QIcon actionIcon = ezQtUiServices::GetCachedIconResource(sIconName.GetData(), iconColor);

        m_pSearchableMenu->AddItem(sDisplayName, sInternalPath, QVariant::fromValue((void*)pRtti), actionIcon);

        if (--iToAdd <= 0)
          break;
      }
    }
  }

  ezStringBuilder tmp;

  // second round: create the actions
  for (const ezRTTI* pRtti : supportedTypes)
  {
    sIconName.Set(":/TypeIcons/", pRtti->GetTypeName(), ".svg");

    // Determine current menu
    const ezCategoryAttribute* pCatA = pRtti->GetAttributeByType<ezCategoryAttribute>();
    const ezInDevelopmentAttribute* pInDev = pRtti->GetAttributeByType<ezInDevelopmentAttribute>();
    const ezColorAttribute* pColA = pRtti->GetAttributeByType<ezColorAttribute>();

    ezColor iconColor = ezColor::MakeZero();

    if (pColA)
    {
      iconColor = pColA->GetColor();
    }
    else if (pCatA && iconColor == ezColor::MakeZero())
    {
      iconColor = ezColorScheme::GetCategoryColor(pCatA->GetCategory(), ezColorScheme::CategoryColorUsage::MenuEntryIcon);
    }

    const QIcon actionIcon = ezQtUiServices::GetCachedIconResource(sIconName.GetData(), iconColor);


    if (m_pSearchableMenu != nullptr)
    {
      ezStringBuilder sFullPath;
      sFullPath = pCatA ? pCatA->GetCategory() : "";
      sFullPath.AppendPath(pRtti->GetTypeName());

      ezStringBuilder sDisplayName = ezTranslate(pRtti->GetTypeName());
      if (pInDev)
      {
        sDisplayName.AppendFormat(" [ {} ]", pInDev->GetString());
      }

      m_pSearchableMenu->AddItem(sDisplayName, sFullPath, QVariant::fromValue((void*)pRtti), actionIcon);
    }
    else
    {
      QMenu* pCat = CreateCategoryMenu(pCatA ? pCatA->GetCategory() : nullptr, existingMenus);

      ezStringBuilder fullName = ezTranslate(pRtti->GetTypeName());

      if (pInDev)
      {
        fullName.AppendFormat(" [ {} ]", pInDev->GetString());
      }

      // Add type action to current menu
      QAction* pAction = new QAction(fullName.GetData(), m_pMenu);
      pAction->setProperty("type", QVariant::fromValue((void*)pRtti));
      EZ_VERIFY(connect(pAction, SIGNAL(triggered()), this, SLOT(OnMenuAction())) != nullptr, "connection failed");

      pAction->setIcon(actionIcon);

      pCat->addAction(pAction);
    }
  }

  if (m_pSearchableMenu != nullptr)
  {
    connect(m_pSearchableMenu, &ezQtSearchableMenu::MenuItemTriggered, m_pMenu, [this](const QString& sName, const QVariant& variant)
      {
        const ezRTTI* pRtti = static_cast<const ezRTTI*>(variant.value<void*>());

        OnMenuAction(pRtti);

        m_pMenu->close();
        //
      });

    connect(m_pSearchableMenu, &ezQtSearchableMenu::SearchTextChanged, m_pMenu,
      [this](const QString& sText)
      { ezQtTypeMenu::s_sLastMenuSearch = sText.toUtf8().data(); });

    m_pMenu->addAction(m_pSearchableMenu);

    // important to do this last to make sure the search bar gets focus
    m_pSearchableMenu->Finalize(ezQtTypeMenu::s_sLastMenuSearch.GetData());
  }
}
