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

ezRttiMappedObjectFactory<ezQtPropertyWidget> ezQtPropertyGridWidget::s_Factory;

static ezQtPropertyWidget* StandardTypeCreator(const ezRTTI* pRtti)
{
  EZ_ASSERT_DEV(pRtti->GetTypeFlags().IsSet(ezTypeFlags::StandardType), "This function is only valid for StandardType properties, regardless of category");

  switch (pRtti->GetVariantType())
  {
  case ezVariant::Type::Bool:
    return new ezQtPropertyEditorCheckboxWidget();

  case ezVariant::Type::Time:
    return new ezQtPropertyEditorTimeWidget();

  case ezVariant::Type::Float:
  case ezVariant::Type::Double:
    return new ezQtPropertyEditorDoubleSpinboxWidget(1);

  case ezVariant::Type::Vector2:
    return new ezQtPropertyEditorDoubleSpinboxWidget(2);

  case ezVariant::Type::Vector3:
    return new ezQtPropertyEditorDoubleSpinboxWidget(3);

  case ezVariant::Type::Vector4:
    return new ezQtPropertyEditorDoubleSpinboxWidget(4);

  case ezVariant::Type::Quaternion:
    return new ezQtPropertyEditorQuaternionWidget();

  case ezVariant::Type::Int8:
    return new ezQtPropertyEditorIntSpinboxWidget(-127, 127);

  case ezVariant::Type::UInt8:
    return new ezQtPropertyEditorIntSpinboxWidget(0, 255);

  case ezVariant::Type::Int16:
    return new ezQtPropertyEditorIntSpinboxWidget(-32767, 32767);

  case ezVariant::Type::UInt16:
    return new ezQtPropertyEditorIntSpinboxWidget(0, 65535);

  case ezVariant::Type::Int32:
  case ezVariant::Type::Int64:
    return new ezQtPropertyEditorIntSpinboxWidget(-2147483645, 2147483645);

  case ezVariant::Type::UInt32:
  case ezVariant::Type::UInt64:
    return new ezQtPropertyEditorIntSpinboxWidget(0, 2147483645);

  case ezVariant::Type::String:
    return new ezQtPropertyEditorLineEditWidget();

  case ezVariant::Type::Color:
  case ezVariant::Type::ColorGamma:
    return new ezQtPropertyEditorColorWidget();

  case ezVariant::Type::Angle:
    return new ezQtPropertyEditorAngleWidget();


  default:
    EZ_REPORT_FAILURE("No default property widget available for type: {0}", pRtti->GetTypeName());
    return nullptr;
  }
}

static ezQtPropertyWidget* EnumCreator(const ezRTTI* pRtti)
{
  return new ezQtPropertyEditorEnumWidget();
}

static ezQtPropertyWidget* BitflagsCreator(const ezRTTI* pRtti)
{
  return new ezQtPropertyEditorBitflagsWidget();
}

static ezQtPropertyWidget* TagSetCreator(const ezRTTI* pRtti)
{
  return new ezQtPropertyEditorTagSetWidget();
}

EZ_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, PropertyGrid)

BEGIN_SUBSYSTEM_DEPENDENCIES
"ToolsFoundation", "PropertyMetaState"
END_SUBSYSTEM_DEPENDENCIES

ON_CORE_STARTUP
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezQtPropertyGridWidget::PropertyMetaStateEventHandler);

  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<bool>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<float>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<double>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVec2>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVec3>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVec4>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezQuat>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezInt8>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezUInt8>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezInt16>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezUInt16>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezInt32>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezUInt32>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezInt64>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezUInt64>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezConstCharPtr>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezString>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezTime>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezColor>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezColorGammaUB>(), StandardTypeCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezAngle>(), StandardTypeCreator);

  // TODO: ezMat3, ezMat4, ezTransform, ezUuid, ezVariant
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezEnumBase>(), EnumCreator);
  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezBitflagsBase>(), BitflagsCreator);

  ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezTagSetWidgetAttribute>(), TagSetCreator);
}

