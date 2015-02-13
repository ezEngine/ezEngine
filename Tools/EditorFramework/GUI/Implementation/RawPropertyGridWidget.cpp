#include <PCH.h>
#include <EditorFramework/GUI/RawPropertyGridWidget.h>
#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>
#include <EditorFramework/EditorGUI.moc.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <QGroupBox>
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

  m_pGroups[0] = nullptr;
  m_pGroups[1] = nullptr;
  m_pSpacer = nullptr;

  m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(ezDelegate<void (const ezSelectionManager::Event&)>(&ezRawPropertyGridWidget::SelectionEventHandler, this));
  m_pDocument->GetObjectTree()->m_PropertyEvents.AddEventHandler(ezDelegate<void (const ezDocumentObjectTreePropertyEvent&)>(&ezRawPropertyGridWidget::PropertyEventHandler, this));
}

ezRawPropertyGridWidget::~ezRawPropertyGridWidget()
{
  m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(ezDelegate<void (const ezSelectionManager::Event&)>(&ezRawPropertyGridWidget::SelectionEventHandler, this));
  m_pDocument->GetObjectTree()->m_PropertyEvents.RemoveEventHandler(ezDelegate<void (const ezDocumentObjectTreePropertyEvent&)>(&ezRawPropertyGridWidget::PropertyEventHandler, this));
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

void ezRawPropertyGridWidget::PropertyEventHandler(const ezDocumentObjectTreePropertyEvent& e)
{
  if (m_Selection.IndexOf(e.m_pObject) == ezInvalidIndex)
    return;

  // TODO Multi-selection
  if (m_Selection[0] != e.m_pObject)
    return;

  m_pRawPropertyWidget[e.m_bEditorProperty ? 0 : 1]->ChangePropertyValue(e.m_sPropertyPath, e.m_NewValue);
}

void ezRawPropertyGridWidget::PropertyChangedHandler(const ezPropertyEditorBaseWidget::Event& ed, bool bEditor)
{
  switch (ed.m_Type)
  {
  case  ezPropertyEditorBaseWidget::Event::Type::ValueChanged:
    {
      ezSetObjectPropertyCommand cmd;
      cmd.m_bEditorProperty = bEditor;
      cmd.m_Object = m_Selection[0]->GetGuid(); // TODO: Multi selection
      cmd.m_NewValue = ed.m_Value;
      cmd.SetPropertyPath(ezToolsReflectionUtils::GetStringFromPropertyPath(*ed.m_pPropertyPath));


      m_pDocument->GetCommandHistory()->StartTransaction();
      ezStatus res = m_pDocument->GetCommandHistory()->AddCommand(cmd);

      ezEditorGUI::GetInstance()->MessageBoxStatus(res, "Changing the property failed.");

      m_pDocument->GetCommandHistory()->EndTransaction(res.m_Result.Failed());

      if (res.m_Result.Failed())
      {
        // reset the UI to the old value

        const ezIReflectedTypeAccessor& accessor = bEditor ? m_Selection[0]->GetEditorTypeAccessor() : m_Selection[0]->GetTypeAccessor();
        const ezVariant OldValue = accessor.GetValue(cmd.GetPropertyPath());

        m_pRawPropertyWidget[bEditor ? 0 : 1]->ChangePropertyValue(cmd.GetPropertyPath(), OldValue);
      }
    }
    break;

  case  ezPropertyEditorBaseWidget::Event::Type::BeginTemporary:
    {
      m_pDocument->GetCommandHistory()->BeginTemporaryCommands();
    }
    break;

  case  ezPropertyEditorBaseWidget::Event::Type::EndTemporary:
    {
      m_pDocument->GetCommandHistory()->EndTemporaryCommands(false);
    }
    break;

  case  ezPropertyEditorBaseWidget::Event::Type::CancelTemporary:
    {
      m_pDocument->GetCommandHistory()->EndTemporaryCommands(true);
    }
    break;

  }

  /// ..asdfsdf
}

void ezRawPropertyGridWidget::EditorPropertyChangedHandler(const ezPropertyEditorBaseWidget::Event& ed)
{
  PropertyChangedHandler(ed, true);
}

void ezRawPropertyGridWidget::ObjectPropertyChangedHandler(const ezPropertyEditorBaseWidget::Event& ed)
{
  PropertyChangedHandler(ed, false);
}

void ezRawPropertyGridWidget::ClearSelection()
{
  if (m_pSpacer)
    m_pLayout->removeItem(m_pSpacer);

  delete m_pGroups[0];
  delete m_pGroups[1];
  delete m_pSpacer;

  m_pGroups[0] = nullptr;
  m_pGroups[1] = nullptr;
  m_pSpacer = nullptr;
  m_pRawPropertyWidget[0] = nullptr;
  m_pRawPropertyWidget[1] = nullptr;

  m_Selection.Clear();
}

void ezRawPropertyGridWidget::SetSelection(const ezDeque<const ezDocumentObjectBase*>& selection)
{
  ClearSelection();

  m_Selection = selection;

  if (m_Selection.IsEmpty())
    return;

  {
    m_pGroups[0] = new QGroupBox(m_pMainContent);
    m_pGroups[1] = new QGroupBox(m_pMainContent);

    m_pGroups[0]->setTitle(QLatin1String("Editor Properties"));

    ezStringBuilder sName;
    sName.Format("Object Properties [%s]", m_Selection[0]->GetTypeAccessor().GetReflectedTypeHandle().GetType()->GetTypeName().GetData());
    m_pGroups[1]->setTitle(QString::fromUtf8(sName));


    m_pLayout->addWidget(m_pGroups[0]);
    m_pLayout->addWidget(m_pGroups[1]);

    QVBoxLayout* pLayout0 = new QVBoxLayout(m_pGroups[0]);
    pLayout0->setSpacing(1);
    m_pGroups[0]->setLayout(pLayout0);

    QVBoxLayout* pLayout1 = new QVBoxLayout(m_pGroups[1]);
    pLayout1->setSpacing(1);
    m_pGroups[1]->setLayout(pLayout1);

    m_pRawPropertyWidget[0] = new ezRawPropertyWidget(m_pGroups[0], m_Selection[0]->GetEditorTypeAccessor());
    m_pRawPropertyWidget[1] = new ezRawPropertyWidget(m_pGroups[1], m_Selection[0]->GetTypeAccessor());

    pLayout0->addWidget(m_pRawPropertyWidget[0]);
    pLayout1->addWidget(m_pRawPropertyWidget[1]);
  }

   // TODO: Multi selection
  m_pRawPropertyWidget[1]->m_PropertyChanged.AddEventHandler(ezDelegate<void (const ezPropertyEditorBaseWidget::Event&)> (&ezRawPropertyGridWidget::ObjectPropertyChangedHandler, this));
  m_pRawPropertyWidget[0]->m_PropertyChanged.AddEventHandler(ezDelegate<void (const ezPropertyEditorBaseWidget::Event&)> (&ezRawPropertyGridWidget::EditorPropertyChangedHandler, this));

  m_pSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
  

  m_pLayout->addSpacerItem(m_pSpacer);
}

