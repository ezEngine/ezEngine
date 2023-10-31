#pragma once

#include <Foundation/Reflection/ReflectionUtils.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

using SerializeFunction = ezResult (*)(const ezVisualScriptNodeDescription& nodeDesc, ezStreamWriter& inout_stream, ezUInt32& out_Size, ezUInt32& out_alignment);
using DeserializeFunction = ezResult (*)(ezVisualScriptGraphDescription::Node& node, ezStreamReader& inout_stream, ezUInt8*& inout_pAdditionalData);
using ToStringFunction = void (*)(const ezVisualScriptNodeDescription& nodeDesc, ezStringBuilder& out_sResult);

namespace
{
  template <typename T, typename U>
  static ezUInt32 GetDynamicSize(ezUInt32 uiCount)
  {
    ezUInt32 uiSize = sizeof(T);
    if (uiCount > 1)
    {
      uiSize += sizeof(U) * (uiCount - 1);
    }
    return uiSize;
  }

  struct NodeUserData_Type
  {
    const ezRTTI* m_pType = nullptr;

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
    ezUInt32 m_uiPadding;
#endif

    static ezResult Serialize(const ezVisualScriptNodeDescription& nodeDesc, ezStreamWriter& inout_stream, ezUInt32& out_uiSize, ezUInt32& out_uiAlignment)
    {
      inout_stream << nodeDesc.m_sTargetTypeName;

      out_uiSize = sizeof(NodeUserData_Type);
      out_uiAlignment = EZ_ALIGNMENT_OF(NodeUserData_Type);
      return EZ_SUCCESS;
    }

    static ezResult ReadType(ezStreamReader& inout_stream, const ezRTTI*& out_pType)
    {
      ezStringBuilder sTypeName;
      inout_stream >> sTypeName;

      out_pType = ezRTTI::FindTypeByName(sTypeName);
      if (out_pType == nullptr)
      {
        ezLog::Error("Unknown type '{}'", sTypeName);
        return EZ_FAILURE;
      }

      return EZ_SUCCESS;
    }

