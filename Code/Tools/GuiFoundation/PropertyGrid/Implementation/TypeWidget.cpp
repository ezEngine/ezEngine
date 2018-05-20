#include <PCH.h>
#include <GuiFoundation/PropertyGrid/Implementation/TypeWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <GuiFoundation/PropertyGrid/Implementation/ManipulatorLabel.moc.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <QGridLayout>
#include <QLabel>

ezQtTypeWidget::ezQtTypeWidget(QWidget* pParent, ezQtPropertyGridWidget* pGrid, ezObjectAccessorBase* pObjectAccessor, const ezRTTI* pType)
  : QWidget(pParent)
  , m_pGrid(pGrid)
  , m_pObjectAccessor(pObjectAccessor)
  , m_pType(pType)
{
  EZ_ASSERT_DEBUG(m_pGrid && m_pObjectAccessor && m_pType, "");
  m_pLayout = new QGridLayout(this);
  m_pLayout->setColumnStretch(0, 1);
  m_pLayout->setColumnStretch(1, 0);
  m_pLayout->setColumnMinimumWidth(1, 5);
  m_pLayout->setColumnStretch(2, 2);
  m_pLayout->setMargin(0);
  m_pLayout->setSpacing(0);
  setLayout(m_pLayout);

  m_pGrid->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtTypeWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtTypeWidget::CommandHistoryEventHandler, this));
  ezManipulatorManager::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtTypeWidget::ManipulatorManagerEventHandler, this));

  BuildUI(pType);
}

ezQtTypeWidget::~ezQtTypeWidget()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtTypeWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtTypeWidget::CommandHistoryEventHandler, this));
  ezManipulatorManager::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtTypeWidget::ManipulatorManagerEventHandler, this));
}

void ezQtTypeWidget::SetSelection(const ezHybridArray<ezPropertySelection, 8>& items)
{
  ezQtScopedUpdatesDisabled _(this);

  m_Items = items;

  for (auto it = m_PropertyWidgets.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_pWidget->SetSelection(m_Items);

    if (it.Value().m_pLabel)
    {
      it.Value().m_pLabel->SetSelection(m_Items);
    }
  }

  UpdatePropertyMetaState();

  for (auto it = m_PropertyWidgets.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_pLabel)
      it.Value().m_pLabel->SetSelection(m_Items);
  }

  ezManipulatorManagerEvent e;
  e.m_pDocument = m_pGrid->GetDocument();
  e.m_pManipulator = ezManipulatorManager::GetSingleton()->GetActiveManipulator(e.m_pDocument, e.m_pSelection);
  e.m_bHideManipulators = false; // irrelevant for this
  ManipulatorManagerEventHandler(e);
}


void ezQtTypeWidget::PrepareToDie()
{
  if (!m_bUndead)
  {
    m_bUndead = true;
    for (auto it = m_PropertyWidgets.GetIterator(); it.IsValid(); ++it)
    {
      it.Value().m_pWidget->PrepareToDie();
    }
  }
}

