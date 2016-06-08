#include <GuiFoundation/PCH.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/TagSetPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <ToolsFoundation/Document/Document.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Algorithm/Hashing.h>

#include <QLayout>
#include <QScrollArea>

ezRttiMappedObjectFactory<ezQtPropertyWidget> ezPropertyGridWidget::s_Factory;

static ezQtPropertyWidget* StandardTypeCreator(const ezRTTI* pRtti)
{
  EZ_ASSERT_DEV(pRtti->GetTypeFlags().IsSet(ezTypeFlags::StandardType), "This function is only valid for StandardType properties, regardless of category");

  switch (pRtti->GetVariantType())
  {
  case ezVariant::Type::Bool:
    return new ezPropertyEditorCheckboxWidget();

  case ezVariant::Type::Time:
    return new ezPropertyEditorTimeWidget();

  case ezVariant::Type::Float:
  case ezVariant::Type::Double:
    return new ezPropertyEditorDoubleSpinboxWidget(1);

  case ezVariant::Type::Vector2:
    return new ezPropertyEditorDoubleSpinboxWidget(2);

  case ezVariant::Type::Vector3:
    return new ezPropertyEditorDoubleSpinboxWidget(3);

  case ezVariant::Type::Vector4:
    return new ezPropertyEditorDoubleSpinboxWidget(4);

  case ezVariant::Type::Quaternion:
    return new ezPropertyEditorQuaternionWidget();

  case ezVariant::Type::Int8:
    return new ezPropertyEditorIntSpinboxWidget(-127, 127);

  case ezVariant::Type::UInt8:
    return new ezPropertyEditorIntSpinboxWidget(0, 255);

  case ezVariant::Type::Int16:
    return new ezPropertyEditorIntSpinboxWidget(-32767, 32767);

  case ezVariant::Type::UInt16:
    return new ezPropertyEditorIntSpinboxWidget(0, 65535);

  case ezVariant::Type::Int32:
  case ezVariant::Type::Int64:
    return new ezPropertyEditorIntSpinboxWidget(-2147483645, 2147483645);

  case ezVariant::Type::UInt32:
  case ezVariant::Type::UInt64:
    return new ezPropertyEditorIntSpinboxWidget(0, 2147483645);

  case ezVariant::Type::String:
    return new ezPropertyEditorLineEditWidget();

  case ezVariant::Type::Color:
    return new ezPropertyEditorColorWidget();

  case ezVariant::Type::Angle:
    return new ezPropertyEditorAngleWidget();


  default:
    EZ_REPORT_FAILURE("No default property widget available for type: %s", pRtti->GetTypeName());
    return nullptr;
  }
}

static ezQtPropertyWidget* EnumCreator(const ezRTTI* pRtti)
{
  return new ezPropertyEditorEnumWidget();
}

static ezQtPropertyWidget* BitflagsCreator(const ezRTTI* pRtti)
{
  return new ezPropertyEditorBitflagsWidget();
}

static ezQtPropertyWidget* TagSetCreator(const ezRTTI* pRtti)
{
  return new ezPropertyEditorTagSetWidget();
}

EZ_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, PropertyGrid)

BEGIN_SUBSYSTEM_DEPENDENCIES
"ToolsFoundation", "PropertyMetaState"
END_SUBSYSTEM_DEPENDENCIES

ON_CORE_STARTUP
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezPropertyGridWidget::PropertyMetaStateEventHandler);

  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<bool>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<float>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<double>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVec2>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVec3>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVec4>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezQuat>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezInt8>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezUInt8>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezInt16>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezUInt16>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezInt32>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezUInt32>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezInt64>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezUInt64>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezConstCharPtr>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezString>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezTime>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezColor>(), StandardTypeCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezAngle>(), StandardTypeCreator);

  // TODO: ezMat3, ezMat4, ezTransform, ezUuid, ezVariant
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezEnumBase>(), EnumCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezBitflagsBase>(), BitflagsCreator);

  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezTagSetWidgetAttribute>(), TagSetCreator);
}

ON_CORE_SHUTDOWN
{
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezPropertyGridWidget::PropertyMetaStateEventHandler);
}

EZ_END_SUBSYSTEM_DECLARATION

ezRttiMappedObjectFactory<ezQtPropertyWidget>& ezPropertyGridWidget::GetFactory()
{
  return s_Factory;
}

