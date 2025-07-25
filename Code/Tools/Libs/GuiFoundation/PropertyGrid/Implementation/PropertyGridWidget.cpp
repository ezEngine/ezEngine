#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/Implementation/ExpressionPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/TagSetPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/VarianceWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <GuiFoundation/Widgets/CurveEditData.h>

#include <ToolsFoundation/Document/Document.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/VariantTypeRegistry.h>


ezRttiMappedObjectFactory<ezQtPropertyWidget> ezQtPropertyGridWidget::s_Factory;

static ezQtPropertyWidget* StandardTypeCreator(const ezRTTI* pRtti)
{
  EZ_ASSERT_DEV(pRtti->GetTypeFlags().IsSet(ezTypeFlags::StandardType), "This function is only valid for StandardType properties, regardless of category");

  if (pRtti == ezGetStaticRTTI<ezVariant>())
  {
    return new ezQtVariantPropertyWidget();
  }

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

    case ezVariant::Type::Vector2I:
      return new ezQtPropertyEditorIntSpinboxWidget(2, -2147483645, 2147483645);

    case ezVariant::Type::Vector3I:
      return new ezQtPropertyEditorIntSpinboxWidget(3, -2147483645, 2147483645);

    case ezVariant::Type::Vector4I:
      return new ezQtPropertyEditorIntSpinboxWidget(4, -2147483645, 2147483645);

    case ezVariant::Type::Vector2U:
      return new ezQtPropertyEditorIntSpinboxWidget(2, 0, 2147483645);

    case ezVariant::Type::Vector3U:
      return new ezQtPropertyEditorIntSpinboxWidget(3, 0, 2147483645);

    case ezVariant::Type::Vector4U:
      return new ezQtPropertyEditorIntSpinboxWidget(4, 0, 2147483645);

    case ezVariant::Type::Quaternion:
      return new ezQtPropertyEditorQuaternionWidget();

    case ezVariant::Type::Transform:
      return new ezQtPropertyEditorTransformWidget();

    case ezVariant::Type::Int8:
      return new ezQtPropertyEditorIntSpinboxWidget(1, -127, 127);

    case ezVariant::Type::UInt8:
      return new ezQtPropertyEditorIntSpinboxWidget(1, 0, 255);

    case ezVariant::Type::Int16:
      return new ezQtPropertyEditorIntSpinboxWidget(1, -32767, 32767);

    case ezVariant::Type::UInt16:
      return new ezQtPropertyEditorIntSpinboxWidget(1, 0, 65535);

    case ezVariant::Type::Int32:
    case ezVariant::Type::Int64:
      return new ezQtPropertyEditorIntSpinboxWidget(1, -2147483645, 2147483645);

    case ezVariant::Type::UInt32:
    case ezVariant::Type::UInt64:
      return new ezQtPropertyEditorIntSpinboxWidget(1, 0, 2147483645);

    case ezVariant::Type::String:
    case ezVariant::Type::StringView:
      return new ezQtPropertyEditorLineEditWidget();

    case ezVariant::Type::Color:
    case ezVariant::Type::ColorGamma:
      return new ezQtPropertyEditorColorWidget();

    case ezVariant::Type::Angle:
      return new ezQtPropertyEditorAngleWidget();

    case ezVariant::Type::HashedString:
      return new ezQtPropertyEditorLineEditWidget();

    default:
      EZ_REPORT_FAILURE("No default property widget available for type: {0}", pRtti->GetTypeName());
      return nullptr;
  }
}

