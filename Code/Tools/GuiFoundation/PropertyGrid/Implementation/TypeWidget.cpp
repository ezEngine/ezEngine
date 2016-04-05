#include <GuiFoundation/PCH.h>
#include <GuiFoundation/PropertyGrid/Implementation/TypeWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

#include <QGridLayout>
#include <QLabel>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <GuiFoundation/PropertyGrid/Implementation/ManipulatorLabel.moc.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <CoreUtils/Localization/TranslationLookup.h>

ezTypeWidget::ezTypeWidget(QWidget* pParent, ezPropertyGridWidget* pGrid, const ezRTTI* pType, ezPropertyPath& parentPath)
  : QWidget(pParent)
{
  m_pGrid = pGrid;
  m_pType = pType;
  m_ParentPath = parentPath;
  m_pLayout = new QGridLayout(this);
  m_pLayout->setColumnStretch(0, 1);
  m_pLayout->setColumnStretch(1, 0);
  m_pLayout->setColumnMinimumWidth(1, 5);
  m_pLayout->setColumnStretch(2, 2);
  m_pLayout->setMargin(0);
  m_pLayout->setSpacing(0);
  setLayout(m_pLayout);

  m_pGrid->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezTypeWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezTypeWidget::CommandHistoryEventHandler, this));
  ezManipulatorManager::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezTypeWidget::ManipulatorManagerEventHandler, this));

  ezPropertyPath ParentPath = m_ParentPath;

  BuildUI(pType, ParentPath);
}

ezTypeWidget::~ezTypeWidget()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezTypeWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezTypeWidget::CommandHistoryEventHandler, this));
  ezManipulatorManager::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezTypeWidget::ManipulatorManagerEventHandler, this));
}

void ezTypeWidget::SetSelection(const ezHybridArray<ezQtPropertyWidget::Selection, 8>& items)
{
  QtScopedUpdatesDisabled _(this);

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
  ManipulatorManagerEventHandler(e);
}

void ezTypeWidget::BuildUI(const ezRTTI* pType, ezPropertyPath& ParentPath, const ezMap<ezString, const ezManipulatorAttribute*>& manipulatorMap)
{
  QtScopedUpdatesDisabled _(this);

  const ezRTTI* pParentType = pType->GetParentType();
  if (pParentType != nullptr)
    BuildUI(pParentType, ParentPath, manipulatorMap);

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

    ParentPath.PushBack(pProp->GetPropertyName());

    ezQtPropertyWidget* pNewWidget = ezPropertyGridWidget::CreatePropertyWidget(pProp);
    EZ_ASSERT_DEV(pNewWidget != nullptr, "No property editor defined for '%s'", pProp->GetPropertyName());
    pNewWidget->setParent(this);
    pNewWidget->Init(m_pGrid, pProp, ParentPath);

    pNewWidget->m_Events.AddEventHandler(ezMakeDelegate(&ezTypeWidget::PropertyChangedHandler, this));
    auto& ref = m_PropertyWidgets[ParentPath.GetPathString()];

    ref.m_pWidget = pNewWidget;
    ref.m_pLabel = nullptr;

    if (pNewWidget->HasLabel())
    {
      ezManipulatorLabel* pLabel = new ezManipulatorLabel(this);
      pLabel->setText(QString::fromUtf8(pNewWidget->GetLabel()));
      pLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
      pLabel->setContentsMargins(18, 0, 0, 0); // 18 is a hacked value to align label with group boxes.
      pLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
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

    ParentPath.PopBack();
  }
}


void ezTypeWidget::BuildUI(const ezRTTI* pType, ezPropertyPath& ParentPath)
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
      }
    }

    pParentType = pParentType->GetParentType();
  }

  BuildUI(pType, ParentPath, manipulatorMap);
}

