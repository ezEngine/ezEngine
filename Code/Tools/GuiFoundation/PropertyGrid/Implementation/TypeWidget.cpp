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


  m_pGrid->GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezTypeWidget::PropertyEventHandler, this));

  ezPropertyPath ParentPath = m_ParentPath;
 
  BuildUI(pType, ParentPath);
}

ezTypeWidget::~ezTypeWidget()
{
  m_pGrid->GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezTypeWidget::PropertyEventHandler, this));
}

void ezTypeWidget::SetSelection(const ezHybridArray<ezPropertyBaseWidget::Selection, 8>& items)
{
  QtScopedUpdatesDisabled _(this);

  m_Items = items;

  for (auto it = m_PropertyWidgets.GetIterator(); it.IsValid(); ++it)
  {
    it.Value()->SetSelection(m_Items);
  }
}

void ezTypeWidget::BuildUI(const ezRTTI* pType, ezPropertyPath& ParentPath)
{
  QtScopedUpdatesDisabled _(this);

  const ezRTTI* pParentType = pType->GetParentType();
  if (pParentType != nullptr)
    BuildUI(pParentType, ParentPath);

  ezUInt32 iRows = m_pLayout->rowCount();
  for (ezUInt32 i = 0; i < pType->GetProperties().GetCount(); ++i)
  {
    const ezAbstractProperty* pProp = pType->GetProperties()[i];

    if (pProp->GetFlags().IsSet(ezPropertyFlags::Hidden))
      continue;

    if (pProp->GetAttributeByType<ezHiddenAttribute>() != nullptr)
      continue;

    ParentPath.PushBack(pProp->GetPropertyName());

    ezPropertyBaseWidget* pNewWidget = ezPropertyGridWidget::CreatePropertyWidget(pProp);
    EZ_ASSERT_DEV(pNewWidget != nullptr, "No property editor defined for '%s'", pProp->GetPropertyName());
    pNewWidget->setParent(this);
    pNewWidget->Init(m_pGrid, pProp, ParentPath);

    pNewWidget->m_Events.AddEventHandler(ezMakeDelegate(&ezTypeWidget::PropertyChangedHandler, this));
    m_PropertyWidgets[ParentPath.GetPathString()] = pNewWidget;
    if (pNewWidget->HasLabel())
    {
      QLabel* pLabel = new QLabel(this);
      pLabel->setText(QString::fromUtf8(pNewWidget->GetLabel()));
      pLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
      pLabel->setContentsMargins(18, 0, 0, 0); // 18 is a hacked value to align label with group boxes.
      pLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
      m_pLayout->addWidget(pLabel, iRows + i, 0, 1, 1);
      m_pLayout->addWidget(pNewWidget, iRows + i, 2, 1, 1);
    }
    else
    {
      m_pLayout->addWidget(pNewWidget, iRows + i, 0, 1, 3);
    }

    ParentPath.PopBack();
  }

}

void ezTypeWidget::PropertyChangedHandler(const ezPropertyBaseWidget::Event& ed)
{
  switch (ed.m_Type)
  {
  case  ezPropertyBaseWidget::Event::Type::ValueChanged:
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

      ezUIServices::GetInstance()->MessageBoxStatus(res, "Changing the property failed.");

    }
    break;

  case  ezPropertyBaseWidget::Event::Type::BeginTemporary:
    {
      m_pGrid->GetDocument()->GetCommandHistory()->BeginTemporaryCommands();
    }
    break;

  case  ezPropertyBaseWidget::Event::Type::EndTemporary:
    {
      m_pGrid->GetDocument()->GetCommandHistory()->FinishTemporaryCommands();
    }
    break;

  case  ezPropertyBaseWidget::Event::Type::CancelTemporary:
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

void ezTypeWidget::UpdateProperty(const ezDocumentObjectBase* pObject, const ezString& sProperty)
{
  if (std::none_of(cbegin(m_Items), cend(m_Items),
    [=](const ezPropertyBaseWidget::Selection& sel) { return pObject == sel.m_pObject; }
    ))
    return;

  for (auto it = m_PropertyWidgets.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Key().StartsWith(sProperty))
    {
      QtScopedUpdatesDisabled _(this);
      it.Value()->SetSelection(m_Items);
      break;
    }
  }
}