static ezQtPropertyWidget* VariantArrayCreator(const ezRTTI* pRtti)
{
  return new ezQtPropertyStandardTypeContainerWidget();
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

static ezQtPropertyWidget* VarianceTypeCreator(const ezRTTI* pRtti)
{
  return new ezQtVarianceTypeWidget();
}

static ezQtPropertyWidget* Curve1DTypeCreator(const ezRTTI* pRtti)
{
  return new ezQtPropertyEditorCurve1DWidget();
}

static ezQtPropertyWidget* ExpressionTypeCreator(const ezRTTI* pRtti)
{
  return new ezQtPropertyEditorExpressionWidget();
}

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, PropertyGrid)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation", "PropertyMetaState"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<bool>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<float>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<double>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVec2>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVec3>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVec4>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVec2I32>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVec3I32>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVec4I32>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVec2U32>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVec3U32>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVec4U32>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezQuat>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezTransform>(), StandardTypeCreator);
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
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezStringView>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezTime>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezColor>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezColorGammaUB>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezAngle>(), StandardTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezHashedString>(), StandardTypeCreator);
    
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVariant>(), StandardTypeCreator);
    //ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVariantArray>(), VariantArrayCreator);

    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezEnumBase>(), EnumCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezBitflagsBase>(), BitflagsCreator);

    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezTagSetWidgetAttribute>(), TagSetCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVarianceTypeBase>(), VarianceTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezSingleCurveData>(), Curve1DTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezExpressionWidgetAttribute>(), ExpressionTypeCreator);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<bool>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<float>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<double>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezVec2>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezVec3>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezVec4>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezVec2I32>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezVec3I32>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezVec4I32>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezVec2U32>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezVec3U32>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezVec4U32>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezQuat>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezTransform>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezInt8>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezUInt8>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezInt16>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezUInt16>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezInt32>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezUInt32>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezInt64>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezUInt64>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezConstCharPtr>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezString>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezStringView>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezTime>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezColor>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezColorGammaUB>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezAngle>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezHashedString>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezVariant>());
    //ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezVariantArray>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezEnumBase>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezBitflagsBase>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezTagSetWidgetAttribute>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezVarianceTypeBase>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezSingleCurveData>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezExpressionWidgetAttribute>());
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezRttiMappedObjectFactory<ezQtPropertyWidget>& ezQtPropertyGridWidget::GetFactory()
{
  return s_Factory;
}

ezQtPropertyGridWidget::ezQtPropertyGridWidget(QWidget* pParent, ezDocument* pDocument, bool bBindToSelectionManager)
  : QWidget(pParent)
{
  setObjectName("ezQtPropertyGridWidget");

  m_pDocument = nullptr;

  m_pScroll = new QScrollArea(this);
  m_pScroll->setObjectName("QScrollArea1");
  m_pScroll->setContentsMargins(0, 0, 0, 0);

  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setObjectName("QVBoxLayout1");
  m_pLayout->setSpacing(0);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);
  m_pLayout->addWidget(m_pScroll);

  m_pContent = new QWidget(this);
  m_pContent->setObjectName("MainQWidget");
  m_pScroll->setWidget(m_pContent);
  m_pScroll->setWidgetResizable(true);

  m_pContentLayout = new QVBoxLayout(m_pContent);
  m_pContentLayout->setObjectName("QVBoxLayout2");
  m_pContentLayout->setSpacing(1);
  m_pContentLayout->setContentsMargins(0, 0, 0, 0);
  m_pContent->setLayout(m_pContentLayout);

  m_pSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
  m_pContentLayout->addSpacerItem(m_pSpacer);

  m_pTypeWidget = nullptr;

  s_Factory.m_Events.AddEventHandler(ezMakeDelegate(&ezQtPropertyGridWidget::FactoryEventHandler, this));
  ezPhantomRttiManager::s_Events.AddEventHandler(ezMakeDelegate(&ezQtPropertyGridWidget::TypeEventHandler, this));

  SetDocument(pDocument, bBindToSelectionManager);
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