void ezTypeWidget::PropertyChangedHandler(const ezQtPropertyWidget::Event& ed)
{
  switch (ed.m_Type)
  {
  case  ezQtPropertyWidget::Event::Type::ValueChanged:
    {
      ezSetObjectPropertyCommand cmd;
      cmd.m_NewValue = ed.m_Value;
      cmd.SetPropertyPath(ed.m_pPropertyPath->GetPathString());

      m_pGrid->GetDocument()->GetCommandHistory()->StartTransaction();

      ezStatus res;

      for (const auto& sel : *ed.m_pItems)
      {
        cmd.m_Object = sel.m_pObject->GetGuid();
        cmd.m_Index = sel.m_Index;

        res = m_pGrid->GetDocument()->GetCommandHistory()->AddCommand(cmd);

        if (res.m_Result.Failed())
          break;
      }

      if (res.m_Result.Failed())
        m_pGrid->GetDocument()->GetCommandHistory()->CancelTransaction();
      else
        m_pGrid->GetDocument()->GetCommandHistory()->FinishTransaction();

      ezUIServices::GetSingleton()->MessageBoxStatus(res, "Changing the property failed.");

    }
    break;

  case  ezQtPropertyWidget::Event::Type::BeginTemporary:
    {
      m_pGrid->GetDocument()->GetCommandHistory()->BeginTemporaryCommands();
    }
    break;

  case  ezQtPropertyWidget::Event::Type::EndTemporary:
    {
      m_pGrid->GetDocument()->GetCommandHistory()->FinishTemporaryCommands();
    }
    break;

  case  ezQtPropertyWidget::Event::Type::CancelTemporary:
    {
      m_pGrid->GetDocument()->GetCommandHistory()->CancelTemporaryCommands();
    }
    break;
  }
}

void ezTypeWidget::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  UpdateProperty(e.m_pObject, e.m_sPropertyPath);
}

void ezTypeWidget::CommandHistoryEventHandler(const ezCommandHistoryEvent& e)
{
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
  }
}


void ezTypeWidget::ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e)
{
  if (m_pGrid->GetDocument() != e.m_pDocument)
    return;

  bool bActiveOnThis = (e.m_pSelection != nullptr) && (m_Items == *e.m_pSelection);
  
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

void ezTypeWidget::UpdateProperty(const ezDocumentObject* pObject, const ezString& sProperty)
{
  if (std::none_of(cbegin(m_Items), cend(m_Items),
                   [=](const ezQtPropertyWidget::Selection& sel) { return pObject == sel.m_pObject; }
                   ))
    return;

  if (!m_QueuedChanges.Contains(sProperty))
  {
    m_QueuedChanges.PushBack(sProperty);
  }
}

void ezTypeWidget::FlushQueuedChanges()
{
  for (const ezString& sProperty : m_QueuedChanges)
  {
    for (auto it = m_PropertyWidgets.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Key().StartsWith(sProperty))
      {
        QtScopedUpdatesDisabled _(this);
        it.Value().m_pWidget->SetSelection(m_Items);
        break;
      }
    }
  }

  m_QueuedChanges.Clear();

  UpdatePropertyMetaState();
}

void ezTypeWidget::UpdatePropertyMetaState()
{
  ezPropertyMetaState* pMeta = ezPropertyMetaState::GetSingleton();

  ezMap<ezString, ezPropertyUiState> PropertyStates;
  pMeta->GetPropertyState(m_Items, PropertyStates);

  for (auto it = m_PropertyWidgets.GetIterator(); it.IsValid(); ++it)
  {
    auto itData = PropertyStates.Find(it.Key());

    const bool bReadOnly = (it.Value().m_pWidget->GetProperty()->GetFlags().IsSet(ezPropertyFlags::ReadOnly)) || (it.Value().m_pWidget->GetProperty()->GetAttributeByType<ezReadOnlyAttribute>() != nullptr);
    ezPropertyUiState::Visibility state = ezPropertyUiState::Default;

    if (itData.IsValid())
    {
      state = itData.Value().m_Visibility;
    }

    if (it.Value().m_pLabel)
    {
      it.Value().m_pLabel->setVisible(state != ezPropertyUiState::Invisible);
      it.Value().m_pLabel->setEnabled(!bReadOnly && state != ezPropertyUiState::Disabled);

      if (itData.IsValid() && !itData.Value().m_sNewLabelText.IsEmpty())
      {
        const char* szLabelText = itData.Value().m_sNewLabelText;
        it.Value().m_pLabel->setText(QString::fromUtf8(ezTranslate(szLabelText)));
        it.Value().m_pLabel->setToolTip(QString::fromUtf8(ezTranslateTooltip(szLabelText)));
      }
      else
      {
        // do NOT translate the first string
        // unless there is a specific override, we want to show the exact property name
        // also we don't want to force people to add translations for each and every property name
        it.Value().m_pLabel->setText(QString::fromUtf8(it.Value().m_sOriginalLabelText.GetData()));

        // though do try to get a tooltip for the property
        // this will not log an error message, if the string is not translated
        it.Value().m_pLabel->setToolTip(QString::fromUtf8(ezTranslateTooltip(it.Value().m_sOriginalLabelText.GetData())));
      }
    }

    it.Value().m_pWidget->setVisible(state != ezPropertyUiState::Invisible);
    it.Value().m_pWidget->setEnabled(!bReadOnly && state != ezPropertyUiState::Disabled);
  }
}

