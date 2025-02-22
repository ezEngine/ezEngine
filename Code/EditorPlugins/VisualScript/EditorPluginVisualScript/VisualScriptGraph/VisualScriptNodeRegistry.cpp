#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptTypeDeduction.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>

#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

#include <Core/Messages/EventMessage.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdRandom.h>

namespace
{
  constexpr const char* szPluginName = "EditorPluginVisualScript";
  static ezHashedString sEventHandlerCategory = ezMakeHashedString("Add Event Handler/");
  static ezHashedString sCoroutinesCategory = ezMakeHashedString("Coroutines");
  static ezHashedString sPropertiesCategory = ezMakeHashedString("Properties");
  static ezHashedString sVariablesCategory = ezMakeHashedString("Variables");
  static ezHashedString sLogicCategory = ezMakeHashedString("Logic");
  static ezHashedString sMathCategory = ezMakeHashedString("Math");
  static ezHashedString sTypeConversionCategory = ezMakeHashedString("Type Conversion");
  static ezHashedString sArrayCategory = ezMakeHashedString("Array");
  static ezHashedString sMessagesCategory = ezMakeHashedString("Messages");
  static ezHashedString sEnumsCategory = ezMakeHashedString("Enums");

  const ezRTTI* FindTopMostBaseClass(const ezRTTI* pRtti)
  {
    const ezRTTI* pReflectedClass = ezGetStaticRTTI<ezReflectedClass>();
    while (pRtti->GetParentType() != nullptr && pRtti->GetParentType() != pReflectedClass)
    {
      pRtti = pRtti->GetParentType();
    }
    return pRtti;
  }

  void CollectFunctionArgumentAttributes(const ezAbstractFunctionProperty* pFuncProp, ezDynamicArray<const ezFunctionArgumentAttributes*>& out_attributes)
  {
    for (auto pAttr : pFuncProp->GetAttributes())
    {
      if (auto pFuncArgAttr = ezDynamicCast<const ezFunctionArgumentAttributes*>(pAttr))
      {
        ezUInt32 uiArgIndex = pFuncArgAttr->GetArgumentIndex();
        out_attributes.EnsureCount(uiArgIndex + 1);
        out_attributes[uiArgIndex] = pFuncArgAttr;
      }
    }
  }

  void AddInputProperty(ezReflectedTypeDescriptor& ref_typeDesc, ezStringView sName, const ezRTTI* pRtti, ezVisualScriptDataType::Enum scriptDataType, ezArrayPtr<const ezPropertyAttribute* const> attributes = {})
  {
    auto& propDesc = ref_typeDesc.m_Properties.ExpandAndGetRef();
    propDesc.m_sName = sName;
    propDesc.m_Flags = ezPropertyFlags::StandardType;

    for (auto pAttr : attributes)
    {
      propDesc.m_Attributes.PushBack(pAttr->GetDynamicRTTI()->GetAllocator()->Clone<ezPropertyAttribute>(pAttr));
    }

    if (pRtti->GetTypeFlags().IsSet(ezTypeFlags::IsEnum))
    {
      propDesc.m_Category = ezPropertyCategory::Member;
      propDesc.m_sType = pRtti->GetTypeName();
      propDesc.m_Flags = ezPropertyFlags::IsEnum;
    }
    else if (pRtti->GetTypeFlags().IsSet(ezTypeFlags::Bitflags))
    {
      propDesc.m_Category = ezPropertyCategory::Member;
      propDesc.m_sType = pRtti->GetTypeName();
      propDesc.m_Flags = ezPropertyFlags::Bitflags;
    }
    else
    {
      if (scriptDataType == ezVisualScriptDataType::Color)
      {
        propDesc.m_Category = ezPropertyCategory::Member;
        propDesc.m_sType = pRtti->GetTypeName();
        propDesc.m_Attributes.PushBack(EZ_DEFAULT_NEW(ezExposeColorAlphaAttribute));
      }
      else if (scriptDataType == ezVisualScriptDataType::Variant)
      {
        propDesc.m_Category = ezPropertyCategory::Member;
        propDesc.m_sType = ezGetStaticRTTI<ezVariant>()->GetTypeName();
        propDesc.m_Attributes.PushBack(EZ_DEFAULT_NEW(ezVisualScriptVariableAttribute));
      }
      else if (scriptDataType == ezVisualScriptDataType::Array)
      {
        propDesc.m_Category = ezPropertyCategory::Array;
        propDesc.m_sType = ezGetStaticRTTI<ezVariant>()->GetTypeName();
        propDesc.m_Attributes.PushBack(EZ_DEFAULT_NEW(ezVisualScriptVariableAttribute));
      }
      else if (scriptDataType == ezVisualScriptDataType::Map)
      {
        propDesc.m_Category = ezPropertyCategory::Map;
        propDesc.m_sType = ezGetStaticRTTI<ezVariant>()->GetTypeName();
        propDesc.m_Attributes.PushBack(EZ_DEFAULT_NEW(ezVisualScriptVariableAttribute));
      }
      else
      {
        propDesc.m_Category = ezPropertyCategory::Member;
        propDesc.m_sType = ezVisualScriptDataType::GetRtti(scriptDataType)->GetTypeName();
      }
    }
  }

  ezStringView StripTypeName(ezStringView sTypeName)
  {
    sTypeName.TrimWordStart("ez");
    return sTypeName;
  }

  ezStringView GetTypeName(const ezRTTI* pRtti)
  {
    ezStringView sTypeName = pRtti->GetTypeName();
    if (auto pScriptExtension = pRtti->GetAttributeByType<ezScriptExtensionAttribute>())
    {
      sTypeName = pScriptExtension->GetTypeName();
    }
    return StripTypeName(sTypeName);
  }

