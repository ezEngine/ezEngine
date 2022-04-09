#pragma once

#include <Core/World/World.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>
#include <VisualScriptPlugin/Runtime/VisualScriptNodeUserData.h>

using ExecuteFunctionGetter = ezVisualScriptGraphDescription::ExecuteFunction (*)(ezVisualScriptDataType::Enum dataType);

#define MAKE_EXEC_FUNC_GETTER(funcName)                                                                               \
  ezVisualScriptGraphDescription::ExecuteFunction EZ_CONCAT(funcName, _Getter)(ezVisualScriptDataType::Enum dataType) \
  {                                                                                                                   \
    static ezVisualScriptGraphDescription::ExecuteFunction functionTable[] = {                                        \
      nullptr, /* Invalid*/                                                                                           \
      &funcName<bool>,                                                                                                \
      &funcName<ezUInt8>,                                                                                             \
      &funcName<ezInt32>,                                                                                             \
      &funcName<ezInt64>,                                                                                             \
      &funcName<float>,                                                                                               \
      &funcName<double>,                                                                                              \
      &funcName<ezColor>,                                                                                             \
      &funcName<ezVec3>,                                                                                              \
      &funcName<ezQuat>,                                                                                              \
      &funcName<ezTransform>,                                                                                         \
      &funcName<ezTime>,                                                                                              \
      &funcName<ezAngle>,                                                                                             \
      &funcName<ezString>,                                                                                            \
      &funcName<ezGameObjectHandle>,                                                                                  \
      &funcName<ezComponentHandle>,                                                                                   \
      &funcName<ezTypedPointer>,                                                                                      \
      &funcName<ezVariant>,                                                                                           \
      &funcName<ezVariantArray>,                                                                                      \
      &funcName<ezVariantDictionary>,                                                                                 \
    };                                                                                                                \
                                                                                                                      \
    static_assert(EZ_ARRAY_SIZE(functionTable) == ezVisualScriptDataType::Count);                                     \
    if (dataType >= 0 && dataType < EZ_ARRAY_SIZE(functionTable))                                                     \
      return functionTable[dataType];                                                                                 \
                                                                                                                      \
    ezLog::Error("Invalid data type for deducted type {}. Script needs re-transform.", dataType);                     \
    return nullptr;                                                                                                   \
  }

template <typename T>
const char* GetTypeName()
{
  if constexpr (std::is_same<T, ezTypedPointer>::value)
  {
    return "ezTypePointer";
  }
  else
  {
    return ezGetStaticRTTI<T>()->GetTypeName();
  }
}

namespace
{
  static int NodeFunction_ReflectedFunction(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_TypeAndProperty>();
    EZ_ASSERT_DEBUG(userData.m_pProperty->GetCategory() == ezPropertyCategory::Function, "Property '{}' is not a function", userData.m_pProperty->GetPropertyName());
    auto pFunction = static_cast<const ezAbstractFunctionProperty*>(userData.m_pProperty);

    ezTypedPointer pInstance;
    ezUInt32 uiSlot = 0;

    if (pFunction->GetFunctionType() == ezFunctionType::Member)
    {
      pInstance = ref_instance.GetPointerData(node.GetInputDataOffset(0));
      if (pInstance.m_pObject == nullptr)
      {
        ezLog::Error("Visual script function call '{}': Target object is invalid (nullptr)", pFunction->GetPropertyName());
        return ezVisualScriptGraphDescription::ReturnValue::Error;
      }

      if (pInstance.m_pType->IsDerivedFrom(userData.m_pType) == false)
      {
        ezLog::Error("Visual script function call '{}': Target object is not of expected type '{}'", pFunction->GetPropertyName(), userData.m_pType->GetTypeName());
        return ezVisualScriptGraphDescription::ReturnValue::Error;
      }

      ++uiSlot;
    }

    ezHybridArray<ezVariant, 8> args;
    ezUInt32 uiArgCount = pFunction->GetArgumentCount();
    if (uiArgCount != node.m_NumInputDataOffsets - uiSlot)
    {
      ezLog::Error("Visual script function call '{}': Argument count mismatch. Script needs re-transform.", pFunction->GetPropertyName());
      return ezVisualScriptGraphDescription::ReturnValue::Error;
    }

    for (ezUInt32 uiArgIndex = 0; uiArgIndex < uiArgCount; ++uiArgIndex)
    {
      const ezRTTI* pArgType = pFunction->GetArgumentType(uiArgIndex);
      ezVariantType::Enum expectedType = pArgType->GetVariantType();
      args.PushBack(ref_instance.GetDataAsVariant(node.GetInputDataOffset(uiSlot), expectedType));

      ++uiSlot;
    }

    ezVariant returnValue;
    pFunction->Execute(pInstance.m_pObject, args, returnValue);

    uiSlot = 0;
    if (returnValue.IsValid())
    {
      ref_instance.SetDataFromVariant(node.GetOutputDataOffset(0), returnValue);
    }

    return 0;
  }