void ezQtPropertyGridWidget::SetDocument(ezDocument* pDocument, bool bBindToSelectionManager)
{
  m_bBindToSelectionManager = bBindToSelectionManager;
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

void ezQtPropertyGridWidget::SetSelectionIncludeExcludeProperties(const char* szIncludeProperties /*= nullptr*/, const char* szExcludeProperties /*= nullptr*/)
{
  m_sSelectionIncludeProperties = szIncludeProperties;
  m_sSelectionExcludeProperties = szExcludeProperties;
}

void ezQtPropertyGridWidget::SetSelection(const ezDeque<const ezDocumentObject*>& selection)
{
  ezQtScopedUpdatesDisabled _(this);

  ClearSelection();

  m_Selection = selection;

  if (m_Selection.IsEmpty())
    return;

  {
    ezHybridArray<ezPropertySelection, 8> Items;
    Items.Reserve(m_Selection.GetCount());

    for (const auto* sel : m_Selection)
    {
      ezPropertySelection s;
      s.m_pObject = sel;

      Items.PushBack(s);
    }

    const ezRTTI* pCommonType = ezQtPropertyWidget::GetCommonBaseType(Items);
    m_pTypeWidget = new ezQtTypeWidget(m_pContent, this, GetObjectAccessor(), pCommonType, m_sSelectionIncludeProperties, m_sSelectionExcludeProperties);
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
  ezQtPropertyWidget* pWidget = ezQtPropertyGridWidget::GetFactory().CreateObject(pProp->GetSpecificType());
  if (pWidget != nullptr)
    return pWidget;

  return new ezQtUnsupportedPropertyWidget("No property grid widget registered");
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
        ezQtPropertyWidget* pWidget = ezQtPropertyGridWidget::GetFactory().CreateObject(pProp->GetSpecificType());
        if (pWidget != nullptr)
          return pWidget;

        if (pProp->GetFlags().IsSet(ezPropertyFlags::Class))
        {
          // Member struct / class
          return new ezQtPropertyTypeWidget(true);
        }
      }
    }
    break;
    case ezPropertyCategory::Set:
    case ezPropertyCategory::Array:
    case ezPropertyCategory::Map:
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
      const bool bIsValueType = ezReflectionUtils::IsValueType(pProp);
      if (bIsValueType)
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

void ezQtPropertyGridWidget::SetCollapseState(ezQtGroupBoxBase* pBox)
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
  ezQtGroupBoxBase* pBox = qobject_cast<ezQtGroupBoxBase*>(sender());
  ezUInt32 uiHash = GetGroupBoxHash(pBox);
  m_CollapseState[uiHash] = pBox->GetCollapseState();
}

void ezQtPropertyGridWidget::ObjectAccessorChangeEventHandler(const ezObjectAccessorChangeEvent& e)
{
  SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
}

void ezQtPropertyGridWidget::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  // TODO: even when not binding to the selection manager we need to test whether our selection is still valid.
  if (!m_bBindToSelectionManager)
    return;

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

    case ezSelectionManagerEvent::Type::ChangedRuntimeOverrideSelection:
      // ignore
      break;
  }
}

void ezQtPropertyGridWidget::FactoryEventHandler(const ezRttiMappedObjectFactory<ezQtPropertyWidget>::Event& e)
{
  if (m_bBindToSelectionManager)
    SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
  else
  {
    ezDeque<const ezDocumentObject*> selection = m_Selection;
    SetSelection(selection);
  }
}

void ezQtPropertyGridWidget::TypeEventHandler(const ezPhantomRttiManagerEvent& e)
{
  // Adding types cannot affect the property grid content.
  if (e.m_Type == ezPhantomRttiManagerEvent::Type::TypeAdded)
    return;

  EZ_PROFILE_SCOPE("TypeEventHandler");
  if (m_bBindToSelectionManager)
    SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
  else
  {
    ezDeque<const ezDocumentObject*> selection = m_Selection;
    SetSelection(selection);
  }
}

ezUInt32 ezQtPropertyGridWidget::GetGroupBoxHash(ezQtGroupBoxBase* pBox) const
{
  ezUInt32 uiHash = 0;

  QWidget* pCur = pBox;
  while (pCur != nullptr && pCur != this)
  {
    ezQtGroupBoxBase* pCurBox = qobject_cast<ezQtGroupBoxBase*>(pCur);
    if (pCurBox != nullptr)
    {
      const QByteArray name = pCurBox->GetTitle().toUtf8().data();
      uiHash += ezHashingUtils::xxHash32(name, name.length());
    }
    pCur = pCur->parentWidget();
  }
  return uiHash;
}
