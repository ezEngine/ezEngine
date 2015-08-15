#include <PCH.h>
#include <EditorFramework/GUI/RawPropertyGridWidget.h>
#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/GUI/CollapsibleGroupBox.moc.h>
#include <QScrollArea>

ezRawPropertyGridWidget::ezRawPropertyGridWidget(ezDocumentBase* pDocument, QWidget* pParent) : QWidget(pParent), m_pDocument(pDocument)
{
  QScrollArea* pScroll = new QScrollArea(this);
  pScroll->setContentsMargins(0, 0, 0, 0);

  QVBoxLayout* pLayout2 = new QVBoxLayout(this);
  pLayout2->setSpacing(1);
  pLayout2->setMargin(1);
  this->setLayout(pLayout2);
  pLayout2->addWidget(pScroll);

  m_pMainContent = new QWidget(this);
  pScroll->setWidget(m_pMainContent);
  pScroll->setWidgetResizable(true);
  m_pMainContent->setBackgroundRole(QPalette::ColorRole::Background);

  m_pLayout = new QVBoxLayout(m_pMainContent);
  m_pLayout->setSpacing(1);
  m_pMainContent->setLayout(m_pLayout);

  m_pGroups = nullptr;
  m_pSpacer = nullptr;

  m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezRawPropertyGridWidget::SelectionEventHandler, this));
  m_pDocument->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezRawPropertyGridWidget::PropertyEventHandler, this));
  m_pDocument->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezRawPropertyGridWidget::StructureEventHandler, this));
}

ezRawPropertyGridWidget::~ezRawPropertyGridWidget()
{
  m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezRawPropertyGridWidget::SelectionEventHandler, this));
  m_pDocument->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezRawPropertyGridWidget::PropertyEventHandler, this));
  m_pDocument->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezRawPropertyGridWidget::StructureEventHandler, this));
}

void ezRawPropertyGridWidget::SelectionEventHandler(const ezSelectionManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezSelectionManager::Event::Type::SelectionCleared:
    ClearSelection();
    break;
  case ezSelectionManager::Event::Type::SelectionSet:
  case ezSelectionManager::Event::Type::ObjectAdded:
  case ezSelectionManager::Event::Type::ObjectRemoved:
    {
      SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
    }
    break;
  }
}

void ezRawPropertyGridWidget::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (m_Selection.IndexOf(e.m_pObject) == ezInvalidIndex)
    return;

  // TODO Multi-selection
  if (m_Selection[0] != e.m_pObject)
    return;

  m_pRawPropertyWidget->ChangePropertyValue(e.m_sPropertyPath, e.m_NewValue);
}

void ezRawPropertyGridWidget::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
  case ezDocumentObjectStructureEvent::Type::AfterObjectMoved:
  case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
    break;
  }
}

void ezRawPropertyGridWidget::PropertyChangedHandler(const ezPropertyEditorBaseWidget::Event& ed)
{
  switch (ed.m_Type)
  {
  case  ezPropertyEditorBaseWidget::Event::Type::ValueChanged:
    {
      ezSetObjectPropertyCommand cmd;
      cmd.m_NewValue = ed.m_Value;
      cmd.SetPropertyPath(ed.m_pPropertyPath->GetPathString());

      m_pDocument->GetCommandHistory()->StartTransaction();

      ezStatus res;

      for (const auto& sel : *ed.m_pItems)
      {
        cmd.m_Object = sel.m_pObject->GetGuid();

        res = m_pDocument->GetCommandHistory()->AddCommand(cmd);

        if (res.m_Result.Failed())
          break;
      }

      if (res.m_Result.Failed())
        m_pDocument->GetCommandHistory()->CancelTransaction();
      else
        m_pDocument->GetCommandHistory()->FinishTransaction();

      ezUIServices::GetInstance()->MessageBoxStatus(res, "Changing the property failed.");

    }
    break;

  case  ezPropertyEditorBaseWidget::Event::Type::BeginTemporary:
    {
      m_pDocument->GetCommandHistory()->BeginTemporaryCommands();
    }
    break;

  case  ezPropertyEditorBaseWidget::Event::Type::EndTemporary:
    {
      m_pDocument->GetCommandHistory()->FinishTemporaryCommands();
    }
    break;

  case  ezPropertyEditorBaseWidget::Event::Type::CancelTemporary:
    {
      m_pDocument->GetCommandHistory()->CancelTemporaryCommands();
    }
    break;

  }

  /// \todo ..asdfsdf (not sure what this means, though)
}

void ezRawPropertyGridWidget::ClearSelection()
{
  if (m_pSpacer)
    m_pLayout->removeItem(m_pSpacer);

  delete m_pGroups;
  delete m_pSpacer;

  m_pGroups = nullptr;
  m_pSpacer = nullptr;
  m_pRawPropertyWidget = nullptr;

  m_Selection.Clear();
}

void ezRawPropertyGridWidget::SetSelection(const ezDeque<const ezDocumentObjectBase*>& selection)
{
  ClearSelection();

  m_Selection = selection;

  if (m_Selection.IsEmpty())
    return;

  {
    m_pGroups = new QWidget(m_pMainContent);

    m_pLayout->addWidget(m_pGroups);

    QVBoxLayout* pLayout1 = new QVBoxLayout();
    pLayout1->setSpacing(1);
    pLayout1->setContentsMargins(5, 0, 0, 0);
    m_pGroups->setLayout(pLayout1);

    ezHybridArray<ezPropertyEditorBaseWidget::Selection, 8> Items;
    Items.Reserve(m_Selection.GetCount());

    for (const auto* sel : m_Selection)
    {
      ezPropertyEditorBaseWidget::Selection s;
      s.m_pObject = sel;

      Items.PushBack(s);
    }

    m_pRawPropertyWidget = new ezRawPropertyWidget(m_pGroups, Items);

    pLayout1->addWidget(m_pRawPropertyWidget);
  }

   // TODO: Multi selection
  m_pRawPropertyWidget->m_PropertyChanged.AddEventHandler(ezMakeDelegate(&ezRawPropertyGridWidget::PropertyChangedHandler, this));

  m_pSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
  

  m_pLayout->addSpacerItem(m_pSpacer);
}