  static int NodeFunction_GetScriptOwner(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    ezWorld* pWorld = ref_instance.GetWorld();
    ref_instance.SetPointerData(node.GetOutputDataOffset(0), pWorld, ezGetStaticRTTI<ezWorld>());

    ezReflectedClass& owner = ref_instance.GetOwner();
    if (auto pComponent = ezDynamicCast<ezComponent*>(&owner))
    {
      ref_instance.SetPointerData(node.GetOutputDataOffset(1), pComponent->GetOwner());
      ref_instance.SetPointerData(node.GetOutputDataOffset(2), pComponent);
    }
    else
    {
      ref_instance.SetPointerData(node.GetOutputDataOffset(1), &owner, owner.GetDynamicRTTI());
    }

    return 0;
  }

  //////////////////////////////////////////////////////////////////////////

  static int NodeFunction_Builtin_Branch(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    bool bCondition = ref_instance.GetData<bool>(node.GetInputDataOffset(0));
    return bCondition ? 0 : 1;
  }

  static int NodeFunction_Builtin_And(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    bool a = ref_instance.GetData<bool>(node.GetInputDataOffset(0));
    bool b = ref_instance.GetData<bool>(node.GetInputDataOffset(1));
    ref_instance.SetData(node.GetOutputDataOffset(0), a && b);
    return 0;
  }

  static int NodeFunction_Builtin_Or(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    bool a = ref_instance.GetData<bool>(node.GetInputDataOffset(0));
    bool b = ref_instance.GetData<bool>(node.GetInputDataOffset(1));
    ref_instance.SetData(node.GetOutputDataOffset(0), a || b);
    return 0;
  }

  static int NodeFunction_Builtin_Not(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    bool a = ref_instance.GetData<bool>(node.GetInputDataOffset(0));
    ref_instance.SetData(node.GetOutputDataOffset(0), !a);
    return 0;
  }

  template <typename T>
  static int NodeFunction_Builtin_Compare(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_Comparison>();
    bool bRes = false;

    if constexpr (std::is_same<T, bool>::value ||
                  std::is_same<T, ezUInt8>::value ||
                  std::is_same<T, ezInt32>::value ||
                  std::is_same<T, ezInt64>::value ||
                  std::is_same<T, float>::value ||
                  std::is_same<T, double>::value ||
                  std::is_same<T, ezColor>::value ||
                  std::is_same<T, ezVec3>::value ||
                  std::is_same<T, ezTime>::value ||
                  std::is_same<T, ezAngle>::value ||
                  std::is_same<T, ezString>::value)
    {
      const T& a = ref_instance.GetData<T>(node.GetInputDataOffset(0));
      const T& b = ref_instance.GetData<T>(node.GetInputDataOffset(1));
      bRes = ezComparisonOperator::Compare(userData.m_ComparisonOperator, a, b);
    }
    else if constexpr (std::is_same<T, ezGameObjectHandle>::value ||
                       std::is_same<T, ezComponentHandle>::value ||
                       std::is_same<T, ezTypedPointer>::value)
    {
      ezTypedPointer a = ref_instance.GetPointerData(node.GetInputDataOffset(0));
      ezTypedPointer b = ref_instance.GetPointerData(node.GetInputDataOffset(1));
      bRes = ezComparisonOperator::Compare(userData.m_ComparisonOperator, a.m_pObject, b.m_pObject);
    }
    else if constexpr (std::is_same<T, ezQuat>::value ||
                       std::is_same<T, ezTransform>::value ||
                       std::is_same<T, ezVariant>::value ||
                       std::is_same<T, ezVariantArray>::value ||
                       std::is_same<T, ezVariantDictionary>::value)
    {
      const T& a = ref_instance.GetData<T>(node.GetInputDataOffset(0));
      const T& b = ref_instance.GetData<T>(node.GetInputDataOffset(1));

      if (userData.m_ComparisonOperator == ezComparisonOperator::Equal)
      {
        bRes = a == b;
      }
      else if (userData.m_ComparisonOperator == ezComparisonOperator::NotEqual)
      {
        bRes = a != b;
      }
      else
      {
        ezStringBuilder sCompOp;
        ezReflectionUtils::EnumerationToString(userData.m_ComparisonOperator, sCompOp, ezReflectionUtils::EnumConversionMode::ValueNameOnly);

        ezLog::Error("Comparison '{}' is not defined for type '{}'", sCompOp, GetTypeName<T>());
      }
    }
    else
    {
      ezLog::Error("Comparison is not defined for type '{}'", GetTypeName<T>());
    }

    ref_instance.SetData(node.GetOutputDataOffset(0), bRes);
    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Compare);