  ezColorGammaUB NiceColorFromName(ezStringView sTypeName, ezStringView sCategory = ezStringView())
  {
    float typeX = ezSimdRandom::FloatZeroToOne(ezSimdVec4i(ezHashingUtils::StringHash(sTypeName))).x();

    float x = typeX;
    if (sCategory.IsEmpty() == false)
    {
      x = ezSimdRandom::FloatZeroToOne(ezSimdVec4i(ezHashingUtils::StringHash(sCategory))).x();
      x += typeX * ezColorScheme::s_fIndexNormalizer;
    }

    return ezColorScheme::DarkUI(x);
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

static ezColorScheme::Enum s_scriptDataTypeToPinColor[] = {
  ezColorScheme::Gray,   // Invalid
  ezColorScheme::Red,    // Bool,
  ezColorScheme::Cyan,   // Byte,
  ezColorScheme::Teal,   // Int,
  ezColorScheme::Teal,   // Int64,
  ezColorScheme::Green,  // Float,
  ezColorScheme::Green,  // Double,
  ezColorScheme::Lime,   // Color,
  ezColorScheme::Orange, // Vector3,
  ezColorScheme::Orange, // Quaternion,
  ezColorScheme::Orange, // Transform,
  ezColorScheme::Violet, // Time,
  ezColorScheme::Green,  // Angle,
  ezColorScheme::Grape,  // String,
  ezColorScheme::Grape,  // HashedString,
  ezColorScheme::Blue,   // GameObject,
  ezColorScheme::Blue,   // Component,
  ezColorScheme::Blue,   // TypedPointer,
  ezColorScheme::Pink,   // Variant,
  ezColorScheme::Pink,   // VariantArray,
  ezColorScheme::Pink,   // VariantDictionary,
  ezColorScheme::Cyan,   // Coroutine,
};

static_assert(EZ_ARRAY_SIZE(s_scriptDataTypeToPinColor) == ezVisualScriptDataType::Count);

// static
ezColor ezVisualScriptNodeRegistry::PinDesc::GetColorForScriptDataType(ezVisualScriptDataType::Enum dataType)
{
  if (dataType == ezVisualScriptDataType::EnumValue || dataType == ezVisualScriptDataType::BitflagValue)
  {
    return ezColorScheme::DarkUI(ezColorScheme::Teal);
  }

  EZ_ASSERT_DEBUG(dataType >= 0 && dataType < EZ_ARRAY_SIZE(s_scriptDataTypeToPinColor), "Out of bounds access");
  return ezColorScheme::DarkUI(s_scriptDataTypeToPinColor[dataType]);
}

ezColor ezVisualScriptNodeRegistry::PinDesc::GetColor() const
{
  if (IsExecutionPin())
  {
    return ezColorScheme::DarkUI(ezColorScheme::Gray);
  }

  if (m_ScriptDataType > ezVisualScriptDataType::Invalid && m_ScriptDataType < ezVisualScriptDataType::Count)
  {
    return GetColorForScriptDataType(m_ScriptDataType);
  }

  if (m_ScriptDataType == ezVisualScriptDataType::EnumValue || m_ScriptDataType == ezVisualScriptDataType::BitflagValue)
  {
    return ezColorScheme::DarkUI(ezColorScheme::Teal);
  }

  if (m_ScriptDataType == ezVisualScriptDataType::Any)
  {
    return ezColorScheme::DarkUI(ezColorScheme::Gray);
  }

  return ezColorScheme::DarkUI(ezColorScheme::Blue);
}

//////////////////////////////////////////////////////////////////////////

void AddExecutionPin(ezVisualScriptNodeRegistry::NodeDesc& inout_nodeDesc, ezStringView sName, ezHashedString sDynamicPinProperty, bool bSplitExecution, ezSmallArray<ezVisualScriptNodeRegistry::PinDesc, 4>& inout_pins)
{
  auto& pin = inout_pins.ExpandAndGetRef();
  pin.m_sName.Assign(sName);
  pin.m_sDynamicPinProperty = sDynamicPinProperty;
  pin.m_pDataType = nullptr;
  pin.m_ScriptDataType = ezVisualScriptDataType::Invalid;
  pin.m_bSplitExecution = bSplitExecution;

  inout_nodeDesc.m_bHasDynamicPins |= (sDynamicPinProperty.IsEmpty() == false);
}

void ezVisualScriptNodeRegistry::NodeDesc::AddInputExecutionPin(ezStringView sName, const ezHashedString& sDynamicPinProperty /*= ezHashedString()*/)
{
  AddExecutionPin(*this, sName, sDynamicPinProperty, false, m_InputPins);

  m_bImplicitExecution = false;
}

void ezVisualScriptNodeRegistry::NodeDesc::AddOutputExecutionPin(ezStringView sName, const ezHashedString& sDynamicPinProperty /*= ezHashedString()*/, bool bSplitExecution /*= false*/)
{
  AddExecutionPin(*this, sName, sDynamicPinProperty, bSplitExecution, m_OutputPins);

  m_bImplicitExecution = false;
}

void AddDataPin(ezVisualScriptNodeRegistry::NodeDesc& inout_nodeDesc, ezStringView sName, const ezRTTI* pDataType, ezVisualScriptDataType::Enum scriptDataType, bool bRequired, ezHashedString sDynamicPinProperty, ezVisualScriptNodeRegistry::PinDesc::DeductTypeFunc deductTypeFunc, bool bReplaceWithArray, ezSmallArray<ezVisualScriptNodeRegistry::PinDesc, 4>& inout_pins)
{
  if ((scriptDataType == ezVisualScriptDataType::AnyPointer || scriptDataType == ezVisualScriptDataType::Any) && deductTypeFunc == nullptr)
  {
    deductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromNodeDataType;
  }

  auto& pin = inout_pins.ExpandAndGetRef();
  pin.m_sName.Assign(sName);
  pin.m_sDynamicPinProperty = sDynamicPinProperty;
  pin.m_DeductTypeFunc = deductTypeFunc;
  pin.m_pDataType = pDataType;
  pin.m_ScriptDataType = scriptDataType;
  pin.m_bRequired = bRequired;
  pin.m_bReplaceWithArray = bReplaceWithArray;

  inout_nodeDesc.m_bHasDynamicPins |= (sDynamicPinProperty.IsEmpty() == false);
}

void ezVisualScriptNodeRegistry::NodeDesc::AddInputDataPin(ezStringView sName, const ezRTTI* pDataType, ezVisualScriptDataType::Enum scriptDataType, bool bRequired, const ezHashedString& sDynamicPinProperty /*= ezHashedString()*/, PinDesc::DeductTypeFunc deductTypeFunc /*= nullptr*/, bool bReplaceWithArray /*= false*/)
{
  AddDataPin(*this, sName, pDataType, scriptDataType, bRequired, sDynamicPinProperty, deductTypeFunc, bReplaceWithArray, m_InputPins);
}

void ezVisualScriptNodeRegistry::NodeDesc::AddOutputDataPin(ezStringView sName, const ezRTTI* pDataType, ezVisualScriptDataType::Enum scriptDataType, const ezHashedString& sDynamicPinProperty /*= ezHashedString()*/, PinDesc::DeductTypeFunc deductTypeFunc /*= nullptr*/)
{
  AddDataPin(*this, sName, pDataType, scriptDataType, false, sDynamicPinProperty, deductTypeFunc, false, m_OutputPins);
}

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_SINGLETON(ezVisualScriptNodeRegistry);

ezVisualScriptNodeRegistry::ezVisualScriptNodeRegistry()
  : m_SingletonRegistrar(this)
{
  ezPhantomRttiManager::s_Events.AddEventHandler(ezMakeDelegate(&ezVisualScriptNodeRegistry::PhantomTypeRegistryEventHandler, this));

  UpdateNodeTypes();
}

ezVisualScriptNodeRegistry::~ezVisualScriptNodeRegistry()
{
  ezPhantomRttiManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezVisualScriptNodeRegistry::PhantomTypeRegistryEventHandler, this));
}

void ezVisualScriptNodeRegistry::PhantomTypeRegistryEventHandler(const ezPhantomRttiManagerEvent& e)
{
  if (e.m_pChangedType->GetPluginName() == "EditorPluginVisualScript")
    return;

  if ((e.m_Type == ezPhantomRttiManagerEvent::Type::TypeAdded && m_TypeToNodeDescs.Contains(e.m_pChangedType) == false) ||
      e.m_Type == ezPhantomRttiManagerEvent::Type::TypeChanged)
  {
    UpdateNodeType(e.m_pChangedType);
  }
}

void ezVisualScriptNodeRegistry::UpdateNodeTypes()
{
  EZ_PROFILE_SCOPE("Update VS Node Types");

  // Base Node Type
  if (m_pBaseType == nullptr)
  {
    ezReflectedTypeDescriptor desc;
    desc.m_sTypeName = "ezVisualScriptNodeBase";
    desc.m_sPluginName = szPluginName;
    desc.m_sParentTypeName = ezGetStaticRTTI<ezReflectedClass>()->GetTypeName();
    desc.m_Flags = ezTypeFlags::Phantom | ezTypeFlags::Abstract | ezTypeFlags::Class;

    m_pBaseType = ezPhantomRttiManager::RegisterType(desc);
  }

  if (m_bBuiltinTypesCreated == false)
  {
    CreateBuiltinTypes();
    m_bBuiltinTypesCreated = true;
  }

  auto& scriptBaseClassesDynEnum = ezDynamicStringEnum::CreateDynamicEnum("ScriptBaseClasses");

  ezRTTI::ForEachType([this](const ezRTTI* pRtti)
    { UpdateNodeType(pRtti); });
}

void ezVisualScriptNodeRegistry::UpdateNodeType(const ezRTTI* pRtti, bool bForceExpose /*= false*/)
{
  static ezHashedString sType = ezMakeHashedString("Type");
  static ezHashedString sProperty = ezMakeHashedString("Property");
  static ezHashedString sValue = ezMakeHashedString("Value");

  if (pRtti->GetAttributeByType<ezHiddenAttribute>() != nullptr || pRtti->GetAttributeByType<ezExcludeFromScript>() != nullptr)
    return;

  if (pRtti->IsDerivedFrom<ezScriptCoroutine>())
  {
    CreateCoroutineNodeType(pRtti);
  }
  else if (pRtti->IsDerivedFrom<ezMessage>())
  {
    CreateMessageNodeTypes(pRtti);
  }
  else
  {
    // expose reflected functions and properties to visual scripts
    {
      // All components should be exposed to visual scripts, furthermore all classes that have script-able functions are also exposed
      bool bExposeToVisualScript = pRtti->IsDerivedFrom<ezComponent>() || bForceExpose;
      bool bHasBaseClassFunctions = false;

      ezStringBuilder sCategory;
      {
        ezStringView sTypeName = GetTypeName(pRtti);
        const ezRTTI* pBaseClass = FindTopMostBaseClass(pRtti);
        if (pBaseClass != pRtti)
        {
          sCategory.Set(StripTypeName(pBaseClass->GetTypeName()), "/", sTypeName);
        }
        else
        {
          sCategory = sTypeName;
        }
      }

      ezHashedString sCategoryHashed;
      sCategoryHashed.Assign(sCategory);

      for (const ezAbstractFunctionProperty* pFuncProp : pRtti->GetFunctions())
      {
        auto pScriptableFunctionAttribute = pFuncProp->GetAttributeByType<ezScriptableFunctionAttribute>();
        if (pScriptableFunctionAttribute == nullptr)
          continue;

        bExposeToVisualScript = true;

        bool bIsBaseClassFunction = pFuncProp->GetAttributeByType<ezScriptBaseClassFunctionAttribute>() != nullptr;
        if (bIsBaseClassFunction)
        {
          bHasBaseClassFunctions = true;
        }

        CreateFunctionCallNodeType(pRtti, bIsBaseClassFunction ? sEventHandlerCategory : sCategoryHashed, pFuncProp, pScriptableFunctionAttribute, bIsBaseClassFunction);
      }

      if (bExposeToVisualScript)
      {
        ezStringBuilder sPropertyNodeTypeName;

        for (const ezAbstractProperty* pProp : pRtti->GetProperties())
        {
          if (pProp->GetCategory() != ezPropertyCategory::Member)
            continue;

          const ezRTTI* pPropRtti = pProp->GetSpecificType();
          if (pPropRtti->GetTypeFlags().IsSet(ezTypeFlags::IsEnum))
          {
            CreateEnumNodeTypes(pPropRtti);
          }

          ezUInt32 uiStart = m_PropertyValues.GetCount();
          m_PropertyValues.PushBack({sType, pRtti->GetTypeName()});
          m_PropertyValues.PushBack({sProperty, pProp->GetPropertyName()});
          m_PropertyValues.PushBack({sValue, ezReflectionUtils::GetDefaultValue(pProp)});

          // Setter
          {
            sPropertyNodeTypeName.Set("Set", pProp->GetPropertyName());
            m_PropertyNodeTypeNames.PushBack(sPropertyNodeTypeName);

            auto& nodeTemplate = m_NodeCreationTemplates.ExpandAndGetRef();
            nodeTemplate.m_pType = m_pSetPropertyType;
            nodeTemplate.m_sTypeName = m_PropertyNodeTypeNames.PeekBack();
            nodeTemplate.m_sCategory = sCategoryHashed;
            nodeTemplate.m_uiPropertyValuesStart = uiStart;
            nodeTemplate.m_uiPropertyValuesCount = 3;
          }

          // Getter
          {
            sPropertyNodeTypeName.Set("Get", pProp->GetPropertyName());
            m_PropertyNodeTypeNames.PushBack(sPropertyNodeTypeName);

            auto& nodeTemplate = m_NodeCreationTemplates.ExpandAndGetRef();
            nodeTemplate.m_pType = m_pGetPropertyType;
            nodeTemplate.m_sTypeName = m_PropertyNodeTypeNames.PeekBack();
            nodeTemplate.m_sCategory = sCategoryHashed;
            nodeTemplate.m_uiPropertyValuesStart = uiStart;
            nodeTemplate.m_uiPropertyValuesCount = 2;
          }
        }
      }

      if (bHasBaseClassFunctions)
      {
        auto& scriptBaseClassesDynEnum = ezDynamicStringEnum::GetDynamicEnum("ScriptBaseClasses");
        scriptBaseClassesDynEnum.AddValidValue(StripTypeName(pRtti->GetTypeName()));

        CreateGetOwnerNodeType(pRtti);
      }
    }
  }
}

