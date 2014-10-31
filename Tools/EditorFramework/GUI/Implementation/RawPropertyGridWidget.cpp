#include <PCH.h>
#include <EditorFramework/GUI/RawPropertyGridWidget.h>
#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Document/Document.h>
#include <QPushButton>
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

}

ezRawPropertyGridWidget::~ezRawPropertyGridWidget()
{
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

void ezRawPropertyGridWidget::EditorPropertyChangedHandler(const ezPropertyEditorBaseWidget::EventData& ed)
{
  const_cast<ezIReflectedTypeAccessor&>(m_Selection[0]->GetEditorTypeAccessor()).SetValue(*ed.m_pPropertyPath, ed.m_Value);
}

void ezRawPropertyGridWidget::ObjectPropertyChangedHandler(const ezPropertyEditorBaseWidget::EventData& ed)
{
  const_cast<ezIReflectedTypeAccessor&>(m_Selection[0]->GetTypeAccessor()).SetValue(*ed.m_pPropertyPath, ed.m_Value);
}

void ezRawPropertyGridWidget::BuildUI(const ezIReflectedTypeAccessor& et, const ezReflectedType* pType, ezPropertyPath& ParentPath, QLayout* pLayout, bool bEditorProp)
{
  ezReflectedTypeHandle hParent = pType->GetParentTypeHandle();
  if (!hParent.IsInvalidated())
    BuildUI(et, hParent.GetType(), ParentPath, pLayout, bEditorProp);

  if (pType->GetPropertyCount() == 0)
    return;

  for (ezUInt32 i = 0; i < pType->GetPropertyCount(); ++i)
  {
    const ezReflectedProperty* pProp = pType->GetPropertyByIndex(i);

    if (pProp->m_Flags.IsAnySet(PropertyFlags::IsPOD))
    {
      ParentPath.PushBack(pProp->m_sPropertyName.GetString().GetData());

      ezPropertyEditorBaseWidget* pNewWidget = nullptr;

      switch (pProp->m_Type)
      {
      case ezVariant::Type::Bool:
        pNewWidget = new ezPropertyEditorCheckboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this);
        break;

      case ezVariant::Type::Time:
      case ezVariant::Type::Float:
      case ezVariant::Type::Double:
        pNewWidget = new ezPropertyEditorDoubleSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this);
        break;

      case ezVariant::Type::Int8:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, -127, 127);
        break;
      case ezVariant::Type::UInt8:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, 0, 255);
        break;
      case ezVariant::Type::Int16:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, -32767, 32767);
        break;
      case ezVariant::Type::UInt16:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, 0, 65535);
        break;
      case ezVariant::Type::Int32:
      case ezVariant::Type::Int64:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, -2147483645, 2147483645);
        break;
      case ezVariant::Type::UInt32:
      case ezVariant::Type::UInt64:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, 0, 2147483645);
        break;

      case ezVariant::Type::String:
        pNewWidget = new ezPropertyEditorLineEditWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this);
        break;

      case ezVariant::Type::Color:
        pNewWidget = new ezPropertyEditorColorWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this);
        break;

      default:
        break;
      }

      if (pNewWidget)
      {
        pLayout->addWidget(pNewWidget);
        pNewWidget->SetValue(et.GetValue(ParentPath));

        if (bEditorProp)
          pNewWidget->m_ValueChanged.AddEventHandler(ezDelegate<void (const ezPropertyEditorBaseWidget::EventData&)>(&ezRawPropertyGridWidget::EditorPropertyChangedHandler, this));
        else
          pNewWidget->m_ValueChanged.AddEventHandler(ezDelegate<void (const ezPropertyEditorBaseWidget::EventData&)>(&ezRawPropertyGridWidget::ObjectPropertyChangedHandler, this));
      }

      ParentPath.PopBack();
    }
    else
    {
      ParentPath.PushBack(pProp->m_sPropertyName.GetString().GetData());

      QGroupBox* pSubGroup = new QGroupBox((QWidget*) pLayout->parent());
      pSubGroup->setTitle(QString::fromUtf8(pProp->m_sPropertyName.GetString().GetData()));

      QVBoxLayout* pSubLayout = new QVBoxLayout(pSubGroup);
      pSubLayout->setSpacing(1);
      pSubGroup->setLayout(pSubLayout);

      pLayout->addWidget(pSubGroup);

      BuildUI(et, pProp->m_hTypeHandle.GetType(), ParentPath, pSubLayout, bEditorProp);

      ParentPath.PopBack();
    }
  }
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
    m_pGroups[1]->setTitle(QLatin1String("Object Properties"));


    m_pLayout->addWidget(m_pGroups[0]);
    m_pLayout->addWidget(m_pGroups[1]);

    QVBoxLayout* pLayout0 = new QVBoxLayout(m_pGroups[0]);
    pLayout0->setSpacing(1);
    m_pGroups[0]->setLayout(pLayout0);

    QVBoxLayout* pLayout1 = new QVBoxLayout(m_pGroups[1]);
    pLayout1->setSpacing(1);
    m_pGroups[1]->setLayout(pLayout1);
  }

  ezPropertyPath path;
  BuildUI(m_Selection[0]->GetEditorTypeAccessor(), m_Selection[0]->GetEditorTypeAccessor().GetReflectedTypeHandle().GetType(), path, m_pGroups[0]->layout(), true);
  BuildUI(m_Selection[0]->GetTypeAccessor(), m_Selection[0]->GetTypeAccessor().GetReflectedTypeHandle().GetType(), path, m_pGroups[1]->layout(), false);

  m_pSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
  
  m_pLayout->addSpacerItem(m_pSpacer);
}

