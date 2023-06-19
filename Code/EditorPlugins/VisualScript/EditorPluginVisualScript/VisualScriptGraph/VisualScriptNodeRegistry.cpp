#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>
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

  const ezRTTI* FindTopMostBaseClass(const ezRTTI* pRtti)
  {
    const ezRTTI* pReflectedClass = ezGetStaticRTTI<ezReflectedClass>();
    while (pRtti->GetParentType() != nullptr && pRtti->GetParentType() != pReflectedClass)
    {
      pRtti = pRtti->GetParentType();
    }
    return pRtti;
  }

  ezResult GetScriptDataType(const ezRTTI* pRtti, ezVisualScriptDataType::Enum& out_scriptDataType, ezStringView sFunctionName, ezStringView sArgName)
  {
    if (pRtti->GetTypeFlags().IsSet(ezTypeFlags::IsEnum))
    {
      out_scriptDataType = ezVisualScriptDataType::Int64;
      return EZ_SUCCESS;
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

  void AddInputProperty(ezReflectedTypeDescriptor& ref_typeDesc, ezStringView sName, const ezRTTI* pRtti, ezVisualScriptDataType::Enum scriptDataType)
  {
    auto& propDesc = ref_typeDesc.m_Properties.ExpandAndGetRef();
    propDesc.m_sName = sName;
    propDesc.m_Flags = ezPropertyFlags::StandardType;

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

  template <typename T>
  void AddInputDataPin(ezReflectedTypeDescriptor& ref_typeDesc, ezVisualScriptNodeRegistry::NodeDesc& ref_nodeDesc, ezStringView sName)
  {
    const ezRTTI* pDataType = ezGetStaticRTTI<T>();

    ezVisualScriptDataType::Enum scriptDataType;
    EZ_VERIFY(GetScriptDataType(pDataType, scriptDataType, "", sName).Succeeded(), "Invalid script data type");

    AddInputProperty(ref_typeDesc, sName, pDataType, scriptDataType);

    ref_nodeDesc.AddInputDataPin(sName, pDataType, scriptDataType, false);
  };

  void AddInputDataPin_Any(ezReflectedTypeDescriptor& ref_typeDesc, ezVisualScriptNodeRegistry::NodeDesc& ref_nodeDesc, ezStringView sName, bool bRequired, bool bAddVariantProperty = false)
  {
    if (bAddVariantProperty)
    {
      AddInputProperty(ref_typeDesc, sName, ezGetStaticRTTI<ezVariant>(), ezVisualScriptDataType::Variant);
    }

    ref_nodeDesc.AddInputDataPin(sName, nullptr, ezVisualScriptDataType::Any, bRequired);
    ref_nodeDesc.m_bNeedsDataTypeDeduction = true;
  }

  template <typename T>
  void AddOutputDataPin(ezVisualScriptNodeRegistry::NodeDesc& ref_nodeDesc, ezStringView sName)
  {
    const ezRTTI* pDataType = ezGetStaticRTTI<T>();

    ezVisualScriptDataType::Enum scriptDataType;
    EZ_VERIFY(GetScriptDataType(pDataType, scriptDataType, "", sName).Succeeded(), "Invalid script data type");

    ref_nodeDesc.AddOutputDataPin(sName, pDataType, scriptDataType);
  };

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
}

void ezVisualScriptNodeRegistry::NodeDesc::AddOutputExecutionPin(ezStringView sName, const ezHashedString& sDynamicPinProperty /*= ezHashedString()*/, bool bSplitExecution /*= false*/)
{
  AddExecutionPin(*this, sName, sDynamicPinProperty, bSplitExecution, m_OutputPins);
}

void AddDataPin(ezVisualScriptNodeRegistry::NodeDesc& inout_nodeDesc, ezStringView sName, const ezRTTI* pDataType, ezVisualScriptDataType::Enum scriptDataType, bool bRequired, ezHashedString sDynamicPinProperty, ezSmallArray<ezVisualScriptNodeRegistry::PinDesc, 4>& inout_pins)
{
  auto& pin = inout_pins.ExpandAndGetRef();
  pin.m_sName.Assign(sName);
  pin.m_sDynamicPinProperty = sDynamicPinProperty;
  pin.m_pDataType = pDataType;
  pin.m_ScriptDataType = scriptDataType;
  pin.m_bRequired = bRequired;

  inout_nodeDesc.m_bHasDynamicPins |= (sDynamicPinProperty.IsEmpty() == false);
}

void ezVisualScriptNodeRegistry::NodeDesc::AddInputDataPin(ezStringView sName, const ezRTTI* pDataType, ezVisualScriptDataType::Enum scriptDataType, bool bRequired, const ezHashedString& sDynamicPinProperty /*= ezHashedString()*/)
{
  AddDataPin(*this, sName, pDataType, scriptDataType, bRequired, sDynamicPinProperty, m_InputPins);
}

void ezVisualScriptNodeRegistry::NodeDesc::AddOutputDataPin(ezStringView sName, const ezRTTI* pDataType, ezVisualScriptDataType::Enum scriptDataType, const ezHashedString& sDynamicPinProperty /*= ezHashedString()*/)
{
  AddDataPin(*this, sName, pDataType, scriptDataType, false, sDynamicPinProperty, m_OutputPins);
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

  ezRTTI::ForEachType([this](const ezRTTI* pRtti) { UpdateNodeType(pRtti); });
}

void ezVisualScriptNodeRegistry::UpdateNodeType(const ezRTTI* pRtti)
{
  if (pRtti->GetAttributeByType<ezHiddenAttribute>() != nullptr)
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
  else
  {
    // expose reflected functions and properties to visual scripts
    {
      bool bHasBaseClassFunctions = false;

      for (const ezAbstractFunctionProperty* pFuncProp : pRtti->GetFunctions())
      {
        bool bIsBaseClassFunction = pFuncProp->GetAttributeByType<ezScriptBaseClassFunctionAttribute>() != nullptr;
        if (bIsBaseClassFunction)
        {
          bHasBaseClassFunctions = true;
        }

        CreateFunctionCallNodeType(pRtti, pFuncProp, bIsBaseClassFunction);
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

void ezVisualScriptNodeRegistry::CreateBuiltinTypes()
{
  const ezColorGammaUB logicColor = PinDesc::GetColorForScriptDataType(ezVisualScriptDataType::Invalid);
  const ezColorGammaUB mathColor = PinDesc::GetColorForScriptDataType(ezVisualScriptDataType::Int);
  const ezColorGammaUB stringColor = PinDesc::GetColorForScriptDataType(ezVisualScriptDataType::String);
  const ezColorGammaUB gameObjectColor = PinDesc::GetColorForScriptDataType(ezVisualScriptDataType::GameObject);
  const ezColorGammaUB variantColor = PinDesc::GetColorForScriptDataType(ezVisualScriptDataType::Variant);
  const ezColorGammaUB coroutineColor = PinDesc::GetColorForScriptDataType(ezVisualScriptDataType::Coroutine);

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

  // Builtin_And
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_And", "Logic", logicColor);

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "{A} AND {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_And;
    nodeDesc.m_bImplicitExecution = true;

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
    nodeDesc.m_bImplicitExecution = true;

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
    nodeDesc.m_bImplicitExecution = true;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Compare
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Compare", "Logic", logicColor);

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = ezPropertyCategory::Member;
      propDesc.m_sName = "Operator";
      propDesc.m_sType = ezGetStaticRTTI<ezComparisonOperator>()->GetTypeName();
      propDesc.m_Flags = ezPropertyFlags::IsEnum;
    }

    auto pAttr = EZ_DEFAULT_NEW(ezTitleAttribute, "{A} {Operator} {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_Compare;
    nodeDesc.m_bImplicitExecution = true;

    AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
    AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_IsValid
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_IsValid", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = ezVisualScriptNodeDescription::Type::Builtin_IsValid;
    nodeDesc.m_bImplicitExecution = true;

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
      nodeDesc.m_bImplicitExecution = true;

      AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
      AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
      nodeDesc.AddOutputDataPin("", nullptr, ezVisualScriptDataType::Any);

      m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

  // Builtin_ToBool, Builtin_ToByte, Builtin_ToInt, Builtin_ToInt64, Builtin_ToFloat, Builtin_ToDouble, Builtin_ToString, Builtin_ToVariant,
  {
    struct ConversionNodeDesc // NOLINT(*Padding): Don't care about excessive padding here.
    {
      ezVisualScriptDataType::Enum m_DataType;
      const char* m_szCategory;
      ezColorGammaUB m_Color;
    };

    ConversionNodeDesc conversionNodeDescs[] = {
      {ezVisualScriptDataType::Bool, "Logic", logicColor},
      {ezVisualScriptDataType::Byte, "Math", mathColor},
      {ezVisualScriptDataType::Int, "Math", mathColor},
      {ezVisualScriptDataType::Int64, "Math", mathColor},
      {ezVisualScriptDataType::Float, "Math", mathColor},
      {ezVisualScriptDataType::Double, "Math", mathColor},
      {ezVisualScriptDataType::String, "String", stringColor},
      {ezVisualScriptDataType::Variant, "Variant", variantColor},
    };

    for (auto& conversionNodeDesc : conversionNodeDescs)
    {
      auto nodeType = ezVisualScriptNodeDescription::Type::GetConversionType(conversionNodeDesc.m_DataType);

      ezReflectedTypeDescriptor typeDesc;
      FillDesc(typeDesc, ezVisualScriptNodeDescription::Type::GetName(nodeType), conversionNodeDesc.m_szCategory, conversionNodeDesc.m_Color);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = nodeType;
      nodeDesc.m_bImplicitExecution = true;

      AddInputDataPin_Any(typeDesc, nodeDesc, "", true);
      nodeDesc.AddOutputDataPin("", ezVisualScriptDataType::GetRtti(conversionNodeDesc.m_DataType), conversionNodeDesc.m_DataType);

      m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

  // Builtin_Variant_ConvertTo
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Variant_ConvertTo", "Variant", variantColor);

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
    nodeDesc.m_bNeedsDataTypeDeduction = true;

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
    nodeDesc.m_bImplicitExecution = true;

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
    nodeDesc.m_bImplicitExecution = true;

    nodeDesc.AddInputDataPin("GameObject", ezGetStaticRTTI<ezGameObject>(), ezVisualScriptDataType::GameObject, true);
    AddOutputDataPin<ezComponent>(nodeDesc, "Component");

    m_TypeToNodeDescs.Insert(ezPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_StartCoroutine
  {
    ezReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_StartCoroutine", szCoroutinesCategory, coroutineColor);

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = ezPropertyCategory::Member;
      propDesc.m_sName = "CoroutineMode";
      propDesc.m_sType = ezGetStaticRTTI<ezScriptCoroutineCreationMode>()->GetTypeName();
      propDesc.m_Flags = ezPropertyFlags::IsEnum;
    }

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
  nodeDesc.m_bImplicitExecution = true;

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

void ezVisualScriptNodeRegistry::CreateFunctionCallNodeType(const ezRTTI* pRtti, const ezAbstractFunctionProperty* pFunction, bool bIsEntryFunction)
{
  const ezScriptableFunctionAttribute* pScriptableFunctionAttribute = pFunction->GetAttributeByType<ezScriptableFunctionAttribute>();
  if (pScriptableFunctionAttribute == nullptr)
    return;

  ezHashSet<ezStringView> dynamicPins;
  for (auto pAttribute : pFunction->GetAttributes())
  {
    if (auto pDynamicPinAttribute = ezDynamicCast<ezDynamicPinAttribute*>(pAttribute))
    {
      dynamicPins.Insert(pDynamicPinAttribute->GetProperty());
    }
  }

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
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = ezPropertyCategory::Member;
      propDesc.m_sName = "CoroutineMode";
      propDesc.m_sType = ezGetStaticRTTI<ezScriptCoroutineCreationMode>()->GetTypeName();
      propDesc.m_Flags = ezPropertyFlags::IsEnum;
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
  nodeDesc.m_pTargetProperty = pFunction;
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
    if (pFunction->GetFlags().IsSet(ezPropertyFlags::Const))
    {
      nodeDesc.m_bImplicitExecution = true;
    }
    else
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
        if (pRtti->IsDerivedFrom<ezComponent>())
        {
          nodeDesc.AddInputDataPin("Component", pRtti, ezVisualScriptDataType::Component, true);
        }
        else if (pRtti->IsDerivedFrom<ezGameObject>())
        {
          nodeDesc.AddInputDataPin("GameObject", pRtti, ezVisualScriptDataType::GameObject, true);
        }
        else
        {
          nodeDesc.AddInputDataPin("Object", pRtti, ezVisualScriptDataType::TypedPointer, true);
        }
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
            AddInputProperty(typeDesc, sArgName, pArgRtti, scriptDataType);
          }

          nodeDesc.AddInputDataPin(sArgName, pArgRtti, pinScriptDataType, false, sDynamicPinProperty);

          if (titleArgIdx == ezInvalidIndex && pinScriptDataType == ezVisualScriptDataType::String)
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
  nodeDesc.m_pTargetProperty = pStartFunc;
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