ON_CORE_SHUTDOWN
{
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezQtPropertyGridWidget::PropertyMetaStateEventHandler);
}

EZ_END_SUBSYSTEM_DECLARATION

ezRttiMappedObjectFactory<ezQtPropertyWidget>& ezQtPropertyGridWidget::GetFactory()
{
  return s_Factory;
}

ezQtPropertyGridWidget::ezQtPropertyGridWidget(QWidget* pParent, ezDocument* pDocument)
  : QWidget(pParent)
{
  m_pDocument = nullptr;

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

  s_Factory.m_Events.AddEventHandler(ezMakeDelegate(&ezQtPropertyGridWidget::FactoryEventHandler, this));
  ezPhantomRttiManager::s_Events.AddEventHandler(ezMakeDelegate(&ezQtPropertyGridWidget::TypeEventHandler, this));

  SetDocument(pDocument);
}

ezQtPropertyGridWidget::~ezQtPropertyGridWidget()
{
  s_Factory.m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyGridWidget::FactoryEventHandler, this));
  ezPhantomRttiManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyGridWidget::TypeEventHandler, this));

  if (m_pDocument)
  {
    m_pDocument->m_ObjectAccessorChangeEvents.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyGridWidget::ObjectAccessorChangeEventHandler, this));
    m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyGridWidget::SelectionEventHandler, this));
  }
}


void ezQtPropertyGridWidget::SetDocument(ezDocument* pDocument)
{
  if (m_pDocument)
  {
    m_pDocument->m_ObjectAccessorChangeEvents.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyGridWidget::ObjectAccessorChangeEventHandler, this));
    m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyGridWidget::SelectionEventHandler, this));
  }

  m_pDocument = pDocument;

  if (m_pDocument)
  {
    m_pDocument->m_ObjectAccessorChangeEvents.AddEventHandler(ezMakeDelegate(&ezQtPropertyGridWidget::ObjectAccessorChangeEventHandler, this));
    m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtPropertyGridWidget::SelectionEventHandler, this));
  }
}

void ezQtPropertyGridWidget::ClearSelection()
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

void ezQtPropertyGridWidget::SetSelection(const ezDeque<const ezDocumentObject*>& selection)
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

    const ezRTTI* pCommonType = ezQtPropertyWidget::GetCommonBaseType(Items);
    m_pTypeWidget = new ezQtTypeWidget(m_pContent, this, pCommonType);
    m_pTypeWidget->SetSelection(Items);

    m_pContentLayout->insertWidget(0, m_pTypeWidget, 0);
  }
}

const ezDocument* ezQtPropertyGridWidget::GetDocument() const
{
  return m_pDocument;
}

const ezDocumentObjectManager* ezQtPropertyGridWidget::GetObjectManager() const
{
  return m_pDocument->GetObjectManager();
}

ezCommandHistory* ezQtPropertyGridWidget::GetCommandHistory() const
{
  return m_pDocument->GetCommandHistory();
}


ezObjectAccessorBase* ezQtPropertyGridWidget::GetObjectAccessor() const
{
  return m_pDocument->GetObjectAccessor();
}

ezQtPropertyWidget* ezQtPropertyGridWidget::CreateMemberPropertyWidget(const ezAbstractProperty* pProp)
{
  // Try to create a registered widget for an existing ezTypeWidgetAttribute.
  const ezTypeWidgetAttribute* pAttrib = pProp->GetAttributeByType<ezTypeWidgetAttribute>();
  if (pAttrib != nullptr)
  {
    ezQtPropertyWidget* pWidget = ezQtPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
    if (pWidget != nullptr)
      return pWidget;
  }

  // Try to create a registered widget for the given property type.
  return ezQtPropertyGridWidget::GetFactory().CreateObject(pProp->GetSpecificType());
}

