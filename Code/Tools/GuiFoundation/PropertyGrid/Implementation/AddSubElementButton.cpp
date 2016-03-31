#include <GuiFoundation/PCH.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>
#include <QPushButton>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QMenu>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <CoreUtils/Localization/TranslationLookup.h>

ezAddSubElementButton::ezAddSubElementButton()
  : ezQtPropertyWidget()
{
  // Reset base class size policy as we are put in a layout that would cause us to vanish instead.
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pButton = new QPushButton(this);
  m_pButton->setText("Add Item");
  m_pButton->setObjectName("Button");

  QSizePolicy policy = m_pButton->sizePolicy();
  policy.setHorizontalStretch(0);
  m_pButton->setSizePolicy(policy);

  m_pLayout->addSpacerItem(new QSpacerItem(0, 0));
  m_pLayout->setStretch(0, 1);
  m_pLayout->addWidget(m_pButton);
  m_pLayout->addSpacerItem(new QSpacerItem(0, 0));
  m_pLayout->setStretch(2, 1);

  m_pMenu = nullptr;

}

void ezAddSubElementButton::OnInit()
{
  if (m_pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
  {
    m_pMenu = new QMenu(m_pButton);
    m_pMenu->setObjectName("Menu");
    m_pButton->setMenu(m_pMenu);
  }
  QMetaObject::connectSlotsByName(this);
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
      
    return ezStringUtils::Compare(a->GetTypeName(), b->GetTypeName()) < 0;
  }
};

QMenu* ezAddSubElementButton::CreateCategoryMenu(const char* szCategory, ezMap<ezString, QMenu*>& existingMenus)
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

  QMenu* pNewMenu = pParentMenu->addMenu(ezTranslate(sPath.GetData()));
  existingMenus[szCategory] = pNewMenu;

  return pNewMenu;
}

void ezAddSubElementButton::on_Menu_aboutToShow()
{
  if (m_Items.IsEmpty())
    return;

  if (!m_pMenu->isEmpty())
    return;

  auto& acc = m_Items[0].m_pObject->GetTypeAccessor();

  auto pProp = GetProperty();

  if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
  {
    m_SupportedTypes.Clear();
    ezReflectionUtils::GatherTypesDerivedFromClass(pProp->GetSpecificType(), m_SupportedTypes, false);
  }
  m_SupportedTypes.Insert(pProp->GetSpecificType());

  // Make category-sorted array of types
  ezDynamicArray<const ezRTTI*> supportedTypes;
  for (const ezRTTI* pRtti : m_SupportedTypes)
  {
    if (pRtti->GetTypeFlags().IsAnySet(ezTypeFlags::Abstract))
      continue;

    supportedTypes.PushBack(pRtti);
  }
  supportedTypes.Sort(TypeComparer());

  ezStringBuilder sIconName;
  ezStringBuilder sCategory = "";

  ezMap<ezString, QMenu*> existingMenus;

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

  // second round: create the actions
  for (const ezRTTI* pRtti : supportedTypes)
  {
    // Determine current menu
    const ezCategoryAttribute* pCatA = pRtti->GetAttributeByType<ezCategoryAttribute>();

    QMenu* pCat = CreateCategoryMenu(pCatA ? pCatA->GetCategory() : nullptr, existingMenus);

    // Add type action to current menu
    QAction* pAction = new QAction(QString::fromUtf8(ezTranslate(pRtti->GetTypeName())), m_pMenu);
    pAction->setProperty("type", qVariantFromValue((void*)pRtti));
    EZ_VERIFY(connect(pAction, SIGNAL(triggered()), this, SLOT(OnMenuAction())) != nullptr, "connection failed");

    sIconName.Set(":/TypeIcons/", pRtti->GetTypeName());
    pAction->setIcon(ezUIServices::GetCachedIconResource(sIconName.GetData()));

    pCat->addAction(pAction);
  }
}

void ezAddSubElementButton::on_Button_clicked()
{
  auto pProp = GetProperty();

  if (!pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
  {
    OnAction(pProp->GetSpecificType());
  }
}

void ezAddSubElementButton::OnMenuAction()
{
  const ezRTTI* pRtti = static_cast<const ezRTTI*>(sender()->property("type").value<void*>());

  OnAction(pRtti);
}

void ezAddSubElementButton::OnAction(const ezRTTI* pRtti)
{
  EZ_ASSERT_DEV(pRtti != nullptr, "user data retrieval failed");

  ezCommandHistory* history = m_pGrid->GetCommandHistory();
  history->StartTransaction();

  ezStatus res;
  if (GetProperty()->GetFlags().IsSet(ezPropertyFlags::StandardType))
  {
    ezInsertObjectPropertyCommand cmd;
    cmd.SetPropertyPath(m_PropertyPath.GetPathString());
    cmd.m_Index = -1;

    for (auto& item : m_Items)
    {
      cmd.m_Object = item.m_pObject->GetGuid();
      cmd.m_NewValue = ezToolsReflectionUtils::GetDefaultValue(GetProperty());

      res = history->AddCommand(cmd);
      if (res.m_Result.Failed())
        break;
    }
  }
  else
  {
    ezAddObjectCommand cmd;
    cmd.m_pType = pRtti;
    cmd.m_sParentProperty = m_PropertyPath.GetPathString();
    // We don't set the index field for member pointers
    if (GetProperty()->GetCategory() != ezPropertyCategory::Member)
      cmd.m_Index = -1;

    for (auto& item : m_Items)
    {
      cmd.m_Parent = item.m_pObject->GetGuid();
      res = history->AddCommand(cmd);
      if (res.m_Result.Failed())
        break;
    }
  }

  if (res.m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  ezUIServices::GetSingleton()->MessageBoxStatus(res, "Adding sub-element to the property failed.");
}


