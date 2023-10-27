#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptTypeDeduction.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>

#include <Core/Messages/EventMessage.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

namespace
{
  constexpr const char* szPluginName = "EditorPluginVisualScript";
  constexpr const char* szEventHandlerCategory = "Add Event Handler/";
  constexpr const char* szCoroutinesCategory = "Coroutines";
  constexpr const char* szEnumsCategory = "Enums";

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
    else
    {
      if (scriptDataType == ezVisualScriptDataType::Variant)
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
  if (dataType == ezVisualScriptDataType::EnumValue)
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

  if (m_ScriptDataType == ezVisualScriptDataType::EnumValue)
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

void AddDataPin(ezVisualScriptNodeRegistry::NodeDesc& inout_nodeDesc, ezStringView sName, const ezRTTI* pDataType, ezVisualScriptDataType::Enum scriptDataType, bool bRequired, ezHashedString sDynamicPinProperty, ezVisualScriptNodeRegistry::PinDesc::DeductTypeFunc deductTypeFunc, ezSmallArray<ezVisualScriptNodeRegistry::PinDesc, 4>& inout_pins)
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

  inout_nodeDesc.m_bHasDynamicPins |= (sDynamicPinProperty.IsEmpty() == false);
}

void ezVisualScriptNodeRegistry::NodeDesc::AddInputDataPin(ezStringView sName, const ezRTTI* pDataType, ezVisualScriptDataType::Enum scriptDataType, bool bRequired, const ezHashedString& sDynamicPinProperty /*= ezHashedString()*/, PinDesc::DeductTypeFunc deductTypeFunc /*= nullptr*/)
{
  AddDataPin(*this, sName, pDataType, scriptDataType, bRequired, sDynamicPinProperty, deductTypeFunc, m_InputPins);
}

void ezVisualScriptNodeRegistry::NodeDesc::AddOutputDataPin(ezStringView sName, const ezRTTI* pDataType, ezVisualScriptDataType::Enum scriptDataType, const ezHashedString& sDynamicPinProperty /*= ezHashedString()*/, PinDesc::DeductTypeFunc deductTypeFunc /*= nullptr*/)
{
  AddDataPin(*this, sName, pDataType, scriptDataType, false, sDynamicPinProperty, deductTypeFunc, m_OutputPins);
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

  auto& componentTypesDynEnum = ezDynamicStringEnum::CreateDynamicEnum("ComponentTypes");
  auto& scriptBaseClassesDynEnum = ezDynamicStringEnum::CreateDynamicEnum("ScriptBaseClasses");

  ezRTTI::ForEachType([this](const ezRTTI* pRtti)
    { UpdateNodeType(pRtti); });
}

void ezVisualScriptNodeRegistry::UpdateNodeType(const ezRTTI* pRtti)
{
  if (pRtti->GetAttributeByType<ezHiddenAttribute>() != nullptr || pRtti->GetAttributeByType<ezExcludeFromScript>() != nullptr)
    return;

  if (pRtti->IsDerivedFrom<ezComponent>())
  {
    auto& componentTypesDynEnum = ezDynamicStringEnum::GetDynamicEnum("ComponentTypes");
    componentTypesDynEnum.AddValidValue(pRtti->GetTypeName(), true);
  }

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
      bool bExposeToVisualScript = false;
      bool bHasBaseClassFunctions = false;

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

        CreateFunctionCallNodeType(pRtti, pFuncProp, pScriptableFunctionAttribute, bIsBaseClassFunction);
      }

      if (bExposeToVisualScript)
      {
        for (const ezAbstractProperty* pProp : pRtti->GetProperties())
        {
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
    GetScriptDataType(pProp->GetSpecificType(), result).IgnoreResult();
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

  // GetReflectedProperty
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "GetProperty", "Properties", logicColor);

    AddInputProperty(typeDesc, "Type", ezGetStaticRTTI<ezString>(), ezVisualScriptDataType::String);
    AddInputProperty(typeDesc, "Property", ezGetStaticRTTI<ezString>(), ezVisualScriptDataType::String);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "{Type}::Get {Property}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::GetReflectedProperty;
    nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromPropertyProperty;
    nodeDesc.AddInputDataPin("Object", nullptr, ezVisualScriptDataType::AnyPointer, true, ezHashedString(), &ezVisualScriptTypeDeduction::DeductFromTypeProperty);
    nodeDesc.AddOutputDataPin("Value", nullptr, ezVisualScriptDataType::Any);

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // SetReflectedProperty
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "SetProperty", "Properties", logicColor);

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

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_GetVariable
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_GetVariable", "Variables", logicColor);

