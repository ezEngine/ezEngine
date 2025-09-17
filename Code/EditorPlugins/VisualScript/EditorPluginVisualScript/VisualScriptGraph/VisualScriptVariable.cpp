#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezVisualScriptVariableType, 1)
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::Bool),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::Byte),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::Int),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::Int64),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::Float),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::Double),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::Color),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::Vector3),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::Quaternion),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::Transform),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::Time),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::Angle),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::String),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::HashedString),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::GameObject),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::Component),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::TypedPointer),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableType::Variant),
  // EZ_ENUM_CONSTANT(ezVisualScriptVariableType::Resource), // Not yet supported in the editor
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

static_assert(ezVisualScriptVariableType::Variant == ezVisualScriptDataType::Variant);
static_assert(ezVisualScriptVariableType::Resource == ezVisualScriptDataType::Resource);

///////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezVisualScriptVariableCategory, 1)
  EZ_ENUM_CONSTANT(ezVisualScriptVariableCategory::Member),
  EZ_ENUM_CONSTANT(ezVisualScriptVariableCategory::Array),
  // EZ_ENUM_CONSTANT(ezVisualScriptVariableCategory::Map), // Maps are not supported yet
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
ezPropertyCategory::Enum ezVisualScriptVariableCategory::GetPropertyCategory(Enum category)
{
  switch (category)
  {
    case Member:
      return ezPropertyCategory::Member;
    case Array:
      return ezPropertyCategory::Array;
    case Map:
      return ezPropertyCategory::Map;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return ezPropertyCategory::Member;
  }
}

///////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVisualScriptVariableTypeDeclaration, ezNoBase, 1, ezRTTIDefaultAllocator<ezVisualScriptVariableTypeDeclaration>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Type", ezVisualScriptVariableType, m_Type),
    EZ_ENUM_MEMBER_PROPERTY("Category", ezVisualScriptVariableCategory, m_Category),
    EZ_MEMBER_PROPERTY("Public", m_bPublic),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezVisualScriptVariableTypeDeclaration);
// clang-format on

ezVisualScriptDataType::Enum ezVisualScriptVariableTypeDeclaration::GetDataType() const
{
  if (m_Category == ezVisualScriptVariableCategory::Array)
  {
    return ezVisualScriptDataType::Array;
  }
  else if (m_Category == ezVisualScriptVariableCategory::Map)
  {
    return ezVisualScriptDataType::Map;
  }

  return static_cast<ezVisualScriptDataType::Enum>(m_Type.GetValue());
}

void operator<<(ezStreamWriter& inout_stream, const ezVisualScriptVariableTypeDeclaration& value)
{
  inout_stream << value.m_Type;
  inout_stream << value.m_Category;
  inout_stream << value.m_bPublic;
}

