#pragma once

using SerializeFunction = ezResult (*)(const ezVisualScriptNodeDescription& nodeDesc, ezStreamWriter& inout_stream, ezUInt32& out_Size, ezUInt32& out_alignment);
using DeserializeFunction = ezResult (*)(ezVisualScriptGraphDescription::Node& node, ezStreamReader& inout_stream, ezUInt8*& inout_pAdditionalData);
using ToStringFunction = void (*)(const ezVisualScriptNodeDescription& nodeDesc, ezStringBuilder& out_sResult);

namespace
{
  struct NodeUserData_Type
  {
    const ezRTTI* m_pType = nullptr;

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
    ezUInt32 m_uiPadding;
#endif

    static ezResult Serialize(const ezVisualScriptNodeDescription& nodeDesc, ezStreamWriter& inout_stream, ezUInt32& out_Size, ezUInt32& out_alignment)
    {
      inout_stream << nodeDesc.m_UserData.m_pTargetType->GetTypeName();

      out_Size = sizeof(NodeUserData_Type);
      out_alignment = EZ_ALIGNMENT_OF(NodeUserData_Type);
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

    static ezResult Deserialize(ezVisualScriptGraphDescription::Node& node, ezStreamReader& inout_stream, ezUInt8*& inout_pAdditionalData)
    {
      NodeUserData_Type userData;
      EZ_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));
      node.SetUserData(userData, inout_pAdditionalData);
      return EZ_SUCCESS;
    }

    static void ToString(const ezVisualScriptNodeDescription& nodeDesc, ezStringBuilder& out_sResult)
    {
      if (nodeDesc.m_UserData.m_pTargetType != nullptr)
      {
        out_sResult.Append(nodeDesc.m_UserData.m_pTargetType->GetTypeName());
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

    static ezResult Serialize(const ezVisualScriptNodeDescription& nodeDesc, ezStreamWriter& inout_stream, ezUInt32& out_Size, ezUInt32& out_alignment)
    {
      EZ_SUCCEED_OR_RETURN(NodeUserData_Type::Serialize(nodeDesc, inout_stream, out_Size, out_alignment));

      inout_stream << nodeDesc.m_UserData.m_pTargetProperty->GetPropertyName();

      out_Size = sizeof(NodeUserData_TypeAndProperty);
      out_alignment = EZ_ALIGNMENT_OF(NodeUserData_TypeAndProperty);
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
        ezLog::Error("{} '{}' not found on type '{}'",
          std::is_same<T, ezAbstractFunctionProperty>::value ? "Function" : "Property",
          sPropName, pType->GetTypeName());
        return EZ_FAILURE;
      }

      return EZ_SUCCESS;
    }

    template <bool PropIsFunction>
    static ezResult Deserialize(ezVisualScriptGraphDescription::Node& node, ezStreamReader& inout_stream, ezUInt8*& inout_pAdditionalData)
    {
      NodeUserData_TypeAndProperty userData;
      EZ_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));

      if constexpr (PropIsFunction)
      {
        EZ_SUCCEED_OR_RETURN(ReadProperty(inout_stream, userData.m_pType, userData.m_pType->GetFunctions(), userData.m_pProperty));
      }
      else
      {
        EZ_SUCCEED_OR_RETURN(ReadProperty(inout_stream, userData.m_pType, userData.m_pType->GetProperties(), userData.m_pProperty));
      }