    AddInputProperty(typeDesc, "Name", ezGetStaticRTTI<ezString>(), ezVisualScriptDataType::String);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "Get {Name}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_GetVariable;
    nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromVariableNameProperty;
    nodeDesc.AddOutputDataPin("Value", nullptr, ezVisualScriptDataType::Any);

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_SetVariable
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_SetVariable", "Variables", logicColor);

    AddInputProperty(typeDesc, "Name", ezGetStaticRTTI<ezString>(), ezVisualScriptDataType::String);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "Set {Name} = {Value}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_SetVariable;
    nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromVariableNameProperty;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    AddInputDataPin_Any(typeDesc, nodeDesc, "Value", false, true);

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
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
      ezReflectedTypeDescriptor typeDesc;
      FillDesc(typeDesc, ezVisualScriptNodeDescription::Type::GetName(nodeTypes[i]), "Variables", logicColor);

      AddInputProperty(typeDesc, "Name", ezGetStaticRTTI<ezString>(), ezVisualScriptDataType::String);

      auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, szNodeTitles[i]);
      typeDesc.m_Attributes.PushBack(pAttr);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = nodeTypes[i];
      nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromVariableNameProperty;
      nodeDesc.AddInputExecutionPin("");
      nodeDesc.AddOutputExecutionPin("");
      nodeDesc.AddOutputDataPin("Value", nullptr, ezVisualScriptDataType::Any);

      m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

  // Builtin_Branch
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Branch", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Branch;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("True");
    nodeDesc.AddOutputExecutionPin("False");

    AddInputDataPin<bool>(typeDesc, nodeDesc, "Condition");

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
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

      ezReflectedTypeDescriptor typeDesc;
      FillDesc(typeDesc, szSwitchTypeNames[i], "Logic", logicColor);

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

      m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

  // Builtin_WhileLoop
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_WhileLoop", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_WhileLoop;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("LoopBody");
    nodeDesc.AddOutputExecutionPin("Completed");

    AddInputDataPin<bool>(typeDesc, nodeDesc, "Condition");

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_ForLoop
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_ForLoop", "Logic", logicColor);

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

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Break
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Break", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Break;
    nodeDesc.AddInputExecutionPin("");

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_And
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_And", "Logic", logicColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "{A} AND {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_And;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddInputDataPin<bool>(typeDesc, nodeDesc, "B");
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Or
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Or", "Logic", logicColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "{A} OR {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Or;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddInputDataPin<bool>(typeDesc, nodeDesc, "B");
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Not
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Not", "Logic", logicColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "NOT {A}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Not;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Compare
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Compare", "Logic", logicColor);

    AddInputProperty(typeDesc, "Operator", ezGetStaticRTTI<ezComparisonOperator>(), ezVisualScriptDataType::Int64);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "{A} {Operator} {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Compare;
    nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromAllInputPins;

    AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
    AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_CompareExec
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_CompareExec", "Logic", logicColor);

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

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_IsValid
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_IsValid", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_IsValid;
    nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromAllInputPins;

    AddInputDataPin_Any(typeDesc, nodeDesc, "", true);
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
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
      ezReflectedTypeDescriptor typeDesc;
      FillDesc(typeDesc, ezVisualScriptNodeDescription::Type::GetName(mathNodeTypes[i]), "Math", mathColor);

      auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, szMathNodeTitles[i]);
      typeDesc.m_Attributes.PushBack(pAttr);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = mathNodeTypes[i];
      nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromAllInputPins;

      AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
      AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
      nodeDesc.AddOutputDataPin("", nullptr, ezVisualScriptDataType::Any);

      m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

  // Builtin_ToBool, Builtin_ToByte, Builtin_ToInt, Builtin_ToInt64, Builtin_ToFloat, Builtin_ToDouble, Builtin_ToString, Builtin_ToVariant,
  {
    struct ConversionNodeDesc
    {
      const char* m_szCategory;
      ezColorGammaUB m_Color;
      ezVisualScriptDataType::Enum m_DataType;
    };

    ConversionNodeDesc conversionNodeDescs[] = {
      {"Type Conversion", logicColor, ezVisualScriptDataType::Bool},
      {"Type Conversion", mathColor, ezVisualScriptDataType::Byte},
      {"Type Conversion", mathColor, ezVisualScriptDataType::Int},
      {"Type Conversion", mathColor, ezVisualScriptDataType::Int64},
      {"Type Conversion", mathColor, ezVisualScriptDataType::Float},
      {"Type Conversion", mathColor, ezVisualScriptDataType::Double},
      {"Type Conversion", stringColor, ezVisualScriptDataType::String},
      {"Type Conversion", variantColor, ezVisualScriptDataType::Variant},
    };

    for (auto& conversionNodeDesc : conversionNodeDescs)
    {
      auto nodeType = ezVisualScriptNodeDescription::Type::GetConversionType(conversionNodeDesc.m_DataType);

      ezReflectedTypeDescriptor typeDesc;
      FillDesc(typeDesc, ezVisualScriptNodeDescription::Type::GetName(nodeType), conversionNodeDesc.m_szCategory, conversionNodeDesc.m_Color);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = nodeType;
      nodeDesc.m_DeductTypeFunc = &ezVisualScriptTypeDeduction::DeductFromAllInputPins;

      AddInputDataPin_Any(typeDesc, nodeDesc, "", true);
      nodeDesc.AddOutputDataPin("", ezVisualScriptDataType::GetRtti(conversionNodeDesc.m_DataType), conversionNodeDesc.m_DataType);

      m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

  // Builtin_String_Format,
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_String_Format", "String", stringColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "String::Format {Text}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_String_Format;

    AddInputDataPin<ezString>(typeDesc, nodeDesc, "Text");
    AddInputProperty(typeDesc, "Params", ezGetStaticRTTI<ezVariantArray>(), ezVisualScriptDataType::Array);
    nodeDesc.AddInputDataPin("Params", ezGetStaticRTTI<ezVariant>(), ezVisualScriptDataType::Variant, false, ezMakeHashedString("Params"));
    AddOutputDataPin<ezString>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Variant_ConvertTo
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Variant_ConvertTo", "Type Conversion", variantColor);

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

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_MakeArray
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_MakeArray", "Array", variantColor);

    ezHashedString sCount = ezMakeHashedString("Count");
    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = ezPropertyCategory::Member;
      propDesc.m_sName = sCount.GetView();
      propDesc.m_sType = ezGetStaticRTTI<ezUInt32>()->GetTypeName();
      propDesc.m_Flags = ezPropertyFlags::StandardType;

      auto pNoTempAttr = EZ_DEFAULT_NEW(ezNoTemporaryTransactionsAttribute);
      propDesc.m_Attributes.PushBack(pNoTempAttr);

      auto pClampAttr = EZ_DEFAULT_NEW(ezClampValueAttribute, 0, 16);
      propDesc.m_Attributes.PushBack(pClampAttr);
    }

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_MakeArray;

    nodeDesc.AddInputDataPin("", ezGetStaticRTTI<ezVariant>(), ezVisualScriptDataType::Variant, false, sCount);
    nodeDesc.AddOutputDataPin("", ezGetStaticRTTI<ezVariantArray>(), ezVisualScriptDataType::Array);

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_TryGetComponentOfBaseType
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_TryGetComponentOfBaseType", "GameObject", gameObjectColor);

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = ezPropertyCategory::Member;
      propDesc.m_sName = "TypeName";
      propDesc.m_sType = ezGetStaticRTTI<ezString>()->GetTypeName();
      propDesc.m_Flags = ezPropertyFlags::StandardType;

      auto pAttr = EZ_DEFAULT_NEW(ezDynamicStringEnumAttribute, "ComponentTypes");
      propDesc.m_Attributes.PushBack(pAttr);
    }

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "GameObject::TryGetComponentOfBaseType {TypeName}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_TryGetComponentOfBaseType;

    nodeDesc.AddInputDataPin("GameObject", ezGetStaticRTTI<ezGameObject>(), ezVisualScriptDataType::GameObject, false);
    AddOutputDataPin<ezComponent>(nodeDesc, "Component");

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_StartCoroutine
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_StartCoroutine", szCoroutinesCategory, coroutineColor);

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

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_StopCoroutine
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_StopCoroutine", szCoroutinesCategory, coroutineColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "StopCoroutine {Name}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_StopCoroutine;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("CoroutineID", ezGetStaticRTTI<ezScriptCoroutineHandle>(), ezVisualScriptDataType::Coroutine, false);
    AddInputDataPin<ezString>(typeDesc, nodeDesc, "Name");

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_StopAllCoroutines
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_StopAllCoroutines", szCoroutinesCategory, coroutineColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_StopAllCoroutines;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_WaitForAll
  {
    ezVisualScriptNodeDescription::Type::Enum waitTypes[] = {
      ezVisualScriptNodeDescription::Type::Builtin_WaitForAll,
      ezVisualScriptNodeDescription::Type::Builtin_WaitForAny,
    };

    for (auto waitType : waitTypes)
    {
      ezReflectedTypeDescriptor typeDesc;
      FillDesc(typeDesc, ezVisualScriptNodeDescription::Type::GetName(waitType), szCoroutinesCategory, coroutineColor);

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

      m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

  // Builtin_Yield
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Yield", szCoroutinesCategory, coroutineColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Yield;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }
}

void ezVisualScriptNodeRegistry::CreateGetOwnerNodeType(const ezRTTI* pRtti)
{
  ezStringView sBaseClass = StripTypeName(pRtti->GetTypeName());

  ezReflectedTypeDescriptor typeDesc;
  {
    ezStringBuilder sTypeName;
    sTypeName.Set(sBaseClass, "::GetScriptOwner");

    ezStringBuilder sCategory;
    sCategory.Set(sBaseClass);

    ezColorGammaUB color = NiceColorFromName(sBaseClass);

    FillDesc(typeDesc, sTypeName, sCategory, color);
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

  m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
}

void ezVisualScriptNodeRegistry::CreateFunctionCallNodeType(const ezRTTI* pRtti, const ezAbstractFunctionProperty* pFunction, const ezScriptableFunctionAttribute* pScriptableFunctionAttribute, bool bIsEntryFunction)
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
      ezStringBuilder sCategory;
      sCategory.Set(szEventHandlerCategory, sTypeName);

      ezColorGammaUB color = NiceColorFromName(sTypeName);

      FillDesc(typeDesc, pRtti, sCategory, &color);
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

        nodeDesc.AddOutputDataPin("Result", pReturnRtti, scriptDataType);
      }
    }

    ezUInt32 titleArgIdx = ezInvalidIndex;

    ezStringBuilder sArgName;
    for (ezUInt32 argIdx = 0; argIdx < pFunction->GetArgumentCount(); ++argIdx)
    {
      sArgName = pScriptableFunctionAttribute->GetArgumentName(argIdx);
      if (sArgName.IsEmpty())
        sArgName.Format("Arg{}", argIdx);

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
      if (bIsDynamicPinProperty && scriptDataType == ezVisualScriptDataType::Array)
      {
        pArgRtti = ezGetStaticRTTI<ezVariant>();
        pinScriptDataType = ezVisualScriptDataType::Variant;
      }

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

          nodeDesc.AddInputDataPin(sArgName, pArgRtti, pinScriptDataType, false, sDynamicPinProperty);

          if (titleArgIdx == ezInvalidIndex &&
              (pinScriptDataType == ezVisualScriptDataType::String || pinScriptDataType == ezVisualScriptDataType::HashedString))
          {
            titleArgIdx = argIdx;
          }
        }
        else if (argType == ezScriptableFunctionAttribute::Out || argType == ezScriptableFunctionAttribute::Inout)
        {
          ezLog::Error("Script function out parameter are not yet supported");
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

  m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
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
    FillDesc(typeDesc, pRtti, szCoroutinesCategory, &coroutineColor);

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
      sArgName.Format("Arg{}", argIdx);

    auto pArgRtti = pStartFunc->GetArgumentType(argIdx);
    auto argType = pScriptableFuncAttribute->GetArgumentType(argIdx);
    if (argType != ezScriptableFunctionAttribute::In)
    {
      ezLog::Error("Script function out parameter are not yet supported");
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

  m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
}

void ezVisualScriptNodeRegistry::CreateMessageNodeTypes(const ezRTTI* pRtti)
{
  if (pRtti == ezGetStaticRTTI<ezMessage>() ||
      pRtti == ezGetStaticRTTI<ezEventMessage>() ||
      pRtti->GetTypeFlags().IsSet(ezTypeFlags::Abstract))
    return;

  // Message Handler
  {
    ezReflectedTypeDescriptor typeDesc;
    {
      FillDesc(typeDesc, pRtti, szEventHandlerCategory);

      ezStringBuilder temp;
      temp.Set(s_szTypeNamePrefix, "On", GetTypeName(pRtti));
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

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Message Sender
  {
    ezReflectedTypeDescriptor typeDesc;
    {
      FillDesc(typeDesc, pRtti, "Messages");

      ezStringBuilder temp;
      temp.Set(s_szTypeNamePrefix, "Send", GetTypeName(pRtti));
      typeDesc.m_sTypeName = temp;

      temp.Set("Send{?SendMode}", GetTypeName(pRtti), " {Delay}");
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

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }
}

void ezVisualScriptNodeRegistry::CreateEnumNodeTypes(const ezRTTI* pRtti)
{
  if (m_EnumTypes.Insert(pRtti))
    return;

  ezStringView sTypeName = GetTypeName(pRtti);
  ezColorGammaUB enumColor = PinDesc::GetColorForScriptDataType(ezVisualScriptDataType::EnumValue);

  // Value
  {
    ezStringBuilder sFullTypeName;
    sFullTypeName.Set(sTypeName, "Value");

    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, sFullTypeName, szEnumsCategory, enumColor);
    AddInputProperty(typeDesc, "Value", pRtti, ezVisualScriptDataType::EnumValue);

    ezStringBuilder sTitle;
    sTitle.Set(GetTypeName(pRtti), "::{Value}");

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, sTitle);
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Constant;
    nodeDesc.AddOutputDataPin("Value", pRtti, ezVisualScriptDataType::EnumValue);

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Switch
  {
    ezStringBuilder sFullTypeName;
    sFullTypeName.Set(sTypeName, "Switch");

    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, sFullTypeName, szEnumsCategory, enumColor);

    ezStringBuilder sTitle;
    sTitle.Set(GetTypeName(pRtti), "::Switch");

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

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }
}

void ezVisualScriptNodeRegistry::FillDesc(ezReflectedTypeDescriptor& desc, const ezRTTI* pRtti, ezStringView sCategoryOverride /*= ezStringView()*/, const ezColorGammaUB* pColorOverride /*= nullptr */)
{
  ezStringBuilder sTypeName = GetTypeName(pRtti);
  const ezRTTI* pBaseClass = FindTopMostBaseClass(pRtti);

  ezStringBuilder sCategory;
  if (sCategoryOverride.IsEmpty())
  {
    if (pBaseClass != pRtti)
    {
      sCategory.Set(StripTypeName(pBaseClass->GetTypeName()), "/", sTypeName);
    }
    else
    {
      sCategory = sTypeName;
    }
  }
  else
  {
    sCategory = sCategoryOverride;
  }

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

  FillDesc(desc, sTypeName, sCategory, color);
}

void ezVisualScriptNodeRegistry::FillDesc(ezReflectedTypeDescriptor& desc, ezStringView sTypeName, ezStringView sCategory, const ezColorGammaUB& color)
{
  ezStringBuilder sTypeNameFull;
  sTypeNameFull.Set(s_szTypeNamePrefix, sTypeName);

  desc.m_sTypeName = sTypeNameFull;
  desc.m_sPluginName = szPluginName;
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags = ezTypeFlags::Phantom | ezTypeFlags::Class;

  // Category
  {
    ezStringBuilder tmp;
    auto pAttr = EZ_DEFAULT_NEW(ezCategoryAttribute, sCategory.GetData(tmp));
    desc.m_Attributes.PushBack(pAttr);
  }

  // Color
  {
    auto pAttr = EZ_DEFAULT_NEW(ezColorAttribute, color);
    desc.m_Attributes.PushBack(pAttr);
  }
}