  template <typename T>
  static int NodeFunction_Builtin_IsValid(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    bool bIsValid = true;
    if constexpr (std::is_same<T, float>::value)
    {
      bIsValid = ezMath::IsFinite(ref_instance.GetData<float>(dataOffset));
    }
    else if constexpr (std::is_same<T, double>::value)
    {
      bIsValid = ezMath::IsFinite(ref_instance.GetData<double>(dataOffset));
    }
    else if constexpr (std::is_same<T, ezColor>::value)
    {
      bIsValid = ref_instance.GetData<ezColor>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same<T, ezVec3>::value)
    {
      bIsValid = ref_instance.GetData<ezVec3>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same<T, ezQuat>::value)
    {
      bIsValid = ref_instance.GetData<ezQuat>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same<T, ezString>::value || std::is_same<T, ezStringView>::value)
    {
      bIsValid = ref_instance.GetData<ezString>(dataOffset).IsEmpty() == false;
    }
    else if constexpr (std::is_same<T, ezGameObjectHandle>::value ||
                       std::is_same<T, ezComponentHandle>::value ||
                       std::is_same<T, ezTypedPointer>::value)
    {
      bIsValid = ref_instance.GetPointerData(dataOffset).m_pObject != nullptr;
    }
    else if constexpr (std::is_same<T, ezVariant>::value)
    {
      bIsValid = ref_instance.GetData<ezVariant>(dataOffset).IsValid();
    }

    ref_instance.SetData(node.GetOutputDataOffset(0), bIsValid);
    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_IsValid);

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static int NodeFunction_Builtin_Add(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same<T, ezUInt8>::value ||
                  std::is_same<T, ezInt32>::value ||
                  std::is_same<T, ezInt64>::value ||
                  std::is_same<T, float>::value ||
                  std::is_same<T, double>::value ||
                  std::is_same<T, ezColor>::value ||
                  std::is_same<T, ezVec3>::value ||
                  std::is_same<T, ezTime>::value ||
                  std::is_same<T, ezAngle>::value)
    {
      const T& a = ref_instance.GetData<T>(node.GetInputDataOffset(0));
      const T& b = ref_instance.GetData<T>(node.GetInputDataOffset(1));
      ref_instance.SetData(node.GetOutputDataOffset(0), T(a + b));
    }
    else
    {
      ezLog::Error("Add is not defined for type '{}'", GetTypeName<T>());
    }

    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Add);

  template <typename T>
  static int NodeFunction_Builtin_Sub(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same<T, ezUInt8>::value ||
                  std::is_same<T, ezInt32>::value ||
                  std::is_same<T, ezInt64>::value ||
                  std::is_same<T, float>::value ||
                  std::is_same<T, double>::value ||
                  std::is_same<T, ezColor>::value ||
                  std::is_same<T, ezVec3>::value ||
                  std::is_same<T, ezTime>::value ||
                  std::is_same<T, ezAngle>::value)
    {
      const T& a = ref_instance.GetData<T>(node.GetInputDataOffset(0));
      const T& b = ref_instance.GetData<T>(node.GetInputDataOffset(1));
      ref_instance.SetData(node.GetOutputDataOffset(0), T(a - b));
    }
    else
    {
      ezLog::Error("Subtract is not defined for type '{}'", GetTypeName<T>());
    }

    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Sub);

  template <typename T>
  static int NodeFunction_Builtin_Mul(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same<T, ezUInt8>::value ||
                  std::is_same<T, ezInt32>::value ||
                  std::is_same<T, ezInt64>::value ||
                  std::is_same<T, float>::value ||
                  std::is_same<T, double>::value ||
                  std::is_same<T, ezColor>::value ||
                  std::is_same<T, ezTime>::value)
    {
      const T& a = ref_instance.GetData<T>(node.GetInputDataOffset(0));
      const T& b = ref_instance.GetData<T>(node.GetInputDataOffset(1));
      ref_instance.SetData(node.GetOutputDataOffset(0), T(a * b));
    }
    else if constexpr (std::is_same<T, ezVec3>::value)
    {
      const ezVec3& a = ref_instance.GetData<ezVec3>(node.GetInputDataOffset(0));
      const ezVec3& b = ref_instance.GetData<ezVec3>(node.GetInputDataOffset(1));
      ref_instance.SetData(node.GetOutputDataOffset(0), a.CompMul(b));
    }
    else if constexpr (std::is_same<T, ezAngle>::value)
    {
      const ezAngle& a = ref_instance.GetData<ezAngle>(node.GetInputDataOffset(0));
      const ezAngle& b = ref_instance.GetData<ezAngle>(node.GetInputDataOffset(1));
      ref_instance.SetData(node.GetOutputDataOffset(0), ezAngle(a * b.GetRadian()));
    }
    else
    {
      ezLog::Error("Multiply is not defined for type '{}'", GetTypeName<T>());
    }

    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Mul);

  template <typename T>
  static int NodeFunction_Builtin_Div(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same<T, ezUInt8>::value ||
                  std::is_same<T, ezInt32>::value ||
                  std::is_same<T, ezInt64>::value ||
                  std::is_same<T, float>::value ||
                  std::is_same<T, double>::value ||
                  std::is_same<T, ezTime>::value)
    {
      const T& a = ref_instance.GetData<T>(node.GetInputDataOffset(0));
      const T& b = ref_instance.GetData<T>(node.GetInputDataOffset(1));
      ref_instance.SetData(node.GetOutputDataOffset(0), T(a / b));
    }
    else if constexpr (std::is_same<T, ezVec3>::value)
    {
      const ezVec3& a = ref_instance.GetData<ezVec3>(node.GetInputDataOffset(0));
      const ezVec3& b = ref_instance.GetData<ezVec3>(node.GetInputDataOffset(1));
      ref_instance.SetData(node.GetOutputDataOffset(0), a.CompDiv(b));
    }
    else if constexpr (std::is_same<T, ezAngle>::value)
    {
      const ezAngle& a = ref_instance.GetData<ezAngle>(node.GetInputDataOffset(0));
      const ezAngle& b = ref_instance.GetData<ezAngle>(node.GetInputDataOffset(1));
      ref_instance.SetData(node.GetOutputDataOffset(0), ezAngle(a / b.GetRadian()));
    }
    else
    {
      ezLog::Error("Divide is not defined for type '{}'", GetTypeName<T>());
    }

    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Div);

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static int NodeFunction_Builtin_ToBool(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    bool bRes = false;
    if constexpr (std::is_same<T, bool>::value)
    {
      bRes = ref_instance.GetData<T>(dataOffset);
    }
    else if constexpr (std::is_same<T, ezUInt8>::value ||
                       std::is_same<T, ezInt32>::value ||
                       std::is_same<T, ezInt64>::value ||
                       std::is_same<T, float>::value ||
                       std::is_same<T, double>::value)
    {
      bRes = ref_instance.GetData<T>(dataOffset) != 0;
    }
    else if constexpr (std::is_same<T, ezGameObjectHandle>::value ||
                       std::is_same<T, ezComponentHandle>::value ||
                       std::is_same<T, ezTypedPointer>::value)
    {
      bRes = ref_instance.GetPointerData(dataOffset).m_pObject != nullptr;
    }
    else
    {
      ezLog::Error("ToBool is not defined for type '{}'", GetTypeName<T>());
    }

    ref_instance.SetData(node.GetOutputDataOffset(0), bRes);
    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToBool);

  template <typename NumberType, typename T>
  EZ_FORCE_INLINE static int NodeFunction_Builtin_ToNumber(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node, const char* szName)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    NumberType res = 0;
    if constexpr (std::is_same<T, bool>::value)
    {
      res = ref_instance.GetData<T>(dataOffset) ? 1 : 0;
    }
    else if constexpr (std::is_same<T, ezUInt8>::value ||
                       std::is_same<T, ezInt32>::value ||
                       std::is_same<T, ezInt64>::value ||
                       std::is_same<T, float>::value ||
                       std::is_same<T, double>::value)
    {
      res = static_cast<NumberType>(ref_instance.GetData<T>(dataOffset));
    }
    else
    {
      ezLog::Error("To{} is not defined for type '{}'", szName, GetTypeName<T>());
    }

    ref_instance.SetData(node.GetOutputDataOffset(0), res);
    return 0;
  }

