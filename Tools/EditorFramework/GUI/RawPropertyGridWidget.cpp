#include <PCH.h>
#include <EditorFramework/GUI/RawPropertyGridWidget.h>
#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <QPushButton>
#include <QGroupBox>
#include <QScrollArea>

ezRawPropertyGridWidget::ezRawPropertyGridWidget(QWidget* pParent) : QWidget(pParent)
{
  QScrollArea* pScroll = new QScrollArea(this);
  pScroll->setContentsMargins(0, 0, 0, 0);

  QVBoxLayout* pLayout2 = new QVBoxLayout(this);
  pLayout2->setSpacing(1);
  pLayout2->setMargin(1);
  this->setLayout(pLayout2);
  pLayout2->addWidget(pScroll);

  QWidget* pContent = new QWidget(this);
  pScroll->setWidget(pContent);
  pScroll->setWidgetResizable(true);
  pContent->setBackgroundRole(QPalette::ColorRole::Background);

  m_pLayout = new QVBoxLayout(pContent);
  m_pLayout->setSpacing(1);
  pContent->setLayout(m_pLayout);

  m_pGroups[0] = new QGroupBox(pContent);
  m_pGroups[1] = new QGroupBox(pContent);

  m_pGroups[0]->setTitle(QLatin1String("Editor Properties"));
  m_pGroups[1]->setTitle(QLatin1String("Object Properties"));


  m_pLayout->addWidget(m_pGroups[0]);
  m_pLayout->addWidget(m_pGroups[1]);

  QVBoxLayout* pLayout0 = new QVBoxLayout(pContent);
  pLayout0->setSpacing(1);
  m_pGroups[0]->setLayout(pLayout0);

  QVBoxLayout* pLayout1 = new QVBoxLayout(pContent);
  pLayout1->setSpacing(1);
  m_pGroups[1]->setLayout(pLayout1);
}

ezRawPropertyGridWidget::~ezRawPropertyGridWidget()
{
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
  //pLayout->setSpacing(10);
  //pLayout->setMargin(0);
  //QFrame* pTypeGroup = new QFrame((QWidget*) pLayout->parent());
  //pTypeGroup->setAutoFillBackground(true);
  //pTypeGroup->setFrameShadow(QFrame::Shadow::Sunken);
  //pTypeGroup->setFrameShape(QFrame::Shape::Box);
  //pTypeGroup->setLineWidth(1);
  //pTypeGroup->setMidLineWidth(0);
  ////pTypeGroup->setTitle(QString::fromUtf8(pType->GetTypeName().GetString().GetData()));

  //QVBoxLayout* pTypeLayout = new QVBoxLayout(pTypeGroup);
  //pTypeLayout->setMargin(0);
  ////pTypeLayout->setContentsMargins(0, 20, 0, 20);
  //pTypeLayout->setSpacing(1);
  //pTypeGroup->setLayout(pTypeLayout);

  //pLayout->addWidget(pTypeGroup);

  //pLayout = pTypeLayout;

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

      case ezVariant::Type::Float:
      case ezVariant::Type::Double:
        pNewWidget = new ezPropertyEditorDoubleSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this);
        break;

      case ezVariant::Type::String:
        pNewWidget = new ezPropertyEditorLineEditWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this);
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

void ezRawPropertyGridWidget::SetSelection(const ezHybridArray<ezDocumentObjectBase*, 32>& selection)
{
  m_Selection = selection;

  ezPropertyPath path;
  BuildUI(m_Selection[0]->GetEditorTypeAccessor(), m_Selection[0]->GetEditorTypeAccessor().GetReflectedTypeHandle().GetType(), path, m_pGroups[0]->layout(), true);
  BuildUI(m_Selection[0]->GetTypeAccessor(), m_Selection[0]->GetTypeAccessor().GetReflectedTypeHandle().GetType(), path, m_pGroups[1]->layout(), false);

  QSpacerItem* pSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
  
  m_pLayout->addSpacerItem(pSpacer);
}

