#include <PCH.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>
#include <QPushButton>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QMenu>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <QInputDialog>

ezQtAddSubElementButton::ezQtAddSubElementButton()
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

void ezQtAddSubElementButton::OnInit()
{
  if (m_pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
  {
    m_pMenu = new QMenu(m_pButton);
    connect(m_pMenu, &QMenu::aboutToShow, this, &ezQtAddSubElementButton::onMenuAboutToShow);
    m_pButton->setMenu(m_pMenu);
    m_pButton->setObjectName("Button");
  }

  if (const ezMaxArraySizeAttribute* pAttr = m_pProp->GetAttributeByType<ezMaxArraySizeAttribute>())
  {
    m_uiMaxElements = pAttr->GetMaxSize();
  }

  if (const ezPreventDuplicatesAttribute* pAttr = m_pProp->GetAttributeByType<ezPreventDuplicatesAttribute>())
  {
    m_bPreventDuplicates = true;
  }

  m_pConstraint = m_pProp->GetAttributeByType<ezConstrainPointerAttribute>();

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

QMenu* ezQtAddSubElementButton::CreateCategoryMenu(const char* szCategory, ezMap<ezString, QMenu*>& existingMenus)
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

void ezQtAddSubElementButton::onMenuAboutToShow()
{
  if (m_Items.IsEmpty())
    return;

  if (m_pMenu->isEmpty())
  {
    auto pProp = GetProperty();

    if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
    {
      m_SupportedTypes.Clear();
      ezReflectionUtils::GatherTypesDerivedFromClass(pProp->GetSpecificType(), m_SupportedTypes, false);
    }
    m_SupportedTypes.Insert(pProp->GetSpecificType());

    ezVariant constraintValue;
    if (m_pConstraint)
    {
      const ezRTTI* pType = GetCommonBaseType(m_Items);
      if (const ezAbstractProperty* pConstraintValueProp = pType->FindPropertyByName(m_pConstraint->GetConstantValueProperty()))
      {
        if (pConstraintValueProp->GetCategory() == ezPropertyCategory::Constant)
        {
          constraintValue = static_cast<const ezAbstractConstantProperty*>(pConstraintValueProp)->GetConstant();
        }
        else if (pConstraintValueProp->GetCategory() == ezPropertyCategory::Member)
        {
          constraintValue = GetCommonValue(m_Items, pConstraintValueProp);
        }
        else
        {
          ezLog::Error("ezConstrainPointerAttribute set for '{0}' but the constant value property '{1}' has an unsupported type.",
            pType->GetTypeName(), m_pConstraint->GetConstantValueProperty().GetData());
        }
      }
      else
      {
        ezLog::Error("ezConstrainPointerAttribute set for '{0}' but the constant value property '{1}' does not exist.",
          pType->GetTypeName(), m_pConstraint->GetConstantValueProperty().GetData());
      }
    }
    // remove all types that are marked as hidden
    for (auto it = m_SupportedTypes.GetIterator(); it.IsValid(); )
    {
      if (it.Key()->GetAttributeByType<ezHiddenAttribute>() != nullptr)
      {
        it = m_SupportedTypes.Remove(it);
        continue;
      }
      if (m_pConstraint)
      {
        const ezAbstractProperty* pConstraintProp = it.Key()->FindPropertyByName(m_pConstraint->GetConstantName());
        if (!constraintValue.IsValid() || !pConstraintProp || pConstraintProp->GetCategory() != ezPropertyCategory::Constant ||
          static_cast<const ezAbstractConstantProperty*>(pConstraintProp)->GetConstant() != constraintValue)
        {
          it = m_SupportedTypes.Remove(it);
          continue;
        }
      }

      ++it;
    }

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
      pAction->setIcon(ezQtUiServices::GetCachedIconResource(sIconName.GetData()));

      pCat->addAction(pAction);
    }
  }

  if (m_uiMaxElements > 0) // 0 means unlimited
  {
    ezObjectAccessorBase* pObjectAccessor = m_pGrid->GetObjectAccessor();
    QList<QAction*> actions = m_pMenu->actions();

    for (auto& item : m_Items)
    {
      ezInt32 iCount = 0;
      pObjectAccessor->GetCount(item.m_pObject, m_pProp, iCount);

      if (iCount >= (ezInt32)m_uiMaxElements)
      {
        if (!m_bNoMoreElementsAllowed)
        {
          m_bNoMoreElementsAllowed = true;

          QAction* pAction = new QAction(QString("Maximum allowed elements in array is %1").arg(m_uiMaxElements));
          m_pMenu->insertAction(actions.isEmpty() ? nullptr : actions[0], pAction);

          for (auto pAct : actions)
          {
            pAct->setEnabled(false);
          }
        }

        return;
      }
    }

    if (m_bNoMoreElementsAllowed)
    {
      for (auto pAct : actions)
      {
        pAct->setEnabled(true);
      }

      m_bNoMoreElementsAllowed = false;
      delete m_pMenu->actions()[0]; // remove the dummy action
    }
  }

  if (m_bPreventDuplicates)
  {
    ezObjectAccessorBase* pObjectAccessor = m_pGrid->GetObjectAccessor();
    ezSet<const ezRTTI*> UsedTypes;

    for (auto& item : m_Items)
    {
      ezInt32 iCount = 0;
      pObjectAccessor->GetCount(item.m_pObject, m_pProp, iCount);

      for (ezInt32 i = 0; i < iCount; ++i)
      {
        ezUuid guid = pObjectAccessor->Get<ezUuid>(item.m_pObject, m_pProp, i);

        if (guid.IsValid())
        {
          UsedTypes.Insert(pObjectAccessor->GetObject(guid)->GetType());
        }
      }

      QList<QAction*> actions = m_pMenu->actions();
      for (auto pAct : actions)
      {
        const ezRTTI* pRtti = static_cast<const ezRTTI*>(pAct->property("type").value<void*>());

        pAct->setEnabled(!UsedTypes.Contains(pRtti));
      }
    }
  }
}

void ezQtAddSubElementButton::on_Button_clicked()
{
  auto pProp = GetProperty();

  if (!pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
  {
    OnAction(pProp->GetSpecificType());
  }
}

void ezQtAddSubElementButton::OnMenuAction()
{
  const ezRTTI* pRtti = static_cast<const ezRTTI*>(sender()->property("type").value<void*>());

  OnAction(pRtti);
}

void ezQtAddSubElementButton::OnAction(const ezRTTI* pRtti)
{
  EZ_ASSERT_DEV(pRtti != nullptr, "user data retrieval failed");
  ezVariant index = (ezInt32)-1;
  ezObjectAccessorBase* pObjectAccessor = m_pGrid->GetObjectAccessor();
  if (m_pProp->GetCategory() == ezPropertyCategory::Map)
  {
    QString text;
    bool bOk = false;
    while (!bOk)
    {
      text = QInputDialog::getText(this, "Set map key for new element", "Key:", QLineEdit::Normal, text, &bOk);
      if (!bOk)
        return;

      index = text.toUtf8().data();
      for (auto& item : m_Items)
      {
        ezVariant value;
        ezStatus res = pObjectAccessor->GetValue(item.m_pObject, m_pProp, value, index);
        if (res.m_Result.Succeeded())
        {
          bOk = false;
          break;
        }
      }
      if (!bOk)
      {
        ezQtUiServices::GetSingleton()->MessageBoxInformation("The selected key is already used in the selection.");
      }
    }
  }

  pObjectAccessor->StartTransaction("Add Element");

  ezStatus res;
  if (GetProperty()->GetFlags().IsSet(ezPropertyFlags::StandardType))
  {
    for (auto& item : m_Items)
    {
      res = pObjectAccessor->InsertValue(item.m_pObject, m_pProp, ezToolsReflectionUtils::GetDefaultValue(GetProperty()), index);
      if (res.m_Result.Failed())
        break;
    }
  }
  else
  {
    for (auto& item : m_Items)
    {
      ezUuid guid;
      res = pObjectAccessor->AddObject(item.m_pObject, m_pProp, index, pRtti, guid);
      if (res.m_Result.Failed())
        break;
    }
  }

  if (res.m_Result.Failed())
    pObjectAccessor->CancelTransaction();
  else
    pObjectAccessor->FinishTransaction();

  ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Adding sub-element to the property failed.");
}


