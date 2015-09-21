#include <GuiFoundation/PCH.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <ToolsFoundation/Document/Document.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Algorithm/Hashing.h>

#include <QLayout>
#include <QScrollArea>

ezRttiMappedObjectFactory<ezPropertyBaseWidget> ezPropertyGridWidget::s_Factory;

static ezPropertyBaseWidget* StandardTypeCreator(const ezRTTI* pRtti)
{
  EZ_ASSERT_DEV(pRtti->GetTypeFlags().IsSet(ezTypeFlags::StandardType), "This function is only valid for StandardType properties, regardless of category");

  switch (pRtti->GetVariantType())
  {
  case ezVariant::Type::Bool:
    return new ezPropertyEditorCheckboxWidget();

  case ezVariant::Type::Time:
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

  default:
    EZ_REPORT_FAILURE("No default property widget available for type: %s", pRtti->GetTypeName());
    return nullptr;
  }
}

static ezPropertyBaseWidget* EnumCreator(const ezRTTI* pRtti)
{
  return new ezPropertyEditorEnumWidget();
}

static ezPropertyBaseWidget* BitflagsCreator(const ezRTTI* pRtti)
{
  return new ezPropertyEditorBitflagsWidget();
}

static ezPropertyBaseWidget* FileBrowserCreator(const ezRTTI* pRtti)
{
  return new ezPropertyEditorFileBrowserWidget();
}

EZ_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, PropertyGrid)

BEGIN_SUBSYSTEM_DEPENDENCIES
"ToolsFoundation"
END_SUBSYSTEM_DEPENDENCIES

ON_CORE_STARTUP
{
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

  // TODO: ezMat3, ezMat4, ezTransform, ezUuid, ezVariant
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezEnumBase>(), EnumCreator);
  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezBitflagsBase>(), BitflagsCreator);

  ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezFileBrowserAttribute>(), FileBrowserCreator);

}

ON_CORE_SHUTDOWN
{
}

EZ_END_SUBSYSTEM_DECLARATION

ezRttiMappedObjectFactory<ezPropertyBaseWidget>& ezPropertyGridWidget::GetFactory()
{
  return s_Factory;
}

ezPropertyGridWidget::ezPropertyGridWidget(ezDocumentBase* pDocument, QWidget* pParent)
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

  m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezPropertyGridWidget::SelectionEventHandler, this));
  s_Factory.m_Events.AddEventHandler(ezMakeDelegate(&ezPropertyGridWidget::FactoryEventHandler, this));
  ezPhantomRttiManager::m_Events.AddEventHandler(ezMakeDelegate(&ezPropertyGridWidget::TypeEventHandler, this));
}

ezPropertyGridWidget::~ezPropertyGridWidget()
{
  m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezPropertyGridWidget::SelectionEventHandler, this));
  s_Factory.m_Events.RemoveEventHandler(ezMakeDelegate(&ezPropertyGridWidget::FactoryEventHandler, this));
  ezPhantomRttiManager::m_Events.RemoveEventHandler(ezMakeDelegate(&ezPropertyGridWidget::TypeEventHandler, this));
}

void ezPropertyGridWidget::ClearSelection()
{
  if (m_pTypeWidget)
  {
    m_pContentLayout->removeWidget(m_pTypeWidget);
    delete m_pTypeWidget;
    m_pTypeWidget = nullptr;
  }

  m_Selection.Clear();
}

void ezPropertyGridWidget::SetSelection(const ezDeque<const ezDocumentObjectBase*>& selection)
{
  QtScopedUpdatesDisabled _(this);

  ClearSelection();

  m_Selection = selection;

  if (m_Selection.IsEmpty())
    return;

  {
    ezHybridArray<ezPropertyBaseWidget::Selection, 8> Items;
    Items.Reserve(m_Selection.GetCount());

    for (const auto* sel : m_Selection)
    {
      ezPropertyBaseWidget::Selection s;
      s.m_pObject = sel;

      Items.PushBack(s);
    }

    ezPropertyPath path;
    const ezRTTI* pCommonType = ezPropertyBaseWidget::GetCommonBaseType(Items);
    m_pTypeWidget = new ezTypeWidget(m_pContent, this, pCommonType, path);
    m_pTypeWidget->SetSelection(Items);

    m_pContentLayout->insertWidget(0, m_pTypeWidget, 0);
  }
}

const ezDocumentBase* ezPropertyGridWidget::GetDocument() const
{
  return m_pDocument;
}

ezPropertyBaseWidget* ezPropertyGridWidget::CreateMemberPropertyWidget(const ezAbstractProperty* pProp)
{
  // Try to create a registered widget for an existing ezTypeWidgetAttribute.
  const ezTypeWidgetAttribute* pAttrib = pProp->GetAttributeByType<ezTypeWidgetAttribute>();
  if (pAttrib != nullptr)
  {
    ezPropertyBaseWidget* pWidget = ezPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
    if (pWidget != nullptr)
      return pWidget;
  }

  // Try to create a registered widget for the given property type.
  return ezPropertyGridWidget::GetFactory().CreateObject(pProp->GetSpecificType());
}

ezPropertyBaseWidget* ezPropertyGridWidget::CreatePropertyWidget(const ezAbstractProperty* pProp)
{
  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Member:
    {
      // Try to create a registered widget for an existing ezTypeWidgetAttribute.
      const ezTypeWidgetAttribute* pAttrib = pProp->GetAttributeByType<ezTypeWidgetAttribute>();
      if (pAttrib != nullptr)
      {
        ezPropertyBaseWidget* pWidget = ezPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
        if (pWidget != nullptr)
          return pWidget;
      }

      if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
      {
        if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
          return new ezPropertyPointerWidget();
        else
          return new ezUnsupportedPropertyWidget("Pointer: Use ezPropertyFlags::PointerOwner or provide derived ezTypeWidgetAttribute");
      }
      else
      {
        ezPropertyBaseWidget* pWidget = CreateMemberPropertyWidget(pProp);
        if (pWidget != nullptr)
          return pWidget;

        if (!pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType | ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
        {
          // Member struct / class
          return new ezPropertyTypeWidget(true);
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
        ezPropertyBaseWidget* pWidget = ezPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
        if (pWidget != nullptr)
          return pWidget;
      }

      // Fallback to default container widgets.
      if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType))
      {
        return new ezPropertyStandardTypeContainerWidget();
      }
      else
      {
        if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
        {
          return new ezUnsupportedPropertyWidget("Pointer: Use ezPropertyFlags::PointerOwner or provide derived ezContainerWidgetAttribute");
        }

        return new ezPropertyTypeContainerWidget();
      }
    }
    break;
  }

  return new ezUnsupportedPropertyWidget();
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

void ezPropertyGridWidget::SelectionEventHandler(const ezSelectionManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezSelectionManager::Event::Type::SelectionCleared:
    {
      ClearSelection();
    }
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

void ezPropertyGridWidget::FactoryEventHandler(const ezRttiMappedObjectFactory<ezPropertyBaseWidget>::Event& e)
{
  SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
}

void ezPropertyGridWidget::TypeEventHandler(const ezPhantomRttiManager::Event& e)
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