#define MAKE_TONUMBER_EXEC_FUNC(NumberType, Name)                                                                                              \
  template <typename T>                                                                                                                        \
  static int EZ_CONCAT(NodeFunction_Builtin_To, Name)(ezVisualScriptInstance & ref_instance, const ezVisualScriptGraphDescription::Node& node) \
  {                                                                                                                                            \
    return NodeFunction_Builtin_ToNumber<NumberType, T>(ref_instance, node, #Name);                                                            \
  }

  MAKE_TONUMBER_EXEC_FUNC(ezUInt8, Byte);
  MAKE_TONUMBER_EXEC_FUNC(ezInt32, Int);
  MAKE_TONUMBER_EXEC_FUNC(ezInt64, Int64);
  MAKE_TONUMBER_EXEC_FUNC(float, Float);
  MAKE_TONUMBER_EXEC_FUNC(double, Double);

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToByte);
  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToInt);
  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToInt64);
  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToFloat);
  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToDouble);

  template <typename T>
  static int NodeFunction_Builtin_ToString(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    ezStringBuilder s;
    if constexpr (std::is_same<T, ezGameObjectHandle>::value ||
                  std::is_same<T, ezComponentHandle>::value ||
                  std::is_same<T, ezTypedPointer>::value)
    {
      ezTypedPointer p = ref_instance.GetPointerData(node.GetInputDataOffset(0));
      s.Format("{} {}", p.m_pType->GetTypeName(), ezArgP(p.m_pObject));
    }
    else
    {
      ezConversionUtils::ToString(ref_instance.GetData<T>(node.GetInputDataOffset(0)), s);
    }
    ref_instance.SetData(node.GetOutputDataOffset(0), ezString(s));
    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToString);

  template <typename T>
  static int NodeFunction_Builtin_ToVariant(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    ezVariant v;
    if constexpr (std::is_same<T, ezTypedPointer>::value)
    {
      ezTypedPointer p = ref_instance.GetPointerData(node.GetInputDataOffset(0));
      v = ezVariant(p.m_pObject, p.m_pType);
    }
    else
    {
      v = ref_instance.GetData<T>(node.GetInputDataOffset(0));
    }
    ref_instance.SetData(node.GetOutputDataOffset(0), v);
    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToVariant);

  template <typename T>
  static int NodeFunction_Builtin_Variant_ConvertTo(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    const ezVariant& v = ref_instance.GetData<ezVariant>(node.GetInputDataOffset(0));
    if constexpr (std::is_same<T, ezTypedPointer>::value)
    {
      if (v.IsA<ezTypedPointer>())
      {
        ezTypedPointer typedPtr = v.Get<ezTypedPointer>();
        ref_instance.SetPointerData(node.GetOutputDataOffset(0), typedPtr.m_pObject, typedPtr.m_pType);
        return 0;
      }

      ref_instance.SetPointerData<void*>(node.GetOutputDataOffset(0), nullptr, nullptr);
      return 1;
    }
    else if constexpr (std::is_same<T, ezVariant>::value)
    {
      ref_instance.SetData(node.GetOutputDataOffset(0), v);
      return 0;
    }
    else
    {
      ezResult conversionResult = EZ_SUCCESS;
      ref_instance.SetData(node.GetOutputDataOffset(0), v.ConvertTo<T>(&conversionResult));
      return conversionResult.Succeeded() ? 0 : 1;
    }
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Variant_ConvertTo);

  //////////////////////////////////////////////////////////////////////////

  static int NodeFunction_Builtin_MakeArray(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    ezVariantArray& a = ref_instance.GetWritableData<ezVariantArray>(node.GetOutputDataOffset(0));
    a.Clear();
    a.Reserve(node.m_NumInputDataOffsets);

    for (ezUInt32 i = 0; i < node.m_NumInputDataOffsets; ++i)
    {
      auto dataOffset = node.GetInputDataOffset(i);

      if (dataOffset.m_uiIsConstant)
      {
        auto expectedType = ezVisualScriptDataType::GetVariantType(static_cast<ezVisualScriptDataType::Enum>(dataOffset.m_uiDataType));
        a.PushBack(ref_instance.GetDataAsVariant(dataOffset, expectedType));
      }
      else
      {
        a.PushBack(ref_instance.GetData<ezVariant>(dataOffset));
      }
    }

    return 0;
  }

  //////////////////////////////////////////////////////////////////////////

  static int NodeFunction_Builtin_TryGetComponentOfBaseType(ezVisualScriptInstance& ref_instance, const ezVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_Type>();

    ezTypedPointer p = ref_instance.GetPointerData(node.GetInputDataOffset(0));
    if (p.m_pType != ezGetStaticRTTI<ezGameObject>())
    {
      ezLog::Error("Visual script call TryGetComponentOfBaseType: Game object is not of type 'ezGameObject'");
      return 0;
    }

    ezComponent* pComponent = nullptr;
    static_cast<ezGameObject*>(p.m_pObject)->TryGetComponentOfBaseType(userData.m_pType, pComponent);
    ref_instance.SetPointerData(node.GetOutputDataOffset(0), pComponent);

    return 0;
  }

  //////////////////////////////////////////////////////////////////////////

  struct ExecuteFunctionContext
  {
    ezVisualScriptGraphDescription::ExecuteFunction m_Func = nullptr;
    ExecuteFunctionGetter m_FuncGetter = nullptr;
  };

  static ExecuteFunctionContext s_TypeToExecuteFunctions[] = {
    {},                                // Invalid,
    {},                                // EntryCall,
    {},                                // MessageHandler,
    {&NodeFunction_ReflectedFunction}, // ReflectedFunction,
    {&NodeFunction_GetScriptOwner},    // GetOwner,

    {}, // FirstBuiltin,

    {&NodeFunction_Builtin_Branch},                  // Builtin_Branch,
    {&NodeFunction_Builtin_And},                     // Builtin_And,
    {&NodeFunction_Builtin_Or},                      // Builtin_Or,
    {&NodeFunction_Builtin_Not},                     // Builtin_Not,
    {nullptr, &NodeFunction_Builtin_Compare_Getter}, // Builtin_Compare,
    {nullptr, &NodeFunction_Builtin_IsValid_Getter}, // Builtin_IsValid,

    {nullptr, &NodeFunction_Builtin_Add_Getter}, // Builtin_Add,
    {nullptr, &NodeFunction_Builtin_Sub_Getter}, // Builtin_Subtract,
    {nullptr, &NodeFunction_Builtin_Mul_Getter}, // Builtin_Multiply,
    {nullptr, &NodeFunction_Builtin_Div_Getter}, // Builtin_Divide,

    {nullptr, &NodeFunction_Builtin_ToBool_Getter},            // Builtin_ToBool,
    {nullptr, &NodeFunction_Builtin_ToByte_Getter},            // Builtin_ToByte,
    {nullptr, &NodeFunction_Builtin_ToInt_Getter},             // Builtin_ToInt,
    {nullptr, &NodeFunction_Builtin_ToInt64_Getter},           // Builtin_ToInt64,
    {nullptr, &NodeFunction_Builtin_ToFloat_Getter},           // Builtin_ToFloat,
    {nullptr, &NodeFunction_Builtin_ToDouble_Getter},          // Builtin_ToDouble,
    {nullptr, &NodeFunction_Builtin_ToString_Getter},          // Builtin_ToString,
    {nullptr, &NodeFunction_Builtin_ToVariant_Getter},         // Builtin_ToVariant,
    {nullptr, &NodeFunction_Builtin_Variant_ConvertTo_Getter}, // Builtin_Variant_ConvertTo,

    {&NodeFunction_Builtin_MakeArray}, // Builtin_MakeArray

    {&NodeFunction_Builtin_TryGetComponentOfBaseType}, // Builtin_TryGetComponentOfBaseType

    {}, // LastBuiltin,
  };

  static_assert(EZ_ARRAY_SIZE(s_TypeToExecuteFunctions) == ezVisualScriptNodeDescription::Type::Count);
} // namespace

ezVisualScriptGraphDescription::ExecuteFunction GetExecuteFunction(ezVisualScriptNodeDescription::Type::Enum nodeType, ezVisualScriptDataType::Enum dataType)
{
  EZ_ASSERT_DEBUG(nodeType >= 0 && nodeType < EZ_ARRAY_SIZE(s_TypeToExecuteFunctions), "Out of bounds access");
  auto& context = s_TypeToExecuteFunctions[nodeType];
  if (context.m_Func != nullptr)
  {
    return context.m_Func;
  }

  if (context.m_FuncGetter != nullptr)
  {
    return context.m_FuncGetter(dataType);
  }

  return nullptr;
}

#undef MAKE_EXEC_FUNC_GETTER
#undef MAKE_TONUMBER_EXEC_FUNC