    static ezResult Deserialize(ezVisualScriptGraphDescription::Node& ref_node, ezStreamReader& inout_stream, ezUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_Type>(inout_pAdditionalData);
      EZ_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));
      return EZ_SUCCESS;
    }

    static void ToString(const ezVisualScriptNodeDescription& nodeDesc, ezStringBuilder& out_sResult)
    {
      if (nodeDesc.m_sTargetTypeName.IsEmpty() == false)
      {
        out_sResult.Append(nodeDesc.m_sTargetTypeName);
      }
    }
  };

  static_assert(sizeof(NodeUserData_Type) == 8);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_TypeAndProperty : public NodeUserData_Type
  {
    const ezAbstractProperty* m_pProperty = nullptr;

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
    ezUInt32 m_uiPadding;
#endif

    static ezResult Serialize(const ezVisualScriptNodeDescription& nodeDesc, ezStreamWriter& inout_stream, ezUInt32& out_uiSize, ezUInt32& out_uiAlignment)
    {
      EZ_SUCCEED_OR_RETURN(NodeUserData_Type::Serialize(nodeDesc, inout_stream, out_uiSize, out_uiAlignment));

      const ezVariantArray& propertiesVar = nodeDesc.m_Value.Get<ezVariantArray>();
      EZ_ASSERT_DEBUG(propertiesVar.GetCount() == 1, "Invalid number of properties");

      inout_stream << propertiesVar[0].Get<ezHashedString>();

      out_uiSize = sizeof(NodeUserData_TypeAndProperty);
      out_uiAlignment = EZ_ALIGNMENT_OF(NodeUserData_TypeAndProperty);
      return EZ_SUCCESS;
    }

    template <typename T>
    static ezResult ReadProperty(ezStreamReader& inout_stream, const ezRTTI* pType, ezArrayPtr<T> properties, const ezAbstractProperty*& out_pProp)
    {
      ezStringBuilder sPropName;
      inout_stream >> sPropName;

      out_pProp = nullptr;
      for (auto& pProp : properties)
      {
        if (sPropName == pProp->GetPropertyName())
        {
          out_pProp = pProp;
          break;
        }
      }

      if (out_pProp == nullptr)
      {
        constexpr bool isFunction = std::is_same_v<T, const ezAbstractFunctionProperty* const>;
        ezLog::Error("{} '{}' not found on type '{}'", isFunction ? "Function" : "Property", sPropName, pType->GetTypeName());
        return EZ_FAILURE;
      }

      return EZ_SUCCESS;
    }

    template <bool PropIsFunction>
    static ezResult Deserialize(ezVisualScriptGraphDescription::Node& ref_node, ezStreamReader& inout_stream, ezUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_TypeAndProperty>(inout_pAdditionalData);
      EZ_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));

      if constexpr (PropIsFunction)
      {
        EZ_SUCCEED_OR_RETURN(ReadProperty(inout_stream, userData.m_pType, userData.m_pType->GetFunctions(), userData.m_pProperty));
      }
      else
      {
        EZ_SUCCEED_OR_RETURN(ReadProperty(inout_stream, userData.m_pType, userData.m_pType->GetProperties(), userData.m_pProperty));
      }

      return EZ_SUCCESS;
    }

    static void ToString(const ezVisualScriptNodeDescription& nodeDesc, ezStringBuilder& out_sResult)
    {
      NodeUserData_Type::ToString(nodeDesc, out_sResult);

      if (nodeDesc.m_Value.IsA<ezVariantArray>())
      {
        const ezVariantArray& propertiesVar = nodeDesc.m_Value.Get<ezVariantArray>();
        if (propertiesVar.IsEmpty() == false)
        {
          out_sResult.Append(".", propertiesVar[0].Get<ezHashedString>());
        }
      }
    }
  };

  static_assert(sizeof(NodeUserData_TypeAndProperty) == 16);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_TypeAndProperties : public NodeUserData_Type
  {
    ezUInt32 m_uiNumProperties = 0;

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
    ezUInt32 m_uiPadding0;
#endif

    // This struct is allocated with enough space behind it to hold an array with m_uiNumProperties size.
    const ezAbstractProperty* m_Properties[1];

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
    ezUInt32 m_uiPadding1;
#endif

    static ezResult Serialize(const ezVisualScriptNodeDescription& nodeDesc, ezStreamWriter& inout_stream, ezUInt32& out_uiSize, ezUInt32& out_uiAlignment)
    {
      EZ_SUCCEED_OR_RETURN(NodeUserData_Type::Serialize(nodeDesc, inout_stream, out_uiSize, out_uiAlignment));

      const ezVariantArray& propertiesVar = nodeDesc.m_Value.Get<ezVariantArray>();

      ezUInt32 uiCount = propertiesVar.GetCount();
      inout_stream << uiCount;

      for (auto& var : propertiesVar)
      {
        ezHashedString sPropName = var.Get<ezHashedString>();
        inout_stream << sPropName;
      }

      static_assert(sizeof(void*) <= sizeof(ezUInt64));
      out_uiSize = GetDynamicSize<NodeUserData_TypeAndProperties, ezUInt64>(uiCount);
      out_uiAlignment = EZ_ALIGNMENT_OF(NodeUserData_TypeAndProperties);
      return EZ_SUCCESS;
    }

    static ezResult Deserialize(ezVisualScriptGraphDescription::Node& ref_node, ezStreamReader& inout_stream, ezUInt8*& inout_pAdditionalData)
    {
      const ezRTTI* pType = nullptr;
      EZ_SUCCEED_OR_RETURN(ReadType(inout_stream, pType));

      ezUInt32 uiCount = 0;
      inout_stream >> uiCount;

      const ezUInt32 uiByteSize = GetDynamicSize<NodeUserData_TypeAndProperties, ezUInt64>(uiCount);
      auto& userData = ref_node.InitUserData<NodeUserData_TypeAndProperties>(inout_pAdditionalData, uiByteSize);
      userData.m_pType = pType;
      userData.m_uiNumProperties = uiCount;

      ezHybridArray<const ezAbstractProperty*, 32> properties;
      userData.m_pType->GetAllProperties(properties);

      for (ezUInt32 i = 0; i < uiCount; ++i)
      {
        const ezAbstractProperty* pProperty = nullptr;
        EZ_SUCCEED_OR_RETURN(NodeUserData_TypeAndProperty::ReadProperty(inout_stream, userData.m_pType, properties.GetArrayPtr(), pProperty));
        userData.m_Properties[i] = pProperty;
      }

      return EZ_SUCCESS;
    }

    static void ToString(const ezVisualScriptNodeDescription& nodeDesc, ezStringBuilder& out_sResult)
    {
      NodeUserData_TypeAndProperty::ToString(nodeDesc, out_sResult);
    }
  };

  static_assert(sizeof(NodeUserData_TypeAndProperties) == 24);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_Switch
  {
    ezUInt32 m_uiNumCases = 0;

    // This struct is allocated with enough space behind it to hold an array with m_uiNumCases size.
    ezInt64 m_Cases[1];

    static ezResult Serialize(const ezVisualScriptNodeDescription& nodeDesc, ezStreamWriter& inout_stream, ezUInt32& out_uiSize, ezUInt32& out_uiAlignment)
    {
      const ezVariantArray& casesVar = nodeDesc.m_Value.Get<ezVariantArray>();

      ezUInt32 uiCount = casesVar.GetCount();
      inout_stream << uiCount;

      for (auto& var : casesVar)
      {
        ezInt64 iCaseValue = var.ConvertTo<ezInt64>();
        inout_stream << iCaseValue;
      }

      out_uiSize = GetDynamicSize<NodeUserData_Switch, ezInt64>(uiCount);
      out_uiAlignment = EZ_ALIGNMENT_OF(NodeUserData_Switch);
      return EZ_SUCCESS;
    }

    static ezResult Deserialize(ezVisualScriptGraphDescription::Node& ref_node, ezStreamReader& inout_stream, ezUInt8*& inout_pAdditionalData)
    {
      ezUInt32 uiCount = 0;
      inout_stream >> uiCount;

      const ezUInt32 uiByteSize = GetDynamicSize<NodeUserData_Switch, ezInt64>(uiCount);
      auto& userData = ref_node.InitUserData<NodeUserData_Switch>(inout_pAdditionalData, uiByteSize);
      userData.m_uiNumCases = uiCount;

      for (ezUInt32 i = 0; i < uiCount; ++i)
      {
        inout_stream >> userData.m_Cases[i];
      }

      return EZ_SUCCESS;
    }

    static void ToString(const ezVisualScriptNodeDescription& nodeDesc, ezStringBuilder& out_sResult)
    {
      // Nothing to add here
    }
  };

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_Comparison
  {
    ezEnum<ezComparisonOperator> m_ComparisonOperator;

    static ezResult Serialize(const ezVisualScriptNodeDescription& nodeDesc, ezStreamWriter& inout_stream, ezUInt32& out_uiSize, ezUInt32& out_uiAlignment)
    {
      ezEnum<ezComparisonOperator> compOp = static_cast<ezComparisonOperator::Enum>(nodeDesc.m_Value.Get<ezInt64>());
      inout_stream << compOp;

      out_uiSize = sizeof(NodeUserData_Comparison);
      out_uiAlignment = EZ_ALIGNMENT_OF(NodeUserData_Comparison);
      return EZ_SUCCESS;
    }

    static ezResult Deserialize(ezVisualScriptGraphDescription::Node& ref_node, ezStreamReader& inout_stream, ezUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_Comparison>(inout_pAdditionalData);
      inout_stream >> userData.m_ComparisonOperator;

      return EZ_SUCCESS;
    }

    static void ToString(const ezVisualScriptNodeDescription& nodeDesc, ezStringBuilder& out_sResult)
    {
      ezStringBuilder sCompOp;
      ezReflectionUtils::EnumerationToString(ezGetStaticRTTI<ezComparisonOperator>(), nodeDesc.m_Value.Get<ezInt64>(), sCompOp, ezReflectionUtils::EnumConversionMode::ValueNameOnly);

      out_sResult.Append(" ", sCompOp);
    }
  };

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_StartCoroutine : public NodeUserData_Type
  {
    ezEnum<ezScriptCoroutineCreationMode> m_CreationMode;

    static ezResult Serialize(const ezVisualScriptNodeDescription& nodeDesc, ezStreamWriter& inout_stream, ezUInt32& out_uiSize, ezUInt32& out_uiAlignment)
    {
      EZ_SUCCEED_OR_RETURN(NodeUserData_Type::Serialize(nodeDesc, inout_stream, out_uiSize, out_uiAlignment));

      ezEnum<ezScriptCoroutineCreationMode> creationMode = static_cast<ezScriptCoroutineCreationMode::Enum>(nodeDesc.m_Value.Get<ezInt64>());
      inout_stream << creationMode;

      out_uiSize = sizeof(NodeUserData_StartCoroutine);
      out_uiAlignment = EZ_ALIGNMENT_OF(NodeUserData_StartCoroutine);
      return EZ_SUCCESS;
    }

    static ezResult Deserialize(ezVisualScriptGraphDescription::Node& ref_node, ezStreamReader& inout_stream, ezUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_StartCoroutine>(inout_pAdditionalData);
      EZ_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));

      inout_stream >> userData.m_CreationMode;

      return EZ_SUCCESS;
    }

    static void ToString(const ezVisualScriptNodeDescription& nodeDesc, ezStringBuilder& out_sResult)
    {
      NodeUserData_Type::ToString(nodeDesc, out_sResult);

      ezStringBuilder sCreationMode;
      ezReflectionUtils::EnumerationToString(ezGetStaticRTTI<ezScriptCoroutineCreationMode>(), nodeDesc.m_Value.Get<ezInt64>(), sCreationMode, ezReflectionUtils::EnumConversionMode::ValueNameOnly);

      out_sResult.Append(" ", sCreationMode);
    }
  };

  static_assert(sizeof(NodeUserData_StartCoroutine) == 16);

  //////////////////////////////////////////////////////////////////////////

  struct UserDataContext
  {
    SerializeFunction m_SerializeFunc = nullptr;
    DeserializeFunction m_DeserializeFunc = nullptr;
    ToStringFunction m_ToStringFunc = nullptr;
  };

  inline UserDataContext s_TypeToUserDataContexts[] = {
    {}, // Invalid,
    {}, // EntryCall,
    {}, // EntryCall_Coroutine,
    {&NodeUserData_TypeAndProperties::Serialize,
      &NodeUserData_TypeAndProperties::Deserialize,
      &NodeUserData_TypeAndProperties::ToString}, // MessageHandler,
    {&NodeUserData_TypeAndProperties::Serialize,
      &NodeUserData_TypeAndProperties::Deserialize,
      &NodeUserData_TypeAndProperties::ToString}, // MessageHandler_Coroutine,
    {&NodeUserData_TypeAndProperty::Serialize,
      &NodeUserData_TypeAndProperty::Deserialize<true>,
      &NodeUserData_TypeAndProperty::ToString}, // ReflectedFunction,
    {&NodeUserData_TypeAndProperty::Serialize,
      &NodeUserData_TypeAndProperty::Deserialize<false>,
      &NodeUserData_TypeAndProperty::ToString}, // GetReflectedProperty,
    {&NodeUserData_TypeAndProperty::Serialize,
      &NodeUserData_TypeAndProperty::Deserialize<false>,
      &NodeUserData_TypeAndProperty::ToString}, // SetReflectedProperty,
    {&NodeUserData_TypeAndProperty::Serialize,
      &NodeUserData_TypeAndProperty::Deserialize<true>,
      &NodeUserData_TypeAndProperty::ToString}, // InplaceCoroutine,
    {},                                         // GetScriptOwner,
    {&NodeUserData_TypeAndProperties::Serialize,
      &NodeUserData_TypeAndProperties::Deserialize,
      &NodeUserData_TypeAndProperties::ToString}, // SendMessage,

    {}, // FirstBuiltin,

    {}, // Builtin_Constant,
    {}, // Builtin_GetVariable,
    {}, // Builtin_SetVariable,
    {}, // Builtin_IncVariable,
    {}, // Builtin_DecVariable,

    {}, // Builtin_Branch,
    {&NodeUserData_Switch::Serialize,
      &NodeUserData_Switch::Deserialize,
      &NodeUserData_Switch::ToString}, // Builtin_Switch,
    {},                                // Builtin_WhileLoop,
    {},                                // Builtin_ForLoop,
    {},                                // Builtin_ForEachLoop,
    {},                                // Builtin_ReverseForEachLoop,
    {},                                // Builtin_Break,
    {},                                // Builtin_Jump,

    {}, // Builtin_And,
    {}, // Builtin_Or,
    {}, // Builtin_Not,
    {&NodeUserData_Comparison::Serialize,
      &NodeUserData_Comparison::Deserialize,
      &NodeUserData_Comparison::ToString}, // Builtin_Compare,
    {},                                    // Builtin_CompareExec,
    {},                                    // Builtin_IsValid,
    {},                                    // Builtin_Select,

    {}, // Builtin_Add,
    {}, // Builtin_Subtract,
    {}, // Builtin_Multiply,
    {}, // Builtin_Divide,
    {}, // Builtin_Expression,

    {}, // Builtin_ToBool,
    {}, // Builtin_ToByte,
    {}, // Builtin_ToInt,
    {}, // Builtin_ToInt64,
    {}, // Builtin_ToFloat,
    {}, // Builtin_ToDouble,
    {}, // Builtin_ToString,
    {}, // Builtin_String_Format,
    {}, // Builtin_ToHashedString,
    {}, // Builtin_ToVariant,
    {}, // Builtin_Variant_ConvertTo,

    {}, // Builtin_MakeArray
    {}, // Builtin_Array_GetElement,
    {}, // Builtin_Array_SetElement,
    {}, // Builtin_Array_GetCount,
    {}, // Builtin_Array_IsEmpty,
    {}, // Builtin_Array_Clear,
    {}, // Builtin_Array_Contains,
    {}, // Builtin_Array_IndexOf,
    {}, // Builtin_Array_Insert,
    {}, // Builtin_Array_PushBack,
    {}, // Builtin_Array_Remove,
    {}, // Builtin_Array_RemoveAt,

    {&NodeUserData_Type::Serialize,
      &NodeUserData_Type::Deserialize,
      &NodeUserData_Type::ToString}, // Builtin_TryGetComponentOfBaseType

    {&NodeUserData_StartCoroutine::Serialize,
      &NodeUserData_StartCoroutine::Deserialize,
      &NodeUserData_StartCoroutine::ToString}, // Builtin_StartCoroutine,
    {},                                        // Builtin_StopCoroutine,
    {},                                        // Builtin_StopAllCoroutines,
    {},                                        // Builtin_WaitForAll,
    {},                                        // Builtin_WaitForAny,
    {},                                        // Builtin_Yield,

    {}, // LastBuiltin,
  };

  static_assert(EZ_ARRAY_SIZE(s_TypeToUserDataContexts) == ezVisualScriptNodeDescription::Type::Count);
} // namespace

const UserDataContext& GetUserDataContext(ezVisualScriptNodeDescription::Type::Enum nodeType)
{
  EZ_ASSERT_DEBUG(nodeType >= 0 && nodeType < EZ_ARRAY_SIZE(s_TypeToUserDataContexts), "Out of bounds access");
  return s_TypeToUserDataContexts[nodeType];
}