      node.SetUserData(userData, inout_pAdditionalData);
      return EZ_SUCCESS;
    }

    static void ToString(const ezVisualScriptNodeDescription& nodeDesc, ezStringBuilder& out_sResult)
    {
      NodeUserData_Type::ToString(nodeDesc, out_sResult);

      if (nodeDesc.m_UserData.m_pTargetProperty != nullptr)
      {
        out_sResult.Append(".", nodeDesc.m_UserData.m_pTargetProperty->GetPropertyName());
      }
    }
  };

  static_assert(sizeof(NodeUserData_TypeAndProperty) == 16);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_Comparison
  {
    ezEnum<ezComparisonOperator> m_ComparisonOperator;

    static ezResult Serialize(const ezVisualScriptNodeDescription& nodeDesc, ezStreamWriter& inout_stream, ezUInt32& out_Size, ezUInt32& out_alignment)
    {
      ezEnum<ezComparisonOperator> compOp = nodeDesc.m_UserData.m_ComparisonOperator;
      inout_stream << compOp;

      out_Size = sizeof(NodeUserData_Comparison);
      out_alignment = EZ_ALIGNMENT_OF(NodeUserData_Comparison);
      return EZ_SUCCESS;
    }

    static ezResult Deserialize(ezVisualScriptGraphDescription::Node& node, ezStreamReader& inout_stream, ezUInt8*& inout_pAdditionalData)
    {
      NodeUserData_Comparison userData;
      inout_stream >> userData.m_ComparisonOperator;
      node.SetUserData(userData, inout_pAdditionalData);

      return EZ_SUCCESS;
    }

    static void ToString(const ezVisualScriptNodeDescription& nodeDesc, ezStringBuilder& out_sResult)
    {
      ezStringBuilder sCompOp;
      ezReflectionUtils::EnumerationToString<ezComparisonOperator>(nodeDesc.m_UserData.m_ComparisonOperator, sCompOp, ezReflectionUtils::EnumConversionMode::ValueNameOnly);

      out_sResult.Append(sCompOp);
    }
  };

  //////////////////////////////////////////////////////////////////////////

  struct UserDataContext
  {
    SerializeFunction m_SerializeFunc = nullptr;
    DeserializeFunction m_DeserializeFunc = nullptr;
    ToStringFunction m_ToStringFunc = nullptr;
  };

  static UserDataContext s_TypeToUserDataContexts[] = {
    {}, // Invalid,
    {}, // EntryCall,
    {}, // MessageHandler,
    {&NodeUserData_TypeAndProperty::Serialize,
      &NodeUserData_TypeAndProperty::Deserialize<true>,
      &NodeUserData_TypeAndProperty::ToString}, // ReflectedFunction,
    {},                                         // GetOwner,

    {}, // FirstBuiltin,

    {}, // Builtin_Branch,
    {}, // Builtin_And,
    {}, // Builtin_Or,
    {}, // Builtin_Not,
    {&NodeUserData_Comparison::Serialize,
      &NodeUserData_Comparison::Deserialize,
      &NodeUserData_Comparison::ToString}, // Builtin_Compare,
    {},                                    // Builtin_IsValid,

    {}, // Builtin_Add,
    {}, // Builtin_Subtract,
    {}, // Builtin_Multiply,
    {}, // Builtin_Divide,

    {}, // Builtin_ToBool,
    {}, // Builtin_ToByte,
    {}, // Builtin_ToInt,
    {}, // Builtin_ToInt64,
    {}, // Builtin_ToFloat,
    {}, // Builtin_ToDouble,
    {}, // Builtin_ToString,
    {}, // Builtin_ToVariant,
    {}, // Builtin_Variant_ConvertTo,

    {}, // Builtin_MakeArray

    {&NodeUserData_Type::Serialize,
      &NodeUserData_Type::Deserialize,
      &NodeUserData_Type::ToString}, // Builtin_TryGetComponentOfBaseType

    {}, // LastBuiltin,
  };

  static_assert(EZ_ARRAY_SIZE(s_TypeToUserDataContexts) == ezVisualScriptNodeDescription::Type::Count);
} // namespace

const UserDataContext& GetUserDataContext(ezVisualScriptNodeDescription::Type::Enum nodeType)
{
  EZ_ASSERT_DEBUG(nodeType >= 0 && nodeType < EZ_ARRAY_SIZE(s_TypeToUserDataContexts), "Out of bounds access");
  return s_TypeToUserDataContexts[nodeType];
}