void operator>>(ezStreamReader& inout_stream, ezVisualScriptVariableTypeDeclaration& value)
{
  inout_stream >> value.m_Type;
  inout_stream >> value.m_Category;
  inout_stream >> value.m_bPublic;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptVariableAttribute, 1, ezRTTIDefaultAllocator<ezVisualScriptVariableAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezQtVisualScriptVariableWidget::ezQtVisualScriptVariableWidget() = default;
ezQtVisualScriptVariableWidget::~ezQtVisualScriptVariableWidget() = default;

void ezQtVisualScriptVariableWidget::InternalSetValue(const ezVariant& value)
{
  ezQtVariantPropertyWidget::InternalSetValue(value);

  bool bEnableTypeSelection = true;
  for (const auto& item : m_Items)
  {
    ezVariant typeDeclVar;
    if (m_pObjectAccessor->GetValueByName(item.m_pObject, "Type", typeDeclVar, item.m_Index).Failed())
      break;

    if (typeDeclVar.GetReflectedType() != ezGetStaticRTTI<ezVisualScriptVariableTypeDeclaration>())
      break;

    auto typeDecl = typeDeclVar.Get<ezVisualScriptVariableTypeDeclaration>();
    if (typeDecl.m_Type != ezVisualScriptVariableType::Variant || typeDecl.m_Category != ezVisualScriptVariableCategory::Member)
    {
      bEnableTypeSelection = false;
      break;
    }
  }

  EnableTypeSelection(bEnableTypeSelection);
}

ezResult ezQtVisualScriptVariableWidget::GetVariantTypeDisplayName(ezVariantType::Enum type, ezStringBuilder& out_sName) const
{
  if (type == ezVariantType::Int8 ||
      type == ezVariantType::Int16 ||
      type == ezVariantType::UInt16 ||
      type == ezVariantType::UInt32 ||
      type == ezVariantType::UInt64 ||
      type == ezVariantType::StringView ||
      type == ezVariantType::TempHashedString)
    return EZ_FAILURE;

  ezVisualScriptDataType::Enum dataType = ezVisualScriptDataType::FromVariantType(type);
  if (type != ezVariantType::Invalid && dataType == ezVisualScriptDataType::Invalid)
    return EZ_FAILURE;

  const ezRTTI* pVisualScriptDataType = ezGetStaticRTTI<ezVisualScriptDataType>();
  if (ezReflectionUtils::EnumerationToString(pVisualScriptDataType, dataType, out_sName) == false)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

ezQtVisualScriptVariableTypeDeclarationWidget::ezQtVisualScriptVariableTypeDeclarationWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 4);
  m_pLayout->setSpacing(1);
  setLayout(m_pLayout);

  m_pTypeList = new QComboBox(this);
  m_pTypeList->installEventFilter(this);
  m_pLayout->addWidget(m_pTypeList);

  m_pCategoryList = new QMenu(this);

  m_pCategoryButton = new QPushButton(this);
  m_pCategoryButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  m_pCategoryButton->installEventFilter(this);
  m_pCategoryButton->setMenu(m_pCategoryList);
  m_pLayout->addWidget(m_pCategoryButton);

  m_pVisibilityButton = new QPushButton(this);
  m_pVisibilityButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  QIcon icon;
  icon.addFile(QString::fromUtf8(":/EditorFramework/Icons/ObjectsHidden.svg"), QSize(), QIcon::Normal, QIcon::Off);
  icon.addFile(QString::fromUtf8(":/EditorFramework/Icons/ObjectsVisible.svg"), QSize(), QIcon::Normal, QIcon::On);
  m_pVisibilityButton->setIcon(icon);
  m_pVisibilityButton->setCheckable(true);
  m_pVisibilityButton->setToolTip(QCoreApplication::translate("VisualScriptVariable", "Make variable public or private", nullptr));
  m_pLayout->addWidget(m_pVisibilityButton);
}

ezQtVisualScriptVariableTypeDeclarationWidget::~ezQtVisualScriptVariableTypeDeclarationWidget() = default;

void ezQtVisualScriptVariableTypeDeclarationWidget::OnInit()
{
  ezHybridArray<ezReflectionUtils::EnumKeyValuePair, 16> enumValues;

  {
    ezReflectionUtils::GetEnumKeysAndValues(ezGetStaticRTTI<ezVisualScriptVariableType>(), enumValues);
    for (auto& val : enumValues)
    {
      m_pTypeList->addItem(ezMakeQString(ezTranslate(val.m_sKey)), val.m_iValue);
    }

    connect(m_pTypeList, &QComboBox::currentIndexChanged, [this](int iIndex)
      { ChangeType(); });
  }

  {
    QActionGroup* pActionGroup = new QActionGroup(this);
    ezReflectionUtils::GetEnumKeysAndValues(ezGetStaticRTTI<ezVisualScriptVariableCategory>(), enumValues);

    QString sIcons[] = {
      ":/EditorPluginVisualScript/Icons/VariableCategoryMember.svg",
      ":/EditorPluginVisualScript/Icons/VariableCategoryArray.svg",
      ":/EditorPluginVisualScript/Icons/VariableCategoryMap.svg"};

    EZ_ASSERT_DEV(EZ_ARRAY_SIZE(sIcons) >= enumValues.GetCount(), "Need exactly one icon per category");

    for (ezUInt32 i = 0; i < enumValues.GetCount(); ++i)
    {
      QIcon icon;
      icon.addFile(sIcons[i]);

      QAction* pAction = new QAction(icon, ezMakeQString(ezTranslate(enumValues[i].m_sKey)), this);
      pAction->setCheckable(true);

      connect(pAction, &QAction::triggered, [this]()
        { ChangeType(); });

      pActionGroup->addAction(pAction);
      m_pCategoryList->addAction(pAction);
    }
  }

  connect(m_pVisibilityButton, &QPushButton::toggled, [this](bool)
    { ChangeType(); });
}

void ezQtVisualScriptVariableTypeDeclarationWidget::InternalSetValue(const ezVariant& value)
{
  auto typeDecl = value.Get<ezVisualScriptVariableTypeDeclaration>();

  {
    ezQtScopedBlockSignals bs(m_pTypeList);
    for (int i = 0; i < m_pTypeList->count(); ++i)
    {
      if (m_pTypeList->itemData(i).toInt() == typeDecl.m_Type)
      {
        m_pTypeList->setCurrentIndex(i);
        break;
      }
    }
  }

  {
    ezQtScopedBlockSignals bs(m_pCategoryList);
    QAction* pAction = m_pCategoryList->actions()[typeDecl.m_Category];
    pAction->setChecked(true);

    m_pCategoryButton->setIcon(pAction->icon());
  }

  {
    ezQtScopedBlockSignals bs(m_pVisibilityButton);
    m_pVisibilityButton->setChecked(typeDecl.m_bPublic);
  }
}

