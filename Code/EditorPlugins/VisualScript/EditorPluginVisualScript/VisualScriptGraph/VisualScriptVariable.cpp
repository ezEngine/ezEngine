#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>

#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVisualScriptVariable, ezNoBase, 1, ezRTTIDefaultAllocator<ezVisualScriptVariable>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("DefaultValue", m_DefaultValue)->AddAttributes(new ezDefaultValueAttribute(0), new ezVisualScriptVariableAttribute()),
    EZ_MEMBER_PROPERTY("Expose", m_bExpose),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptVariableAttribute, 1, ezRTTIDefaultAllocator<ezVisualScriptVariableAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

static ezQtPropertyWidget* VisualScriptVariableTypeCreator(const ezRTTI* pRtti)
{
  return new ezQtVisualScriptVariableWidget();
}

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginVisualScript, VisualScriptVariable)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation", "PropertyMetaState"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezVisualScriptVariableAttribute>(), VisualScriptVariableTypeCreator);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezVisualScriptVariableAttribute>());
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezQtVisualScriptVariableWidget::ezQtVisualScriptVariableWidget() = default;
ezQtVisualScriptVariableWidget::~ezQtVisualScriptVariableWidget() = default;

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