ezPropertyGridWidget::ezPropertyGridWidget(QWidget* pParent, ezDocument* pDocument)
  : QWidget(pParent)
{
  m_pDocument = pDocument;

  m_pScroll = new QScrollArea(this);
  m_pScroll->setContentsMargins(0, 0, 0, 0);

  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setSpacing(0);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);
  m_pLayout->addWidget(m_pScroll);

  m_pContent = new QWidget(this);
  m_pScroll->setWidget(m_pContent);
  m_pScroll->setWidgetResizable(true);
  m_pContent->setBackgroundRole(QPalette::ColorRole::Window);
  m_pContent->setAutoFillBackground(true);

  m_pContentLayout = new QVBoxLayout(m_pContent);
  m_pContentLayout->setSpacing(1);
  m_pContentLayout->setMargin(1);
  m_pContent->setLayout(m_pContentLayout);

  m_pSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
  m_pContentLayout->addSpacerItem(m_pSpacer);

  m_pTypeWidget = nullptr;

  s_Factory.m_Events.AddEventHandler(ezMakeDelegate(&ezPropertyGridWidget::FactoryEventHandler, this));
  ezPhantomRttiManager::s_Events.AddEventHandler(ezMakeDelegate(&ezPropertyGridWidget::TypeEventHandler, this));

  m_pDocument = nullptr;
  SetDocument(pDocument);
}

ezPropertyGridWidget::~ezPropertyGridWidget()
{
  s_Factory.m_Events.RemoveEventHandler(ezMakeDelegate(&ezPropertyGridWidget::FactoryEventHandler, this));
  ezPhantomRttiManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezPropertyGridWidget::TypeEventHandler, this));

  if (m_pDocument)
  {
    m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezPropertyGridWidget::SelectionEventHandler, this));
  }
}


void ezPropertyGridWidget::SetDocument(ezDocument* pDocument)
{
  if (m_pDocument)
  {
    m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezPropertyGridWidget::SelectionEventHandler, this));
  }

  m_pDocument = pDocument;

  if (m_pDocument)
  {
    m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezPropertyGridWidget::SelectionEventHandler, this));
  }
}

void ezPropertyGridWidget::ClearSelection()
{
  if (m_pTypeWidget)
  {
    m_pContentLayout->removeWidget(m_pTypeWidget);
    m_pTypeWidget->hide();

    m_pTypeWidget->PrepareToDie();

    m_pTypeWidget->deleteLater();
    m_pTypeWidget = nullptr;
  }

  m_Selection.Clear();
}

void ezPropertyGridWidget::SetSelection(const ezDeque<const ezDocumentObject*>& selection)
{
  QtScopedUpdatesDisabled _(this);

  ClearSelection();

  m_Selection = selection;

  if (m_Selection.IsEmpty())
    return;

  {
    ezHybridArray<ezQtPropertyWidget::Selection, 8> Items;
    Items.Reserve(m_Selection.GetCount());

    for (const auto* sel : m_Selection)
    {
      ezQtPropertyWidget::Selection s;
      s.m_pObject = sel;

      Items.PushBack(s);
    }

    ezPropertyPath path;
    const ezRTTI* pCommonType = ezQtPropertyWidget::GetCommonBaseType(Items);
    m_pTypeWidget = new ezTypeWidget(m_pContent, this, pCommonType, path);
    m_pTypeWidget->SetSelection(Items);

    m_pContentLayout->insertWidget(0, m_pTypeWidget, 0);
  }
}

const ezDocument* ezPropertyGridWidget::GetDocument() const
{
  return m_pDocument;
}

const ezDocumentObjectManager* ezPropertyGridWidget::GetObjectManager() const
{
  return m_pDocument->GetObjectManager();
}

ezCommandHistory* ezPropertyGridWidget::GetCommandHistory() const
{
  return m_pDocument->GetCommandHistory();
}

ezQtPropertyWidget* ezPropertyGridWidget::CreateMemberPropertyWidget(const ezAbstractProperty* pProp)
{
  // Try to create a registered widget for an existing ezTypeWidgetAttribute.
  const ezTypeWidgetAttribute* pAttrib = pProp->GetAttributeByType<ezTypeWidgetAttribute>();
  if (pAttrib != nullptr)
  {
    ezQtPropertyWidget* pWidget = ezPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
    if (pWidget != nullptr)
      return pWidget;
  }

  // Try to create a registered widget for the given property type.
  return ezPropertyGridWidget::GetFactory().CreateObject(pProp->GetSpecificType());
}