void ezQtTypeWidget::BuildUI(const ezRTTI* pType, const ezMap<ezString, const ezManipulatorAttribute*>& manipulatorMap)
{
  ezQtScopedUpdatesDisabled _(this);

  const ezRTTI* pParentType = pType->GetParentType();
  if (pParentType != nullptr)
    BuildUI(pParentType, manipulatorMap);

  ezUInt32 iRows = m_pLayout->rowCount();
  for (ezUInt32 i = 0; i < pType->GetProperties().GetCount(); ++i)
  {
    const ezAbstractProperty* pProp = pType->GetProperties()[i];

    if (pProp->GetFlags().IsSet(ezPropertyFlags::Hidden))
      continue;

    if (pProp->GetAttributeByType<ezHiddenAttribute>() != nullptr)
      continue;

    if (pProp->GetSpecificType()->GetAttributeByType<ezHiddenAttribute>() != nullptr)
      continue;

    if (pProp->GetCategory() == ezPropertyCategory::Constant)
      continue;

    ezQtPropertyWidget* pNewWidget = ezQtPropertyGridWidget::CreatePropertyWidget(pProp);
    EZ_ASSERT_DEV(pNewWidget != nullptr, "No property editor defined for '{0}'", pProp->GetPropertyName());
    pNewWidget->setParent(this);
    pNewWidget->Init(m_pGrid, m_pObjectAccessor, pType, pProp);
    auto& ref = m_PropertyWidgets[pProp->GetPropertyName()];

    ref.m_pWidget = pNewWidget;
    ref.m_pLabel = nullptr;

    if (pNewWidget->HasLabel())
    {
      ezQtManipulatorLabel* pLabel = new ezQtManipulatorLabel(this);
      pLabel->setText(QString::fromUtf8(pNewWidget->GetLabel()));
      pLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
      pLabel->setContentsMargins(0, 0, 0, 0); // 18 is a hacked value to align label with group boxes.
      pLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

      connect(pLabel, &QWidget::customContextMenuRequested, pNewWidget, &ezQtPropertyWidget::OnCustomContextMenu);

      m_pLayout->addWidget(pLabel, iRows + i, 0, 1, 1);
      m_pLayout->addWidget(pNewWidget, iRows + i, 2, 1, 1);

      auto itManip = manipulatorMap.Find(pProp->GetPropertyName());
      if (itManip.IsValid())
      {
        pLabel->SetManipulator(itManip.Value());
      }

      ref.m_pLabel = pLabel;
      ref.m_sOriginalLabelText = pNewWidget->GetLabel();
    }
    else
    {
      m_pLayout->addWidget(pNewWidget, iRows + i, 0, 1, 3);
    }
  }
}


void ezQtTypeWidget::BuildUI(const ezRTTI* pType)
{
  ezMap<ezString, const ezManipulatorAttribute*> manipulatorMap;

  const ezRTTI* pParentType = pType;
  while (pParentType != nullptr)
  {
    const auto& attr = pParentType->GetAttributes();

    for (ezPropertyAttribute* pAttr : attr)
    {
      if (pAttr->GetDynamicRTTI()->IsDerivedFrom<ezManipulatorAttribute>())
      {
        const ezManipulatorAttribute* pManipAttr = static_cast<const ezManipulatorAttribute*>(pAttr);

        if (!pManipAttr->m_sProperty1.IsEmpty())
          manipulatorMap[pManipAttr->m_sProperty1] = pManipAttr;
        if (!pManipAttr->m_sProperty2.IsEmpty())
          manipulatorMap[pManipAttr->m_sProperty2] = pManipAttr;
        if (!pManipAttr->m_sProperty3.IsEmpty())
          manipulatorMap[pManipAttr->m_sProperty3] = pManipAttr;
        if (!pManipAttr->m_sProperty4.IsEmpty())
          manipulatorMap[pManipAttr->m_sProperty4] = pManipAttr;
        if (!pManipAttr->m_sProperty5.IsEmpty())
          manipulatorMap[pManipAttr->m_sProperty5] = pManipAttr;
        if (!pManipAttr->m_sProperty6.IsEmpty())
          manipulatorMap[pManipAttr->m_sProperty6] = pManipAttr;
      }
    }

    pParentType = pParentType->GetParentType();
  }

  BuildUI(pType, manipulatorMap);
}

void ezQtTypeWidget::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (m_bUndead)
    return;

  UpdateProperty(e.m_pObject, e.m_sProperty);
}

void ezQtTypeWidget::CommandHistoryEventHandler(const ezCommandHistoryEvent& e)
{
  if (m_bUndead)
    return;

  switch (e.m_Type)
  {
  case ezCommandHistoryEvent::Type::UndoEnded:
  case ezCommandHistoryEvent::Type::RedoEnded:
  case ezCommandHistoryEvent::Type::TransactionEnded:
  case ezCommandHistoryEvent::Type::TransactionCanceled:
    {
      FlushQueuedChanges();
    }
    break;

  default:
    break;
  }
}


void ezQtTypeWidget::ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e)
{
  if (m_bUndead)
    return;

  if (m_pGrid->GetDocument() != e.m_pDocument)
    return;

  const bool bActiveOnThis = (e.m_pSelection != nullptr) && (m_Items == *e.m_pSelection);

  for (auto it = m_PropertyWidgets.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_pLabel)
    {
      if (bActiveOnThis && e.m_pManipulator == it.Value().m_pLabel->GetManipulator())
      {
        it.Value().m_pLabel->SetManipulatorActive(true);
      }
      else
      {
        it.Value().m_pLabel->SetManipulatorActive(false);
      }
    }
  }

}