ezResult ezVisualScriptNodeRegistry::GetScriptDataType(const ezRTTI* pRtti, ezVisualScriptDataType::Enum& out_scriptDataType, ezStringView sFunctionName /*= ezStringView()*/, ezStringView sArgName /*= ezStringView()*/)
{
  if (pRtti->GetTypeFlags().IsSet(ezTypeFlags::IsEnum))
  {
    CreateEnumNodeTypes(pRtti);
  }

  ezVisualScriptDataType::Enum scriptDataType = ezVisualScriptDataType::FromRtti(pRtti);
  if (scriptDataType == ezVisualScriptDataType::Invalid)
  {
    ezLog::Warning("The script function '{}' uses an argument '{}' of type '{}' which is not a valid script data type, therefore this function will not be available in visual scripts", sFunctionName, sArgName, pRtti->GetTypeName());
    return EZ_FAILURE;
  }

  out_scriptDataType = scriptDataType;
  return EZ_SUCCESS;
}

ezVisualScriptDataType::Enum ezVisualScriptNodeRegistry::GetScriptDataType(const ezAbstractProperty* pProp)
{
  if (pProp->GetCategory() == ezPropertyCategory::Member)
  {
    ezVisualScriptDataType::Enum result = ezVisualScriptDataType::Invalid;
    GetScriptDataType(pProp->GetSpecificType(), result, "Member", pProp->GetPropertyName()).IgnoreResult();
    return result;
  }
  else if (pProp->GetCategory() == ezPropertyCategory::Array)
  {
    return ezVisualScriptDataType::Array;
  }
  else if (pProp->GetCategory() == ezPropertyCategory::Map)
  {
    return ezVisualScriptDataType::Map;
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezVisualScriptDataType::Invalid;
}

template <typename T>
void ezVisualScriptNodeRegistry::AddInputDataPin(ezReflectedTypeDescriptor& ref_typeDesc, NodeDesc& ref_nodeDesc, ezStringView sName)
{
  const ezRTTI* pDataType = ezGetStaticRTTI<T>();

  ezVisualScriptDataType::Enum scriptDataType;
  EZ_VERIFY(GetScriptDataType(pDataType, scriptDataType, "", sName).Succeeded(), "Invalid script data type");

  AddInputProperty(ref_typeDesc, sName, pDataType, scriptDataType);

  ref_nodeDesc.AddInputDataPin(sName, pDataType, scriptDataType, false);
};

void ezVisualScriptNodeRegistry::AddInputDataPin_Any(ezReflectedTypeDescriptor& ref_typeDesc, NodeDesc& ref_nodeDesc, ezStringView sName, bool bRequired, bool bAddVariantProperty /*= false*/, PinDesc::DeductTypeFunc deductTypeFunc /*= nullptr*/)
{
  if (bAddVariantProperty)
  {
    AddInputProperty(ref_typeDesc, sName, ezGetStaticRTTI<ezVariant>(), ezVisualScriptDataType::Variant);
  }

  ref_nodeDesc.AddInputDataPin(sName, nullptr, ezVisualScriptDataType::Any, bRequired, ezHashedString(), deductTypeFunc);
}

template <typename T>
void ezVisualScriptNodeRegistry::AddOutputDataPin(NodeDesc& ref_nodeDesc, ezStringView sName)
{
  const ezRTTI* pDataType = ezGetStaticRTTI<T>();

  ezVisualScriptDataType::Enum scriptDataType;
  EZ_VERIFY(GetScriptDataType(pDataType, scriptDataType, "", sName).Succeeded(), "Invalid script data type");

  ref_nodeDesc.AddOutputDataPin(sName, pDataType, scriptDataType);
};

void ezVisualScriptNodeRegistry::CreateBuiltinTypes()
{
  const ezColorGammaUB logicColor = PinDesc::GetColorForScriptDataType(ezVisualScriptDataType::Invalid);
  const ezColorGammaUB mathColor = PinDesc::GetColorForScriptDataType(ezVisualScriptDataType::Int);
  const ezColorGammaUB stringColor = PinDesc::GetColorForScriptDataType(ezVisualScriptDataType::String);
  const ezColorGammaUB gameObjectColor = PinDesc::GetColorForScriptDataType(ezVisualScriptDataType::GameObject);
  const ezColorGammaUB variantColor = PinDesc::GetColorForScriptDataType(ezVisualScriptDataType::Variant);
  const ezColorGammaUB coroutineColor = PinDesc::GetColorForScriptDataType(ezVisualScriptDataType::Coroutine);

  ezReflectedTypeDescriptor typeDesc;

  // GetReflectedProperty
  {
    FillDesc(typeDesc, "GetProperty", logicColor);

    AddInputProperty(typeDesc, "Type", ezGetStaticRTTI<ezString>(), ezVisualScriptDataType::String);
    AddInputProperty(typeDesc, "Property", ezGetStaticRTTI<ezString>(), ezVisualScriptDataType::String);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "{Type}::Get {Property}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::GetReflectedProperty;
    nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromPropertyProperty;
    nodeDesc.AddInputDataPin("Object", nullptr, ezVisualScriptDataType::AnyPointer, true, ezHashedString(), &ezVisualScriptTypeDeduction::DeductFromTypeProperty);
    nodeDesc.AddOutputDataPin("Value", nullptr, ezVisualScriptDataType::Any);

    m_pGetPropertyType = RegisterNodeType(typeDesc, std::move(nodeDesc), sPropertiesCategory);
  }

  // SetReflectedProperty
  {
    FillDesc(typeDesc, "SetProperty", logicColor);

    AddInputProperty(typeDesc, "Type", ezGetStaticRTTI<ezString>(), ezVisualScriptDataType::String);
    AddInputProperty(typeDesc, "Property", ezGetStaticRTTI<ezString>(), ezVisualScriptDataType::String);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "{Type}::Set {Property} = {Value}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::SetReflectedProperty;
    nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromPropertyProperty;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Object", nullptr, ezVisualScriptDataType::AnyPointer, true, ezHashedString(), &ezVisualScriptTypeDeduction::DeductFromTypeProperty);
    AddInputDataPin_Any(typeDesc, nodeDesc, "Value", false, true);

    m_pSetPropertyType = RegisterNodeType(typeDesc, std::move(nodeDesc), sPropertiesCategory);
  }

  // Builtin_GetVariable
  {
    FillDesc(typeDesc, "Builtin_GetVariable", logicColor);

    AddInputProperty(typeDesc, "Name", ezGetStaticRTTI<ezString>(), ezVisualScriptDataType::String);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "Get {Name}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_GetVariable;
    nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromVariableNameProperty;
    nodeDesc.AddOutputDataPin("Value", nullptr, ezVisualScriptDataType::Any);

    m_pGetVariableType = RegisterNodeType(typeDesc, std::move(nodeDesc), sVariablesCategory);
  }

  // Builtin_SetVariable
  {
    FillDesc(typeDesc, "Builtin_SetVariable", logicColor);

    AddInputProperty(typeDesc, "Name", ezGetStaticRTTI<ezString>(), ezVisualScriptDataType::String);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "Set {Name} = {Value}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_SetVariable;
    nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromVariableNameProperty;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    AddInputDataPin_Any(typeDesc, nodeDesc, "Value", false, true);
    nodeDesc.AddOutputDataPin("Value", nullptr, ezVisualScriptDataType::Any);

    m_pSetVariableType = RegisterNodeType(typeDesc, std::move(nodeDesc), sVariablesCategory);
  }

  // Builtin_IncVariable, Builtin_DecVariable
  {
    ezVisualScriptNodeDescription::Type::Enum nodeTypes[] = {
      ezVisualScriptNodeDescription::Type::Builtin_IncVariable,
      ezVisualScriptNodeDescription::Type::Builtin_DecVariable,
    };

    const char* szNodeTitles[] = {
      "++ {Name}",
      "-- {Name}",
    };

    static_assert(EZ_ARRAY_SIZE(nodeTypes) == EZ_ARRAY_SIZE(szNodeTitles));

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(nodeTypes); ++i)
    {
      FillDesc(typeDesc, ezVisualScriptNodeDescription::Type::GetName(nodeTypes[i]), logicColor);

      AddInputProperty(typeDesc, "Name", ezGetStaticRTTI<ezString>(), ezVisualScriptDataType::String);

      auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, szNodeTitles[i]);
      typeDesc.m_Attributes.PushBack(pAttr);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = nodeTypes[i];
      nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromVariableNameProperty;
      nodeDesc.AddInputExecutionPin("");
      nodeDesc.AddOutputExecutionPin("");
      nodeDesc.AddOutputDataPin("Value", nullptr, ezVisualScriptDataType::Any);

      RegisterNodeType(typeDesc, std::move(nodeDesc), sVariablesCategory);
    }
  }

  // Builtin_TempVariable
  {
    FillDesc(typeDesc, "Builtin_TempVariable", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_TempVariable;
    nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromAllInputPins;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    AddInputDataPin_Any(typeDesc, nodeDesc, "Value", false, true);
    nodeDesc.AddOutputDataPin("Value", nullptr, ezVisualScriptDataType::Any);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sVariablesCategory);
  }

  // Builtin_Branch
  {
    FillDesc(typeDesc, "Builtin_Branch", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Branch;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("True");
    nodeDesc.AddOutputExecutionPin("False");

    AddInputDataPin<bool>(typeDesc, nodeDesc, "Condition");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Switch
  {
    ezVisualScriptDataType::Enum switchDataTypes[] = {
      ezVisualScriptDataType::Int64,
      ezVisualScriptDataType::HashedString,
    };

    const char* szSwitchTypeNames[] = {
      "Builtin_SwitchInt64",
      "Builtin_SwitchString",
    };

    const char* szSwitchTitles[] = {
      "Int64::Switch",
      "HashedString::Switch",
    };

    static_assert(EZ_ARRAY_SIZE(switchDataTypes) == EZ_ARRAY_SIZE(szSwitchTypeNames));
    static_assert(EZ_ARRAY_SIZE(switchDataTypes) == EZ_ARRAY_SIZE(szSwitchTitles));

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(switchDataTypes); ++i)
    {
      const ezRTTI* pValueType = ezVisualScriptDataType::GetRtti(switchDataTypes[i]);

      FillDesc(typeDesc, szSwitchTypeNames[i], logicColor);

      {
        auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
        propDesc.m_Category = ezPropertyCategory::Array;
        propDesc.m_sName = "Cases";
        propDesc.m_sType = pValueType->GetTypeName();
        propDesc.m_Flags = ezPropertyFlags::StandardType;

        auto pMaxSizeAttr = EZ_DEFAULT_NEW(ezMaxArraySizeAttribute, 16);
        propDesc.m_Attributes.PushBack(pMaxSizeAttr);

        auto pNoTempAttr = EZ_DEFAULT_NEW(ezNoTemporaryTransactionsAttribute);
        propDesc.m_Attributes.PushBack(pNoTempAttr);
      }

      auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, szSwitchTitles[i]);
      typeDesc.m_Attributes.PushBack(pAttr);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Switch;
      nodeDesc.AddInputExecutionPin("");
      nodeDesc.AddOutputExecutionPin("Case", ezMakeHashedString("Cases"));
      nodeDesc.AddOutputExecutionPin("Default");

      nodeDesc.AddInputDataPin("Value", pValueType, switchDataTypes[i], true);

      RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
    }
  }

  // Builtin_WhileLoop
  {
    FillDesc(typeDesc, "Builtin_WhileLoop", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_WhileLoop;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("LoopBody");
    nodeDesc.AddOutputExecutionPin("Completed");

    AddInputDataPin<bool>(typeDesc, nodeDesc, "Condition");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_ForLoop
  {
    FillDesc(typeDesc, "Builtin_ForLoop", logicColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "ForLoop [{FirstIndex}..{LastIndex}]");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_ForLoop;
    nodeDesc.AddInputExecutionPin("");
    AddInputDataPin<int>(typeDesc, nodeDesc, "FirstIndex");
    AddInputDataPin<int>(typeDesc, nodeDesc, "LastIndex");

    nodeDesc.AddOutputExecutionPin("LoopBody");
    AddOutputDataPin<int>(nodeDesc, "Index");
    nodeDesc.AddOutputExecutionPin("Completed");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_ForEachLoop
  {
    FillDesc(typeDesc, "Builtin_ForEachLoop", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_ForEachLoop;
    nodeDesc.AddInputExecutionPin("");
    AddInputDataPin<ezVariantArray>(typeDesc, nodeDesc, "Array");

    nodeDesc.AddOutputExecutionPin("LoopBody");
    AddOutputDataPin<ezVariant>(nodeDesc, "Element");
    AddOutputDataPin<int>(nodeDesc, "Index");
    nodeDesc.AddOutputExecutionPin("Completed");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_ReverseForEachLoop
  {
    FillDesc(typeDesc, "Builtin_ReverseForEachLoop", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop;
    nodeDesc.AddInputExecutionPin("");
    AddInputDataPin<ezVariantArray>(typeDesc, nodeDesc, "Array");

    nodeDesc.AddOutputExecutionPin("LoopBody");
    AddOutputDataPin<ezVariant>(nodeDesc, "Element");
    AddOutputDataPin<int>(nodeDesc, "Index");
    nodeDesc.AddOutputExecutionPin("Completed");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Break
  {
    FillDesc(typeDesc, "Builtin_Break", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Break;
    nodeDesc.AddInputExecutionPin("");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_And
  {
    FillDesc(typeDesc, "Builtin_And", logicColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "{A} AND {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_And;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddInputDataPin<bool>(typeDesc, nodeDesc, "B");
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Or
  {
    FillDesc(typeDesc, "Builtin_Or", logicColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "{A} OR {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Or;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddInputDataPin<bool>(typeDesc, nodeDesc, "B");
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Not
  {
    FillDesc(typeDesc, "Builtin_Not", logicColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "NOT {A}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Not;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Compare
  {
    FillDesc(typeDesc, "Builtin_Compare", logicColor);

    AddInputProperty(typeDesc, "Operator", ezGetStaticRTTI<ezComparisonOperator>(), ezVisualScriptDataType::Int64);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "{A} {Operator} {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Compare;
    nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromAllInputPins;

    AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
    AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_CompareExec
  {
    FillDesc(typeDesc, "Builtin_CompareExec", logicColor);

    AddInputProperty(typeDesc, "Operator", ezGetStaticRTTI<ezComparisonOperator>(), ezVisualScriptDataType::Int64);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "{A} {Operator} {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_CompareExec;
    nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromAllInputPins;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("True");
    nodeDesc.AddOutputExecutionPin("False");
    AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
    AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_IsValid
  {
    FillDesc(typeDesc, "Builtin_IsValid", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_IsValid;
    nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromAllInputPins;

    AddInputDataPin_Any(typeDesc, nodeDesc, "", true);
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Select
  {
    FillDesc(typeDesc, "Builtin_Select", logicColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "{Condition} ? {A} : {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Select;
    nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromAllInputPins;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "Condition");
    AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
    AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
    nodeDesc.AddOutputDataPin("", nullptr, ezVisualScriptDataType::Any);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Add, Builtin_Sub, Builtin_Mul, Builtin_Div
  {
    ezVisualScriptNodeDescription::Type::Enum mathNodeTypes[] = {
      ezVisualScriptNodeDescription::Type::Builtin_Add,
      ezVisualScriptNodeDescription::Type::Builtin_Subtract,
      ezVisualScriptNodeDescription::Type::Builtin_Multiply,
      ezVisualScriptNodeDescription::Type::Builtin_Divide,
    };

    const char* szMathNodeTitles[] = {
      "{A} + {B}",
      "{A} - {B}",
      "{A} * {B}",
      "{A} / {B}",
    };

    static_assert(EZ_ARRAY_SIZE(mathNodeTypes) == EZ_ARRAY_SIZE(szMathNodeTitles));

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(mathNodeTypes); ++i)
    {
      FillDesc(typeDesc, ezVisualScriptNodeDescription::Type::GetName(mathNodeTypes[i]), mathColor);

      auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, szMathNodeTitles[i]);
      typeDesc.m_Attributes.PushBack(pAttr);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = mathNodeTypes[i];
      nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromAllInputPins;

      AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
      AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
      nodeDesc.AddOutputDataPin("", nullptr, ezVisualScriptDataType::Any);

      RegisterNodeType(typeDesc, std::move(nodeDesc), sMathCategory);
    }
  }

  // Builtin_Expression
  {
    FillDesc(typeDesc, "Builtin_Expression", mathColor);

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = ezPropertyCategory::Member;
      propDesc.m_sName = "Expression";
      propDesc.m_sType = ezGetStaticRTTI<ezString>()->GetTypeName();
      propDesc.m_Flags = ezPropertyFlags::StandardType;

      auto pExpressionWidgetAttr = EZ_DEFAULT_NEW(ezExpressionWidgetAttribute, "Inputs", "Outputs");
      propDesc.m_Attributes.PushBack(pExpressionWidgetAttr);
    }

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = ezPropertyCategory::Array;
      propDesc.m_sName = "Inputs";
      propDesc.m_sType = ezGetStaticRTTI<ezVisualScriptExpressionVariable>()->GetTypeName();
      propDesc.m_Flags = ezPropertyFlags::Class;

      auto pMaxSizeAttr = EZ_DEFAULT_NEW(ezMaxArraySizeAttribute, 16);
      propDesc.m_Attributes.PushBack(pMaxSizeAttr);
    }

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = ezPropertyCategory::Array;
      propDesc.m_sName = "Outputs";
      propDesc.m_sType = ezGetStaticRTTI<ezVisualScriptExpressionVariable>()->GetTypeName();
      propDesc.m_Flags = ezPropertyFlags::Class;

      auto pMaxSizeAttr = EZ_DEFAULT_NEW(ezMaxArraySizeAttribute, 16);
      propDesc.m_Attributes.PushBack(pMaxSizeAttr);
    }

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "Expression::{Expression}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Expression;
    nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductDummy;

    nodeDesc.AddInputDataPin("Input", nullptr, ezVisualScriptDataType::Any, false, ezMakeHashedString("Inputs"), &ezVisualScriptTypeDeduction::DeductFromExpressionInput);
    nodeDesc.AddOutputDataPin("Output", nullptr, ezVisualScriptDataType::Any, ezMakeHashedString("Outputs"), &ezVisualScriptTypeDeduction::DeductFromExpressionOutput);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sMathCategory);
  }

  // Builtin_ToBool, Builtin_ToByte, Builtin_ToInt, Builtin_ToInt64, Builtin_ToFloat, Builtin_ToDouble, Builtin_ToString, Builtin_ToVariant,
  {
    struct ConversionNodeDesc
    {
      ezColorGammaUB m_Color;
      ezVisualScriptDataType::Enum m_DataType;
    };

    ConversionNodeDesc conversionNodeDescs[] = {
      {logicColor, ezVisualScriptDataType::Bool},
      {mathColor, ezVisualScriptDataType::Byte},
      {mathColor, ezVisualScriptDataType::Int},
      {mathColor, ezVisualScriptDataType::Int64},
      {mathColor, ezVisualScriptDataType::Float},
      {mathColor, ezVisualScriptDataType::Double},
      {stringColor, ezVisualScriptDataType::String},
      {variantColor, ezVisualScriptDataType::Variant},
    };

    for (auto& conversionNodeDesc : conversionNodeDescs)
    {
      auto nodeType = ezVisualScriptNodeDescription::Type::GetConversionType(conversionNodeDesc.m_DataType);

      FillDesc(typeDesc, ezVisualScriptNodeDescription::Type::GetName(nodeType), conversionNodeDesc.m_Color);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = nodeType;
      nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromAllInputPins;

      AddInputDataPin_Any(typeDesc, nodeDesc, "", true);
      nodeDesc.AddOutputDataPin("", ezVisualScriptDataType::GetRtti(conversionNodeDesc.m_DataType), conversionNodeDesc.m_DataType);

      RegisterNodeType(typeDesc, std::move(nodeDesc), sTypeConversionCategory);
    }
  }

  // Builtin_String_Format
  {
    FillDesc(typeDesc, "Builtin_String_Format", stringColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "String::Format {Text}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_String_Format;

    AddInputDataPin<ezString>(typeDesc, nodeDesc, "Text");
    AddInputProperty(typeDesc, "Params", ezGetStaticRTTI<ezVariantArray>(), ezVisualScriptDataType::Array);
    nodeDesc.AddInputDataPin("Params", ezGetStaticRTTI<ezVariant>(), ezVisualScriptDataType::Variant, false, ezMakeHashedString("Params"));
    AddOutputDataPin<ezString>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), ezMakeHashedString("String"));
  }

  // Builtin_Variant_ConvertTo
  {
    FillDesc(typeDesc, "Builtin_Variant_ConvertTo", variantColor);

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = ezPropertyCategory::Member;
      propDesc.m_sName = "Type";
      propDesc.m_sType = ezGetStaticRTTI<ezVisualScriptDataType>()->GetTypeName();
      propDesc.m_Flags = ezPropertyFlags::IsEnum;

      auto pAttr = EZ_DEFAULT_NEW(ezDefaultValueAttribute, ezVisualScriptDataType::Bool);
      propDesc.m_Attributes.PushBack(pAttr);
    }

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "Variant::ConvertTo {Type}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Variant_ConvertTo;
    nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromScriptDataTypeProperty;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("Succeeded");
    nodeDesc.AddOutputExecutionPin("Failed");
    nodeDesc.AddInputDataPin("Variant", ezGetStaticRTTI<ezVariant>(), ezVisualScriptDataType::Variant, true);
    nodeDesc.AddOutputDataPin("Result", nullptr, ezVisualScriptDataType::Any);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sTypeConversionCategory);
  }

  // Builtin_MakeArray
  {
    FillDesc(typeDesc, "Builtin_MakeArray", variantColor);

    ezHashedString sElements = ezMakeHashedString("Elements");
    AddInputProperty(typeDesc, sElements, ezGetStaticRTTI<ezVariant>(), ezVisualScriptDataType::Array);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_MakeArray;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin(sElements, ezGetStaticRTTI<ezVariant>(), ezVisualScriptDataType::Variant, false, sElements);
    nodeDesc.AddOutputDataPin("Array", ezGetStaticRTTI<ezVariantArray>(), ezVisualScriptDataType::Array);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_GetElement
  {
    FillDesc(typeDesc, "Builtin_Array_GetElement", variantColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "Array::GetElement[{Index}]");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Array_GetElement;

    nodeDesc.AddInputDataPin("Array", ezGetStaticRTTI<ezVariantArray>(), ezVisualScriptDataType::Array, true);
    AddInputDataPin<int>(typeDesc, nodeDesc, "Index");
    AddOutputDataPin<ezVariant>(nodeDesc, "Element");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_SetElement
  {
    FillDesc(typeDesc, "Builtin_Array_SetElement", variantColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "Array::SetElement[{Index}]");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Array_SetElement;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", ezGetStaticRTTI<ezVariantArray>(), ezVisualScriptDataType::Array, true);
    AddInputDataPin<int>(typeDesc, nodeDesc, "Index");
    AddInputDataPin<ezVariant>(typeDesc, nodeDesc, "Element");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_GetCount
  {
    FillDesc(typeDesc, "Builtin_Array::GetCount", variantColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Array_GetCount;

    nodeDesc.AddInputDataPin("Array", ezGetStaticRTTI<ezVariantArray>(), ezVisualScriptDataType::Array, true);
    AddOutputDataPin<int>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_IsEmpty
  {
    FillDesc(typeDesc, "Builtin_Array::IsEmpty", variantColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Array_IsEmpty;

    nodeDesc.AddInputDataPin("Array", ezGetStaticRTTI<ezVariantArray>(), ezVisualScriptDataType::Array, true);
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_Clear
  {
    FillDesc(typeDesc, "Builtin_Array::Clear", variantColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Array_Clear;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", ezGetStaticRTTI<ezVariantArray>(), ezVisualScriptDataType::Array, true);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_Contains
  {
    FillDesc(typeDesc, "Builtin_Array_Contains", variantColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "Array::Contains {Element}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Array_Contains;

    nodeDesc.AddInputDataPin("Array", ezGetStaticRTTI<ezVariantArray>(), ezVisualScriptDataType::Array, true);
    AddInputDataPin<ezVariant>(typeDesc, nodeDesc, "Element");
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_IndexOf
  {
    FillDesc(typeDesc, "Builtin_Array_IndexOf", variantColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "Array::IndexOf {Element}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Array_IndexOf;

    nodeDesc.AddInputDataPin("Array", ezGetStaticRTTI<ezVariantArray>(), ezVisualScriptDataType::Array, true);
    AddInputDataPin<ezVariant>(typeDesc, nodeDesc, "Element");
    AddInputDataPin<int>(typeDesc, nodeDesc, "StartIndex");
    AddOutputDataPin<int>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_Insert
  {
    FillDesc(typeDesc, "Builtin_Array::Insert", variantColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Array_Insert;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", ezGetStaticRTTI<ezVariantArray>(), ezVisualScriptDataType::Array, true);
    AddInputDataPin<ezVariant>(typeDesc, nodeDesc, "Element");
    AddInputDataPin<int>(typeDesc, nodeDesc, "Index");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_PushBack
  {
    FillDesc(typeDesc, "Builtin_Array::PushBack", variantColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Array_PushBack;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", ezGetStaticRTTI<ezVariantArray>(), ezVisualScriptDataType::Array, true);
    AddInputDataPin<ezVariant>(typeDesc, nodeDesc, "Element");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_Remove
  {
    FillDesc(typeDesc, "Builtin_Array_Remove", variantColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "Array::Remove {Element}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Array_Remove;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", ezGetStaticRTTI<ezVariantArray>(), ezVisualScriptDataType::Array, true);
    AddInputDataPin<ezVariant>(typeDesc, nodeDesc, "Element");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_RemoveAt
  {
    FillDesc(typeDesc, "Builtin_Array_RemoveAt", variantColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "Array::RemoveAt {Index}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Array_RemoveAt;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", ezGetStaticRTTI<ezVariantArray>(), ezVisualScriptDataType::Array, true);
    AddInputDataPin<int>(typeDesc, nodeDesc, "Index");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_TryGetComponentOfBaseType
  {
    FillDesc(typeDesc, "Builtin_TryGetComponentOfBaseType", gameObjectColor);

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = ezPropertyCategory::Member;
      propDesc.m_sName = "TypeName";
      propDesc.m_sType = ezGetStaticRTTI<ezString>()->GetTypeName();
      propDesc.m_Flags = ezPropertyFlags::StandardType;

      auto pAttr = EZ_DEFAULT_NEW(ezRttiTypeStringAttribute, "ezComponent");
      propDesc.m_Attributes.PushBack(pAttr);
    }

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "GameObject::TryGet {TypeName}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_TryGetComponentOfBaseType;

    nodeDesc.AddInputDataPin("GameObject", ezGetStaticRTTI<ezGameObject>(), ezVisualScriptDataType::GameObject, false);
    AddOutputDataPin<ezComponent>(nodeDesc, "Component");

    RegisterNodeType(typeDesc, std::move(nodeDesc), ezMakeHashedString("GameObject"));
  }

  // Builtin_StartCoroutine
  {
    FillDesc(typeDesc, "Builtin_StartCoroutine", coroutineColor);

    AddInputProperty(typeDesc, "CoroutineMode", ezGetStaticRTTI<ezScriptCoroutineCreationMode>(), ezVisualScriptDataType::Int64);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "StartCoroutine {Name}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_StartCoroutine;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("CoroutineBody", ezHashedString(), true);
    AddInputDataPin<ezString>(typeDesc, nodeDesc, "Name");
    nodeDesc.AddOutputDataPin("CoroutineID", ezGetStaticRTTI<ezScriptCoroutineHandle>(), ezVisualScriptDataType::Coroutine);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
  }

  // Builtin_StopCoroutine
  {
    FillDesc(typeDesc, "Builtin_StopCoroutine", coroutineColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "StopCoroutine {Name}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_StopCoroutine;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("CoroutineID", ezGetStaticRTTI<ezScriptCoroutineHandle>(), ezVisualScriptDataType::Coroutine, false);
    AddInputDataPin<ezString>(typeDesc, nodeDesc, "Name");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
  }

  // Builtin_StopAllCoroutines
  {
    FillDesc(typeDesc, "Builtin_StopAllCoroutines", coroutineColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_StopAllCoroutines;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
  }

  // Builtin_WaitForAll
  {
    ezVisualScriptNodeDescription::Type::Enum waitTypes[] = {
      ezVisualScriptNodeDescription::Type::Builtin_WaitForAll,
      ezVisualScriptNodeDescription::Type::Builtin_WaitForAny,
    };

    for (auto waitType : waitTypes)
    {
      FillDesc(typeDesc, ezVisualScriptNodeDescription::Type::GetName(waitType), coroutineColor);

      ezHashedString sCount = ezMakeHashedString("Count");
      {
        auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
        propDesc.m_Category = ezPropertyCategory::Member;
        propDesc.m_sName = sCount.GetView();
        propDesc.m_sType = ezGetStaticRTTI<ezUInt32>()->GetTypeName();
        propDesc.m_Flags = ezPropertyFlags::StandardType;

        auto pNoTempAttr = EZ_DEFAULT_NEW(ezNoTemporaryTransactionsAttribute);
        propDesc.m_Attributes.PushBack(pNoTempAttr);

        auto pDefaultAttr = EZ_DEFAULT_NEW(ezDefaultValueAttribute, 1);
        propDesc.m_Attributes.PushBack(pDefaultAttr);

        auto pClampAttr = EZ_DEFAULT_NEW(ezClampValueAttribute, 1, 16);
        propDesc.m_Attributes.PushBack(pClampAttr);
      }

      NodeDesc nodeDesc;
      nodeDesc.m_Type = waitType;

      nodeDesc.AddInputExecutionPin("");
      nodeDesc.AddOutputExecutionPin("");
      nodeDesc.AddInputDataPin("", ezGetStaticRTTI<ezScriptCoroutineHandle>(), ezVisualScriptDataType::Coroutine, false, sCount);

      RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
    }
  }

  // Builtin_Yield
  {
    FillDesc(typeDesc, "Builtin_Yield", coroutineColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Yield;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
  }
}

void ezVisualScriptNodeRegistry::CreateGetOwnerNodeType(const ezRTTI* pRtti)
{
  ezStringView sBaseClass = StripTypeName(pRtti->GetTypeName());

  ezReflectedTypeDescriptor typeDesc;
  {
    ezStringBuilder sTypeName;
    sTypeName.Set(sBaseClass, "::GetScriptOwner");

    ezColorGammaUB color = NiceColorFromName(sBaseClass);

    FillDesc(typeDesc, sTypeName, color);
  }

  NodeDesc nodeDesc;
  nodeDesc.m_sFilterByBaseClass.Assign(sBaseClass);
  nodeDesc.m_pTargetType = pRtti;
  nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::GetScriptOwner;

  ezVisualScriptDataType::Enum scriptDataType;
  if (GetScriptDataType(pRtti, scriptDataType, "GetScriptOwner", "").Failed())
    return;

  if (pRtti->IsDerivedFrom<ezComponent>())
  {
    nodeDesc.AddOutputDataPin("World", ezGetStaticRTTI<ezWorld>(), ezVisualScriptDataType::TypedPointer);
    nodeDesc.AddOutputDataPin("GameObject", ezGetStaticRTTI<ezGameObject>(), ezVisualScriptDataType::GameObject);
    nodeDesc.AddOutputDataPin("Component", ezGetStaticRTTI<ezComponent>(), ezVisualScriptDataType::Component);
  }
  else
  {
    nodeDesc.AddOutputDataPin("World", ezGetStaticRTTI<ezWorld>(), ezVisualScriptDataType::TypedPointer);
    nodeDesc.AddOutputDataPin("Owner", pRtti, scriptDataType);
  }

  ezHashedString sBaseClassHashed;
  sBaseClassHashed.Assign(sBaseClass);

  RegisterNodeType(typeDesc, std::move(nodeDesc), sBaseClassHashed);
}

void ezVisualScriptNodeRegistry::CreateFunctionCallNodeType(const ezRTTI* pRtti, const ezHashedString& sCategory, const ezAbstractFunctionProperty* pFunction, const ezScriptableFunctionAttribute* pScriptableFunctionAttribute, bool bIsEntryFunction)
{
  ezHashSet<ezStringView> dynamicPins;
  for (auto pAttribute : pFunction->GetAttributes())
  {
    if (auto pDynamicPinAttribute = ezDynamicCast<const ezDynamicPinAttribute*>(pAttribute))
    {
      dynamicPins.Insert(pDynamicPinAttribute->GetProperty());
    }
  }

  ezHybridArray<const ezFunctionArgumentAttributes*, 8> argumentAttributes;
  CollectFunctionArgumentAttributes(pFunction, argumentAttributes);

  ezStringView sTypeName = StripTypeName(pRtti->GetTypeName());

  ezStringView sFunctionName = pFunction->GetPropertyName();
  sFunctionName.TrimWordStart("Reflection_");

  ezReflectedTypeDescriptor typeDesc;
  bool bHasTitle = false;
  {
    if (bIsEntryFunction)
    {
      ezColorGammaUB color = NiceColorFromName(sTypeName);

      FillDesc(typeDesc, pRtti, &color);
    }
    else
    {
      FillDesc(typeDesc, pRtti);
    }

    ezStringBuilder temp;
    temp.Set(typeDesc.m_sTypeName, "::", sFunctionName);
    typeDesc.m_sTypeName = temp;

    if (bIsEntryFunction)
    {
      AddInputProperty(typeDesc, "CoroutineMode", ezGetStaticRTTI<ezScriptCoroutineCreationMode>(), ezVisualScriptDataType::Int64);
    }

    if (auto pTitleAttribute = pFunction->GetAttributeByType<ezTitleAttribute>())
    {
      auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, pTitleAttribute->GetTitle());
      typeDesc.m_Attributes.PushBack(pAttr);

      bHasTitle = true;
    }
  }

  NodeDesc nodeDesc;
  nodeDesc.m_pTargetType = pRtti;
  nodeDesc.m_TargetProperties.PushBack(pFunction);
  if (bIsEntryFunction)
  {
    nodeDesc.m_sFilterByBaseClass.Assign(sTypeName);
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::EntryCall;
  }
  else
  {
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::ReflectedFunction;
  }

  {
    if (pFunction->GetFlags().IsSet(ezPropertyFlags::Const) == false)
    {
      if (bIsEntryFunction == false)
      {
        nodeDesc.AddInputExecutionPin("");
      }
      nodeDesc.AddOutputExecutionPin("");
    }

    if (bIsEntryFunction == false)
    {
      if (pFunction->GetFunctionType() == ezFunctionType::Member)
      {
        // GameObject and World pins will default to the script owner's game object/world thus they are not required
        const bool bRequired = pRtti->IsDerivedFrom<ezGameObject>() == false && pRtti->IsDerivedFrom<ezWorld>() == false;
        nodeDesc.AddInputDataPin(sTypeName, pRtti, ezVisualScriptDataType::FromRtti(pRtti), bRequired);
      }

      if (const ezRTTI* pReturnRtti = pFunction->GetReturnType())
      {
        ezVisualScriptDataType::Enum scriptDataType;
        if (GetScriptDataType(pReturnRtti, scriptDataType, pFunction->GetPropertyName(), "return value").Failed())
        {
          return;
        }

        if (m_ExposedTypes.Insert(pReturnRtti) == false)
          UpdateNodeType(pReturnRtti, true);

        nodeDesc.AddOutputDataPin("Result", pReturnRtti, scriptDataType);
      }
    }

    EZ_ASSERT_ALWAYS(pFunction->GetArgumentCount() == pScriptableFunctionAttribute->GetArgumentCount(),
      "The function reflection for '{}::{}' does not match the actual signature. Num arguments: {}, reflected arguments: {}.", sTypeName, sFunctionName, pFunction->GetArgumentCount(), pScriptableFunctionAttribute->GetArgumentCount());

    ezUInt32 titleArgIdx = ezInvalidIndex;

    ezStringBuilder sArgName;
    for (ezUInt32 argIdx = 0; argIdx < pFunction->GetArgumentCount(); ++argIdx)
    {
      sArgName = pScriptableFunctionAttribute->GetArgumentName(argIdx);
      if (sArgName.IsEmpty())
        sArgName.SetFormat("Arg{}", argIdx);

      auto pArgRtti = pFunction->GetArgumentType(argIdx);
      auto argType = pScriptableFunctionAttribute->GetArgumentType(argIdx);
      const bool bIsDynamicPinProperty = dynamicPins.Contains(sArgName);

      ezHashedString sDynamicPinProperty;
      if (bIsDynamicPinProperty)
      {
        sDynamicPinProperty.Assign(sArgName);
      }

      ezVisualScriptDataType::Enum scriptDataType;
      if (GetScriptDataType(pArgRtti, scriptDataType, pFunction->GetPropertyName(), sArgName).Failed())
      {
        return;
      }

      ezVisualScriptDataType::Enum pinScriptDataType = scriptDataType;
      const bool bIsArrayDynamicPinProperty = bIsDynamicPinProperty && scriptDataType == ezVisualScriptDataType::Array;
      if (bIsArrayDynamicPinProperty)
      {
        pArgRtti = ezGetStaticRTTI<ezVariant>();
        pinScriptDataType = ezVisualScriptDataType::Variant;
      }

      if (m_ExposedTypes.Insert(pArgRtti) == false)
        UpdateNodeType(pArgRtti, true);

      if (bIsEntryFunction)
      {
        nodeDesc.AddOutputDataPin(sArgName, pArgRtti, scriptDataType);
      }
      else
      {
        if (argType == ezScriptableFunctionAttribute::In || argType == ezScriptableFunctionAttribute::Inout)
        {
          if (ezVisualScriptDataType::IsPointer(scriptDataType) == false)
          {
            ezArrayPtr<const ezPropertyAttribute* const> attributes;
            if (argIdx < argumentAttributes.GetCount() && argumentAttributes[argIdx] != nullptr)
            {
              attributes = argumentAttributes[argIdx]->GetArgumentAttributes();
            }

            AddInputProperty(typeDesc, sArgName, pArgRtti, scriptDataType, attributes);
          }

          nodeDesc.AddInputDataPin(sArgName, pArgRtti, pinScriptDataType, false, sDynamicPinProperty, nullptr, bIsArrayDynamicPinProperty);

          if (titleArgIdx == ezInvalidIndex &&
              (pinScriptDataType == ezVisualScriptDataType::String || pinScriptDataType == ezVisualScriptDataType::HashedString))
          {
            titleArgIdx = argIdx;
          }
        }
        else if (argType == ezScriptableFunctionAttribute::Out || argType == ezScriptableFunctionAttribute::Inout)
        {
          // ezLog::Error("Script function out parameter are not yet supported");
          return;

#if 0
          if (!pFunction->GetArgumentFlags(argIdx).IsSet(ezPropertyFlags::Reference))
          {
            // TODO: ezPropertyFlags::Reference is also set for const-ref parameters, should we change that ?

            ezLog::Error("Script function '{}' argument {} is marked 'out' but is not a non-const reference value", pRtti->GetTypeName(), argIdx);
            return;
          }

          nodeDesc.AddOutputDataPin(sArgName, pArgRtti, scriptDataType, sDynamicPinProperty);
#endif
        }
      }
    }

    if (bIsEntryFunction)
    {
      nodeDesc.AddOutputDataPin("CoroutineID", ezGetStaticRTTI<ezScriptCoroutineHandle>(), ezVisualScriptDataType::Coroutine);
    }

    if (bHasTitle == false && titleArgIdx != ezInvalidIndex)
    {
      ezStringBuilder sTitle;
      sTitle.Set(GetTypeName(pRtti), "::", sFunctionName, " {", pScriptableFunctionAttribute->GetArgumentName(titleArgIdx), "}");

      auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, sTitle);
      typeDesc.m_Attributes.PushBack(pAttr);
    }
  }

  RegisterNodeType(typeDesc, std::move(nodeDesc), sCategory);
}

void ezVisualScriptNodeRegistry::CreateCoroutineNodeType(const ezRTTI* pRtti)
{
  if (pRtti->GetTypeFlags().IsSet(ezTypeFlags::Abstract))
    return;

  const ezAbstractFunctionProperty* pStartFunc = nullptr;
  const ezScriptableFunctionAttribute* pScriptableFuncAttribute = nullptr;
  for (auto pFunc : pRtti->GetFunctions())
  {
    if (ezStringUtils::IsEqual(pFunc->GetPropertyName(), "Start"))
    {
      if (auto pAttr = pFunc->GetAttributeByType<ezScriptableFunctionAttribute>())
      {
        pStartFunc = pFunc;
        pScriptableFuncAttribute = pAttr;
        break;
      }
    }
  }

  if (pStartFunc == nullptr || pScriptableFuncAttribute == nullptr)
  {
    ezLog::Warning("The script coroutine '{}' has no reflected script function called 'Start'.", pRtti->GetTypeName());
    return;
  }

  ezReflectedTypeDescriptor typeDesc;
  {
    const ezColorGammaUB coroutineColor = PinDesc::GetColorForScriptDataType(ezVisualScriptDataType::Coroutine);
    FillDesc(typeDesc, pRtti, &coroutineColor);

    ezStringBuilder temp;
    temp.Set("Coroutine::", typeDesc.m_sTypeName);
    typeDesc.m_sTypeName = temp;

    if (auto pTitleAttribute = pRtti->GetAttributeByType<ezTitleAttribute>())
    {
      auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, pTitleAttribute->GetTitle());
      typeDesc.m_Attributes.PushBack(pAttr);
    }
  }

  NodeDesc nodeDesc;
  nodeDesc.m_pTargetType = pRtti;
  nodeDesc.m_TargetProperties.PushBack(pStartFunc);
  nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::InplaceCoroutine;

  nodeDesc.AddInputExecutionPin("");
  nodeDesc.AddOutputExecutionPin("Succeeded");
  nodeDesc.AddOutputExecutionPin("Failed");

  ezStringBuilder sArgName;
  for (ezUInt32 argIdx = 0; argIdx < pStartFunc->GetArgumentCount(); ++argIdx)
  {
    sArgName = pScriptableFuncAttribute->GetArgumentName(argIdx);
    if (sArgName.IsEmpty())
      sArgName.SetFormat("Arg{}", argIdx);

    auto pArgRtti = pStartFunc->GetArgumentType(argIdx);
    auto argType = pScriptableFuncAttribute->GetArgumentType(argIdx);
    if (argType != ezScriptableFunctionAttribute::In)
    {
      // ezLog::Error("Script function out parameter are not yet supported");
      return;
    }

    ezVisualScriptDataType::Enum scriptDataType = ezVisualScriptDataType::Invalid;
    if (GetScriptDataType(pArgRtti, scriptDataType, pStartFunc->GetPropertyName(), sArgName).Failed())
    {
      return;
    }

    if (ezVisualScriptDataType::IsPointer(scriptDataType) == false)
    {
      AddInputProperty(typeDesc, sArgName, pArgRtti, scriptDataType);
    }

    nodeDesc.AddInputDataPin(sArgName, pArgRtti, scriptDataType, false);
  }

  RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
}

void ezVisualScriptNodeRegistry::CreateMessageNodeTypes(const ezRTTI* pRtti)
{
  if (pRtti == ezGetStaticRTTI<ezMessage>() ||
      pRtti == ezGetStaticRTTI<ezEventMessage>() ||
      pRtti->GetTypeFlags().IsSet(ezTypeFlags::Abstract))
    return;

  ezStringView sTypeName = GetTypeName(pRtti);

  // Message Handler
  {
    ezReflectedTypeDescriptor typeDesc;
    {
      FillDesc(typeDesc, pRtti);

      ezStringBuilder temp;
      temp.Set(s_szTypeNamePrefix, "On", sTypeName);
      typeDesc.m_sTypeName = temp;

      AddInputProperty(typeDesc, "CoroutineMode", ezGetStaticRTTI<ezScriptCoroutineCreationMode>(), ezVisualScriptDataType::Int64);
    }

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::MessageHandler;

    nodeDesc.AddOutputExecutionPin("");

    ezHybridArray<const ezAbstractProperty*, 32> properties;
    pRtti->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      auto pPropRtti = pProp->GetSpecificType();
      ezVisualScriptDataType::Enum scriptDataType = GetScriptDataType(pProp);
      if (scriptDataType == ezVisualScriptDataType::Invalid)
        continue;

      nodeDesc.AddOutputDataPin(pProp->GetPropertyName(), pPropRtti, scriptDataType);

      nodeDesc.m_TargetProperties.PushBack(pProp);
    }

    nodeDesc.AddOutputDataPin("CoroutineID", ezGetStaticRTTI<ezScriptCoroutineHandle>(), ezVisualScriptDataType::Coroutine);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sEventHandlerCategory);
  }

  // Message Sender
  {
    ezReflectedTypeDescriptor typeDesc;
    {
      FillDesc(typeDesc, pRtti);

      ezStringBuilder temp;
      temp.Set(s_szTypeNamePrefix, "Send", sTypeName);
      typeDesc.m_sTypeName = temp;

      temp.Set("Send{?SendMode}", sTypeName, " {Delay}");
      auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, temp);
      typeDesc.m_Attributes.PushBack(pAttr);
    }

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::SendMessage;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("GameObject", ezGetStaticRTTI<ezGameObject>(), ezVisualScriptDataType::GameObject, false);
    nodeDesc.AddInputDataPin("Component", ezGetStaticRTTI<ezComponent>(), ezVisualScriptDataType::Component, false);
    AddInputDataPin<ezVisualScriptSendMessageMode>(typeDesc, nodeDesc, "SendMode");
    AddInputDataPin<ezTime>(typeDesc, nodeDesc, "Delay");

    ezHybridArray<const ezAbstractProperty*, 32> properties;
    pRtti->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
        continue;

      auto szPropName = pProp->GetPropertyName();
      auto pPropRtti = pProp->GetSpecificType();
      ezVisualScriptDataType::Enum scriptDataType = GetScriptDataType(pProp);
      if (scriptDataType == ezVisualScriptDataType::Invalid)
        continue;

      if (ezVisualScriptDataType::IsPointer(scriptDataType) == false)
      {
        AddInputProperty(typeDesc, szPropName, pPropRtti, scriptDataType);
      }

      nodeDesc.AddInputDataPin(szPropName, pPropRtti, scriptDataType, false);
      nodeDesc.AddOutputDataPin(szPropName, pPropRtti, scriptDataType);

      nodeDesc.m_TargetProperties.PushBack(pProp);
    }

    RegisterNodeType(typeDesc, std::move(nodeDesc), sMessagesCategory);
  }
}

void ezVisualScriptNodeRegistry::CreateEnumNodeTypes(const ezRTTI* pRtti)
{
  if (m_ExposedTypes.Insert(pRtti))
    return;

  ezStringView sTypeName = GetTypeName(pRtti);
  ezColorGammaUB enumColor = PinDesc::GetColorForScriptDataType(ezVisualScriptDataType::EnumValue);

  // Value
  {
    ezStringBuilder sFullTypeName;
    sFullTypeName.Set(sTypeName, "Value");

    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, sFullTypeName, enumColor);
    AddInputProperty(typeDesc, "Value", pRtti, ezVisualScriptDataType::EnumValue);

    ezStringBuilder sTitle;
    sTitle.Set(sTypeName, "::{Value}");

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, sTitle);
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Constant;
    nodeDesc.AddOutputDataPin("Value", pRtti, ezVisualScriptDataType::EnumValue);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sEnumsCategory);
  }

  // Switch
  {
    ezStringBuilder sFullTypeName;
    sFullTypeName.Set(sTypeName, "Switch");

    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, sFullTypeName, enumColor);

    ezStringBuilder sTitle;
    sTitle.Set(sTypeName, "::Switch");

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, sTitle);
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Switch;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddInputDataPin("Value", pRtti, ezVisualScriptDataType::EnumValue, false);

    ezHybridArray<ezReflectionUtils::EnumKeyValuePair, 16> enumKeysAndValues;
    ezReflectionUtils::GetEnumKeysAndValues(pRtti, enumKeysAndValues, ezReflectionUtils::EnumConversionMode::ValueNameOnly);
    for (auto& keyAndValue : enumKeysAndValues)
    {
      nodeDesc.AddOutputExecutionPin(keyAndValue.m_sKey);
    }

    RegisterNodeType(typeDesc, std::move(nodeDesc), sEnumsCategory);
  }
}

void ezVisualScriptNodeRegistry::FillDesc(ezReflectedTypeDescriptor& desc, const ezRTTI* pRtti, const ezColorGammaUB* pColorOverride /*= nullptr */)
{
  ezStringBuilder sTypeName = GetTypeName(pRtti);
  const ezRTTI* pBaseClass = FindTopMostBaseClass(pRtti);

  ezColorGammaUB color;
  if (pColorOverride == nullptr)
  {
    if (pBaseClass != pRtti)
    {
      color = NiceColorFromName(sTypeName, StripTypeName(pBaseClass->GetTypeName()));
    }
    else
    {
      auto scriptDataType = ezVisualScriptDataType::FromRtti(pRtti);
      if (scriptDataType != ezVisualScriptDataType::Invalid &&
          scriptDataType != ezVisualScriptDataType::Component &&
          scriptDataType != ezVisualScriptDataType::TypedPointer)
      {
        color = PinDesc::GetColorForScriptDataType(scriptDataType);
      }
      else
      {
        color = NiceColorFromName(sTypeName);
      }
    }
  }
  else
  {
    color = *pColorOverride;
  }

  FillDesc(desc, sTypeName, color);
}

void ezVisualScriptNodeRegistry::FillDesc(ezReflectedTypeDescriptor& desc, ezStringView sTypeName, const ezColorGammaUB& color)
{
  ezStringBuilder sTypeNameFull;
  sTypeNameFull.Set(s_szTypeNamePrefix, sTypeName);

  desc = {};
  desc.m_sTypeName = sTypeNameFull;
  desc.m_sPluginName = szPluginName;
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags = ezTypeFlags::Phantom | ezTypeFlags::Class;

  // Color
  {
    auto pAttr = EZ_DEFAULT_NEW(ezColorAttribute, color);
    desc.m_Attributes.PushBack(pAttr);
  }
}

const ezRTTI* ezVisualScriptNodeRegistry::RegisterNodeType(ezReflectedTypeDescriptor& typeDesc, NodeDesc&& nodeDesc, const ezHashedString& sCategory)
{
  const ezRTTI* pRtti = ezPhantomRttiManager::RegisterType(typeDesc);
  m_TypeToNodeDescs.Insert(pRtti, std::move(nodeDesc));

  auto& nodeTemplate = m_NodeCreationTemplates.ExpandAndGetRef();
  nodeTemplate.m_pType = pRtti;
  nodeTemplate.m_sCategory = sCategory;

  return pRtti;
}