ezQtPropertyWidget* ezPropertyGridWidget::CreatePropertyWidget(const ezAbstractProperty* pProp)
{
  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Member:
    {
      // Try to create a registered widget for an existing ezTypeWidgetAttribute.
      const ezTypeWidgetAttribute* pAttrib = pProp->GetAttributeByType<ezTypeWidgetAttribute>();
      if (pAttrib != nullptr)
      {
        ezQtPropertyWidget* pWidget = ezPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
        if (pWidget != nullptr)
          return pWidget;
      }

      if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
      {
        if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
          return new ezQtPropertyPointerWidget();
        else
          return new ezQtUnsupportedPropertyWidget("Pointer: Use ezPropertyFlags::PointerOwner or provide derived ezTypeWidgetAttribute");
      }
      else
      {
        ezQtPropertyWidget* pWidget = CreateMemberPropertyWidget(pProp);
        if (pWidget != nullptr)
          return pWidget;

        if (!pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType | ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
        {
          // Member struct / class
          return new ezQtPropertyTypeWidget(true);
        }
      }
    }
    break;
  case ezPropertyCategory::Set:
  case ezPropertyCategory::Array:
    {
      // Try to create a registered container widget for an existing ezContainerWidgetAttribute.
      const ezContainerWidgetAttribute* pAttrib = pProp->GetAttributeByType<ezContainerWidgetAttribute>();
      if (pAttrib != nullptr)
      {
        ezQtPropertyWidget* pWidget = ezPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
        if (pWidget != nullptr)
          return pWidget;
      }

      // Fallback to default container widgets.
      if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType))
      {
        return new ezQtPropertyStandardTypeContainerWidget();
      }
      else
      {
        if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
        {
          return new ezQtUnsupportedPropertyWidget("Pointer: Use ezPropertyFlags::PointerOwner or provide derived ezContainerWidgetAttribute");
        }

        return new ezQtPropertyTypeContainerWidget();
      }
    }
    break;

  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
    break;
  }

  return new ezQtUnsupportedPropertyWidget();
}

void ezPropertyGridWidget::SetCollapseState(ezCollapsibleGroupBox* pBox)
{
  ezUInt32 uiHash = GetGroupBoxHash(pBox);
  bool bCollapsed = false;
  auto it = m_CollapseState.Find(uiHash);
  if (it.IsValid())
    bCollapsed = it.Value();

  pBox->SetCollapseState(bCollapsed);
}

void ezPropertyGridWidget::OnCollapseStateChanged(bool bCollapsed)
{
  ezCollapsibleGroupBox* pBox = qobject_cast<ezCollapsibleGroupBox*>(sender());
  ezUInt32 uiHash = GetGroupBoxHash(pBox);
  m_CollapseState[uiHash] = pBox->GetCollapseState();
}


void GetDefaultValues(const ezRTTI* pType, ezPropertyPath& ParentPath, const ezDocument* pDocument, ezPropertyMetaStateEvent& e)
{
  const ezRTTI* pParentType = pType->GetParentType();
  if (pParentType != nullptr)
    GetDefaultValues(pParentType, ParentPath, pDocument, e);

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

    switch (pProp->GetCategory())
    {
    case ezPropertyCategory::Member:
      {
        if (!pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) && !pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType | ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
        {
          // Member struct / class
          GetDefaultValues(pProp->GetSpecificType(), ParentPath, pDocument, e);
        }
        else
        {
          ezString sPath = ParentPath.GetPathString();
          (*e.m_pPropertyStates)[sPath].m_bIsDefaultValue = pDocument->IsDefaultValue(e.m_pObject, ParentPath);
        }
      }
      break;

    default:
      break;
    }

    ParentPath.PopBack();
  }
}

void ezPropertyGridWidget::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  const ezDocument* pDocument = e.m_pObject->GetDocumentObjectManager()->GetDocument();
  const ezRTTI* pType = e.m_pObject->GetTypeAccessor().GetType();
  ezPropertyPath ParentPath;
  GetDefaultValues(pType, ParentPath, pDocument, e);
}

void ezPropertyGridWidget::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  switch (e.m_Type)
  {
  case ezSelectionManagerEvent::Type::SelectionCleared:
    {
      ClearSelection();
    }
    break;
  case ezSelectionManagerEvent::Type::SelectionSet:
  case ezSelectionManagerEvent::Type::ObjectAdded:
  case ezSelectionManagerEvent::Type::ObjectRemoved:
    {
      SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
    }
    break;
  }
}

void ezPropertyGridWidget::FactoryEventHandler(const ezRttiMappedObjectFactory<ezQtPropertyWidget>::Event& e)
{
  SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
}

void ezPropertyGridWidget::TypeEventHandler(const ezPhantomRttiManagerEvent& e)
{
  SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
}

ezUInt32 ezPropertyGridWidget::GetGroupBoxHash(ezCollapsibleGroupBox* pBox) const
{
  ezUInt32 uiHash = 0;

  QWidget* pCur = pBox;
  while (pCur != nullptr && pCur != this)
  {
    ezCollapsibleGroupBox* pCurBox = qobject_cast<ezCollapsibleGroupBox*>(pCur);
    if (pCurBox != nullptr)
    {
      uiHash = ezHashing::MurmurHash(ezHashing::StringWrapper(pCurBox->title().toUtf8().data()), uiHash);
    }
    pCur = pCur->parentWidget();
  }
  return uiHash;
}