void ezQtVisualScriptVariableTypeDeclarationWidget::ChangeType()
{
  QAction* pAction = nullptr;
  ezUInt32 uiCategory = 0;
  for (auto action : m_pCategoryList->actions())
  {
    if (action->isChecked())
    {
      pAction = action;
      break;
    }
    ++uiCategory;
  }
  EZ_ASSERT_DEV(pAction != nullptr, "No category selected");

  m_pObjectAccessor->StartTransaction("Change variable type");

  for (const auto& item : m_Items)
  {
    ezVisualScriptVariableTypeDeclaration typeDecl;

    typeDecl.m_Type = static_cast<ezVisualScriptVariableType::Enum>(m_pTypeList->currentData().toInt());
    typeDecl.m_Category = static_cast<ezVisualScriptVariableCategory::Enum>(uiCategory);
    typeDecl.m_bPublic = m_pVisibilityButton->isChecked();

    m_pObjectAccessor->SetValue(item.m_pObject, m_pProp, typeDecl, item.m_Index).AssertSuccess();
  }

  m_pObjectAccessor->FinishTransaction();

  m_pCategoryButton->setIcon(pAction->icon());
}

///////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVisualScriptVariable, ezNoBase, 2, ezRTTIDefaultAllocator<ezVisualScriptVariable>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("Type", m_TypeDecl),
    EZ_MEMBER_PROPERTY("DefaultValue", m_DefaultValue)->AddAttributes(new ezDefaultValueAttribute(0), new ezVisualScriptVariableAttribute()),
    EZ_MEMBER_PROPERTY("ClampRange", m_bClampRange),
    EZ_MEMBER_PROPERTY("MinValue", m_fMinValue),
    EZ_MEMBER_PROPERTY("MaxValue", m_fMaxValue)->AddAttributes(new ezDefaultValueAttribute(1)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

// static
void ezVisualScriptVariable::ConvertDefaultValue(ezVariant& inout_defaultValue, ezVisualScriptVariableTypeDeclaration targetTypeDecl)
{
  auto ConvertOrSetToDefault = [](ezVariant& v, ezVisualScriptDataType::Enum targetType)
  {
    if (targetType == ezVisualScriptDataType::Variant)
      return;

    auto variantTargetType = ezVisualScriptDataType::GetVariantType(targetType);
    if (variantTargetType == ezVariantType::Invalid || variantTargetType == ezVariantType::TypedObject || variantTargetType == ezVariantType::TypedPointer)
    {
      v = ezVariant();
      return;
    }

    ezResult res = EZ_SUCCESS;
    v = v.ConvertTo(variantTargetType, &res);
    if (res.Failed())
    {
      v = ezReflectionUtils::GetDefaultVariantFromType(ezVisualScriptDataType::GetRtti(targetType));
    }
  };

  auto targetType = static_cast<ezVisualScriptDataType::Enum>(targetTypeDecl.m_Type.GetValue());
  if (targetTypeDecl.m_Category == ezVisualScriptVariableCategory::Array)
  {
    if (inout_defaultValue.IsA<ezVariantArray>())
    {
      ezVariantArray a = inout_defaultValue.Get<ezVariantArray>();
      for (auto& v : a)
      {
        ConvertOrSetToDefault(v, targetType);
      }
      inout_defaultValue = a;
    }
    else
    {
      ezVariantArray a;
      ConvertOrSetToDefault(inout_defaultValue, targetType);
      a.PushBack(inout_defaultValue);
      inout_defaultValue = a;
    }
  }
  else if (targetTypeDecl.m_Category == ezVisualScriptVariableCategory::Map)
  {
    if (inout_defaultValue.IsA<ezVariantDictionary>())
    {
      ezVariantDictionary d = inout_defaultValue.Get<ezVariantDictionary>();
      for (auto it = d.GetIterator(); it.IsValid(); it.Next())
      {
        ezVariant v = it.Value();
        ConvertOrSetToDefault(v, targetType);
        d[it.Key()] = v;
      }
      inout_defaultValue = d;
    }
    else
    {
      ezVariantDictionary d;
      ConvertOrSetToDefault(inout_defaultValue, targetType);
      d["Key"] = inout_defaultValue;
      inout_defaultValue = d;
    }
  }
  else
  {
    ConvertOrSetToDefault(inout_defaultValue, targetType);
  }
}

/////////////////////////////////////////////////////////////////////////////

static ezQtPropertyWidget* VisualScriptVariableTypeCreator(const ezRTTI* pRtti)
{
  return new ezQtVisualScriptVariableWidget();
}

static ezQtPropertyWidget* VisualScriptVariableTypeDeclarationCreator(const ezRTTI* pRtti)
{
  return new ezQtVisualScriptVariableTypeDeclarationWidget();
}

void ezVisualScriptVariable_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  const ezRTTI* pRtti = ezGetStaticRTTI<ezVisualScriptVariable>();

  auto& typeAccessor = e.m_pObject->GetTypeAccessor();

  if (typeAccessor.GetType() != pRtti)
    return;

  auto typeDecl = typeAccessor.GetValue("Type").Get<ezVisualScriptVariableTypeDeclaration>();
  const bool bIsPublicNumberType = typeDecl.m_bPublic && ezVisualScriptDataType::IsNumber(static_cast<ezVisualScriptDataType::Enum>(typeDecl.m_Type.GetValue()));

  auto& props = *e.m_pPropertyStates;

  auto clampRangeVisibility = bIsPublicNumberType ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  props["ClampRange"].m_Visibility = clampRangeVisibility;
  props["MinValue"].m_Visibility = clampRangeVisibility;
  props["MaxValue"].m_Visibility = clampRangeVisibility;
}

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginVisualScript, VisualScriptVariable)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation", "PropertyMetaState"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVisualScriptVariableAttribute>(), VisualScriptVariableTypeCreator);
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVisualScriptVariableTypeDeclaration>(), VisualScriptVariableTypeDeclarationCreator);

    ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezVisualScriptVariable_PropertyMetaStateEventHandler);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezVisualScriptVariableAttribute>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezVisualScriptVariableTypeDeclaration>());

    ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezVisualScriptVariable_PropertyMetaStateEventHandler);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