ezQtPropertyWidget* ezQtPropertyGridWidget::CreatePropertyWidget(const ezAbstractProperty* pProp)
{
  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Member:
    {
      // Try to create a registered widget for an existing ezTypeWidgetAttribute.
      const ezTypeWidgetAttribute* pAttrib = pProp->GetAttributeByType<ezTypeWidgetAttribute>();
      if (pAttrib != nullptr)
      {
        ezQtPropertyWidget* pWidget = ezQtPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
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

        if (pProp->GetFlags().IsSet(ezPropertyFlags::EmbeddedClass))
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
        ezQtPropertyWidget* pWidget = ezQtPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
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

void ezQtPropertyGridWidget::SetCollapseState(ezQtCollapsibleGroupBox* pBox)
{
  ezUInt32 uiHash = GetGroupBoxHash(pBox);
  bool bCollapsed = false;
  auto it = m_CollapseState.Find(uiHash);
  if (it.IsValid())
    bCollapsed = it.Value();

  pBox->SetCollapseState(bCollapsed);
}

void ezQtPropertyGridWidget::OnCollapseStateChanged(bool bCollapsed)
{
  ezQtCollapsibleGroupBox* pBox = qobject_cast<ezQtCollapsibleGroupBox*>(sender());
  ezUInt32 uiHash = GetGroupBoxHash(pBox);
  m_CollapseState[uiHash] = pBox->GetCollapseState();
}


void GetDefaultValues(const ezRTTI* pType, const ezDocument* pDocument, ezPropertyMetaStateEvent& e)
{
  const ezRTTI* pParentType = pType->GetParentType();
  if (pParentType != nullptr)
    GetDefaultValues(pParentType, pDocument, e);

  for (ezUInt32 i = 0; i < pType->GetProperties().GetCount(); ++i)
  {
    const ezAbstractProperty* pProp = pType->GetProperties()[i];
    if (pProp->GetFlags().IsSet(ezPropertyFlags::Hidden))
      continue;

    if (pProp->GetAttributeByType<ezHiddenAttribute>() != nullptr)
      continue;

    if (pProp->GetSpecificType()->GetAttributeByType<ezHiddenAttribute>() != nullptr)
      continue;

    switch (pProp->GetCategory())
    {
    case ezPropertyCategory::Member:
      {
        if (!pProp->GetFlags().IsSet(ezPropertyFlags::EmbeddedClass))
        {
          (*e.m_pPropertyStates)[pProp->GetPropertyName()].m_bIsDefaultValue = pDocument->IsDefaultValue(e.m_pObject, pProp->GetPropertyName(), true);
        }
      }
      break;

    default:
      break;
    }
  }
}

void ezQtPropertyGridWidget::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  const ezDocument* pDocument = e.m_pObject->GetDocumentObjectManager()->GetDocument();
  const ezRTTI* pType = e.m_pObject->GetTypeAccessor().GetType();
  GetDefaultValues(pType, pDocument, e);
}


void ezQtPropertyGridWidget::ObjectAccessorChangeEventHandler(const ezObjectAccessorChangeEvent& e)
{
  SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
}

void ezQtPropertyGridWidget::SelectionEventHandler(const ezSelectionManagerEvent& e)
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

void ezQtPropertyGridWidget::FactoryEventHandler(const ezRttiMappedObjectFactory<ezQtPropertyWidget>::Event& e)
{
  SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
}

void ezQtPropertyGridWidget::TypeEventHandler(const ezPhantomRttiManagerEvent& e)
{
  SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
}

ezUInt32 ezQtPropertyGridWidget::GetGroupBoxHash(ezQtCollapsibleGroupBox* pBox) const
{
  ezUInt32 uiHash = 0;

  QWidget* pCur = pBox;
  while (pCur != nullptr && pCur != this)
  {
    ezQtCollapsibleGroupBox* pCurBox = qobject_cast<ezQtCollapsibleGroupBox*>(pCur);
    if (pCurBox != nullptr)
    {
      uiHash = ezHashing::MurmurHash(ezHashing::StringWrapper(pCurBox->title().toUtf8().data()), uiHash);
    }
    pCur = pCur->parentWidget();
  }
  return uiHash;
}