void ezQtTypeWidget::UpdateProperty(const ezDocumentObject* pObject, const ezString& sProperty)
{
  if (std::none_of(cbegin(m_Items), cend(m_Items),
                   [=](const ezPropertySelection& sel) { return pObject == sel.m_pObject; }
  ))
    return;


  if (!m_QueuedChanges.Contains(sProperty))
  {
    m_QueuedChanges.PushBack(sProperty);
  }

  // In case the change happened outside the command history we have to update at once.
  if (!m_pGrid->GetCommandHistory()->IsInTransaction() && !m_pGrid->GetCommandHistory()->IsInUndoRedo())
    FlushQueuedChanges();
}

void ezQtTypeWidget::FlushQueuedChanges()
{
  for (const ezString& sProperty : m_QueuedChanges)
  {
    for (auto it = m_PropertyWidgets.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Key().StartsWith(sProperty))
      {
        ezQtScopedUpdatesDisabled _(this);
        it.Value().m_pWidget->SetSelection(m_Items);
        break;
      }
    }
  }

  m_QueuedChanges.Clear();

  UpdatePropertyMetaState();
}

void ezQtTypeWidget::UpdatePropertyMetaState()
{
  ezPropertyMetaState* pMeta = ezPropertyMetaState::GetSingleton();

  ezMap<ezString, ezPropertyUiState> PropertyStates;
  pMeta->GetPropertyState(m_Items, PropertyStates);

  for (auto it = m_PropertyWidgets.GetIterator(); it.IsValid(); ++it)
  {
    auto itData = PropertyStates.Find(it.Key());

    const bool bReadOnly = (it.Value().m_pWidget->GetProperty()->GetFlags().IsSet(ezPropertyFlags::ReadOnly)) || (it.Value().m_pWidget->GetProperty()->GetAttributeByType<ezReadOnlyAttribute>() != nullptr);
    ezPropertyUiState::Visibility state = ezPropertyUiState::Default;
    bool bIsDefaultValue = true;
    if (itData.IsValid())
    {
      state = itData.Value().m_Visibility;
      bIsDefaultValue = itData.Value().m_bIsDefaultValue;
    }

    if (it.Value().m_pLabel)
    {
      it.Value().m_pLabel->setVisible(state != ezPropertyUiState::Invisible);
      it.Value().m_pLabel->setEnabled(!bReadOnly && state != ezPropertyUiState::Disabled);
      it.Value().m_pLabel->SetIsDefault(bIsDefaultValue);

      if (itData.IsValid() && !itData.Value().m_sNewLabelText.IsEmpty())
      {
        const char* szLabelText = itData.Value().m_sNewLabelText;
        it.Value().m_pLabel->setText(QString::fromUtf8(ezTranslate(szLabelText)));
        it.Value().m_pLabel->setToolTip(QString::fromUtf8(ezTranslateTooltip(szLabelText)));
      }
      else
      {
        bool temp = ezTranslatorLogMissing::s_bActive;
        ezTranslatorLogMissing::s_bActive = false;

        // unless there is a specific override, we want to show the exact property name
        // also we don't want to force people to add translations for each and every property name
        it.Value().m_pLabel->setText(QString::fromUtf8(ezTranslate(it.Value().m_sOriginalLabelText.GetData())));

        // though do try to get a tooltip for the property
        // this will not log an error message, if the string is not translated
        it.Value().m_pLabel->setToolTip(QString::fromUtf8(ezTranslateTooltip(it.Value().m_sOriginalLabelText.GetData())));

        ezTranslatorLogMissing::s_bActive = temp;
      }
    }

    it.Value().m_pWidget->setVisible(state != ezPropertyUiState::Invisible);
    it.Value().m_pWidget->setEnabled(!bReadOnly && state != ezPropertyUiState::Disabled);
    it.Value().m_pWidget->SetIsDefault(bIsDefaultValue);
  }
}