///////////////////////////////////////////////////////////////////////////

class ezVisualScriptVariablePatch_1_2 : public ezGraphPatch
{
public:
  ezVisualScriptVariablePatch_1_2()
    : ezGraphPatch("ezVisualScriptVariable", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto* pDefaultValue = pNode->FindProperty("DefaultValue");
    auto* pExpose = pNode->FindProperty("Expose");

    if (pDefaultValue && pExpose)
    {
      ezVisualScriptVariableTypeDeclaration typeDecl;

      ezVariantType::Enum variantType = pDefaultValue->m_Value.GetType();
      typeDecl.m_Type = static_cast<ezVisualScriptVariableType::Enum>(ezVisualScriptDataType::FromVariantType(variantType));
      typeDecl.m_Category = ezVisualScriptVariableCategory::Member;

      if (variantType == ezVariantType::VariantArray)
      {
        typeDecl.m_Type = ezVisualScriptVariableType::Variant;
        typeDecl.m_Category = ezVisualScriptVariableCategory::Array;
      }
      else if (variantType == ezVariantType::VariantDictionary)
      {
        typeDecl.m_Type = ezVisualScriptVariableType::Variant;
        typeDecl.m_Category = ezVisualScriptVariableCategory::Map;
      }

      typeDecl.m_bPublic = pExpose->m_Value.ConvertTo<bool>();

      pNode->AddProperty("Type", typeDecl);
    }
  }
};

ezVisualScriptVariablePatch_1_2 g_ezVisualScriptVariablePatch_1_2;

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezVisualScriptExpressionDataType, 1)
  EZ_ENUM_CONSTANT(ezVisualScriptExpressionDataType::Int),
  EZ_ENUM_CONSTANT(ezVisualScriptExpressionDataType::Float),
  EZ_ENUM_CONSTANT(ezVisualScriptExpressionDataType::Vector3),
  EZ_ENUM_CONSTANT(ezVisualScriptExpressionDataType::Color),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVisualScriptExpressionVariable, ezNoBase, 1, ezRTTIDefaultAllocator<ezVisualScriptExpressionVariable>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_ENUM_MEMBER_PROPERTY("Type", ezVisualScriptExpressionDataType, m_Type),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptDataType::Enum ezVisualScriptExpressionDataType::GetVisualScriptDataType(Enum dataType)
{
  switch (dataType)
  {
    case Int:
      return ezVisualScriptDataType::Int;
    case Float:
      return ezVisualScriptDataType::Float;
    case Vector3:
      return ezVisualScriptDataType::Vector3;
    case Color:
      return ezVisualScriptDataType::Color;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return ezVisualScriptDataType::Invalid;
}
