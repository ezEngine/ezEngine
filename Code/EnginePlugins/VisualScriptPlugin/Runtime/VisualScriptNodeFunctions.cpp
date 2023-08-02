#include <Core/Scripting/ScriptWorldModule.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>
#include <VisualScriptPlugin/Runtime/VisualScriptNodeUserData.h>

using ExecResult = ezVisualScriptGraphDescription::ExecResult;
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
      &funcName<ezScriptCoroutineHandle>,                                                                             \
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
ezStringView GetTypeName()
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
  static EZ_FORCE_INLINE ezResult FillFunctionArgs(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node, const ezAbstractFunctionProperty* pFunction, ezUInt32 uiStartSlot, ezDynamicArray<ezVariant>& out_args)
  {
    const ezUInt32 uiArgCount = pFunction->GetArgumentCount();
    if (uiArgCount != node.m_NumInputDataOffsets - uiStartSlot)
    {
      ezLog::Error("Visual script {} '{}': Argument count mismatch. Script needs re-transform.", ezVisualScriptNodeDescription::Type::GetName(node.m_Type), pFunction->GetPropertyName());
      return EZ_FAILURE;
    }

    for (ezUInt32 i = 0; i < uiArgCount; ++i)
    {
      const ezRTTI* pArgType = pFunction->GetArgumentType(i);
      out_args.PushBack(inout_context.GetDataAsVariant(node.GetInputDataOffset(uiStartSlot + i), pArgType));
    }

    return EZ_SUCCESS;
  }

  static EZ_FORCE_INLINE ezScriptWorldModule* GetScriptModule(ezVisualScriptExecutionContext& inout_context)
  {
    ezWorld* pWorld = inout_context.GetInstance().GetWorld();
    if (pWorld == nullptr)
    {
      ezLog::Error("Visual script coroutines need a script instance with a valid ezWorld");
      return nullptr;
    }

    return pWorld->GetOrCreateModule<ezScriptWorldModule>();
  }

  static ExecResult NodeFunction_ReflectedFunction(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_TypeAndProperty>();
    EZ_ASSERT_DEBUG(userData.m_pProperty->GetCategory() == ezPropertyCategory::Function, "Property '{}' is not a function", userData.m_pProperty->GetPropertyName());
    auto pFunction = static_cast<const ezAbstractFunctionProperty*>(userData.m_pProperty);

    ezTypedPointer pInstance;
    ezUInt32 uiSlot = 0;

    if (pFunction->GetFunctionType() == ezFunctionType::Member)
    {
      pInstance = inout_context.GetPointerData(node.GetInputDataOffset(0));
      if (pInstance.m_pObject == nullptr)
      {
        ezLog::Error("Visual script function call '{}': Target object is invalid (nullptr)", pFunction->GetPropertyName());
        return ExecResult::Error();
      }

      if (pInstance.m_pType->IsDerivedFrom(userData.m_pType) == false)
      {
        ezLog::Error("Visual script function call '{}': Target object is not of expected type '{}'", pFunction->GetPropertyName(), userData.m_pType->GetTypeName());
        return ExecResult::Error();
      }

      ++uiSlot;
    }

    ezHybridArray<ezVariant, 8> args;
    if (FillFunctionArgs(inout_context, node, pFunction, uiSlot, args).Failed())
    {
      return ExecResult::Error();
    }

    ezVariant returnValue;
    pFunction->Execute(pInstance.m_pObject, args, returnValue);

    if (returnValue.IsValid())
    {
      inout_context.SetDataFromVariant(node.GetOutputDataOffset(0), returnValue);
    }

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_InplaceCoroutine(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    ezScriptCoroutine* pCoroutine = inout_context.GetCurrentCoroutine();
    if (pCoroutine == nullptr)
    {
      auto pModule = GetScriptModule(inout_context);
      if (pModule == nullptr)
        return ExecResult::Error();

      auto& userData = node.GetUserData<NodeUserData_TypeAndProperty>();
      pModule->CreateCoroutine(userData.m_pType, userData.m_pType->GetTypeName(), inout_context.GetInstance(), ezScriptCoroutineCreationMode::AllowOverlap, pCoroutine);

      EZ_ASSERT_DEBUG(userData.m_pProperty->GetCategory() == ezPropertyCategory::Function, "Property '{}' is not a function", userData.m_pProperty->GetPropertyName());
      auto pFunction = static_cast<const ezAbstractFunctionProperty*>(userData.m_pProperty);

      ezHybridArray<ezVariant, 8> args;
      if (FillFunctionArgs(inout_context, node, pFunction, 0, args).Failed())
      {
        return ExecResult::Error();
      }

      pCoroutine->Start(args);

      inout_context.SetCurrentCoroutine(pCoroutine);
    }

    auto result = pCoroutine->Update(inout_context.GetDeltaTimeSinceLastExecution());
    if (result.m_State == ezScriptCoroutine::Result::State::Running)
    {
      return ExecResult::ContinueLater(result.m_MaxDelay);
    }

    ezWorld* pWorld = inout_context.GetInstance().GetWorld();
    auto pModule = pWorld->GetOrCreateModule<ezScriptWorldModule>();
    pModule->StopAndDeleteCoroutine(pCoroutine->GetHandle());
    inout_context.SetCurrentCoroutine(nullptr);

    return ExecResult::RunNext(result.m_State == ezScriptCoroutine::Result::State::Completed ? 0 : 1);
  }

  static ExecResult NodeFunction_GetScriptOwner(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    ezWorld* pWorld = inout_context.GetInstance().GetWorld();
    inout_context.SetPointerData(node.GetOutputDataOffset(0), pWorld, ezGetStaticRTTI<ezWorld>());

    ezReflectedClass& owner = inout_context.GetInstance().GetOwner();
    if (auto pComponent = ezDynamicCast<ezComponent*>(&owner))
    {
      inout_context.SetPointerData(node.GetOutputDataOffset(1), pComponent->GetOwner());
      inout_context.SetPointerData(node.GetOutputDataOffset(2), pComponent);
    }
    else
    {
      inout_context.SetPointerData(node.GetOutputDataOffset(1), &owner, owner.GetDynamicRTTI());
    }

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  static ExecResult NodeFunction_Builtin_Branch(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    bool bCondition = inout_context.GetData<bool>(node.GetInputDataOffset(0));
    return ExecResult::RunNext(bCondition ? 0 : 1);
  }

  static ExecResult NodeFunction_Builtin_And(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    bool a = inout_context.GetData<bool>(node.GetInputDataOffset(0));
    bool b = inout_context.GetData<bool>(node.GetInputDataOffset(1));
    inout_context.SetData(node.GetOutputDataOffset(0), a && b);
    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Or(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    bool a = inout_context.GetData<bool>(node.GetInputDataOffset(0));
    bool b = inout_context.GetData<bool>(node.GetInputDataOffset(1));
    inout_context.SetData(node.GetOutputDataOffset(0), a || b);
    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Not(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    bool a = inout_context.GetData<bool>(node.GetInputDataOffset(0));
    inout_context.SetData(node.GetOutputDataOffset(0), !a);
    return ExecResult::RunNext(0);
  }

  template <typename T>
  static ExecResult NodeFunction_Builtin_Compare(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
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
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      bRes = ezComparisonOperator::Compare(userData.m_ComparisonOperator, a, b);
    }
    else if constexpr (std::is_same<T, ezGameObjectHandle>::value ||
                       std::is_same<T, ezComponentHandle>::value ||
                       std::is_same<T, ezTypedPointer>::value)
    {
      ezTypedPointer a = inout_context.GetPointerData(node.GetInputDataOffset(0));
      ezTypedPointer b = inout_context.GetPointerData(node.GetInputDataOffset(1));
      bRes = ezComparisonOperator::Compare(userData.m_ComparisonOperator, a.m_pObject, b.m_pObject);
    }
    else if constexpr (std::is_same<T, ezQuat>::value ||
                       std::is_same<T, ezTransform>::value ||
                       std::is_same<T, ezVariant>::value ||
                       std::is_same<T, ezVariantArray>::value ||
                       std::is_same<T, ezVariantDictionary>::value)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));

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

    inout_context.SetData(node.GetOutputDataOffset(0), bRes);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Compare);

  template <typename T>
  static ExecResult NodeFunction_Builtin_IsValid(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    bool bIsValid = true;
    if constexpr (std::is_same<T, float>::value)
    {
      bIsValid = ezMath::IsFinite(inout_context.GetData<float>(dataOffset));
    }
    else if constexpr (std::is_same<T, double>::value)
    {
      bIsValid = ezMath::IsFinite(inout_context.GetData<double>(dataOffset));
    }
    else if constexpr (std::is_same<T, ezColor>::value)
    {
      bIsValid = inout_context.GetData<ezColor>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same<T, ezVec3>::value)
    {
      bIsValid = inout_context.GetData<ezVec3>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same<T, ezQuat>::value)
    {
      bIsValid = inout_context.GetData<ezQuat>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same<T, ezString>::value || std::is_same<T, ezStringView>::value)
    {
      bIsValid = inout_context.GetData<ezString>(dataOffset).IsEmpty() == false;
    }
    else if constexpr (std::is_same<T, ezGameObjectHandle>::value ||
                       std::is_same<T, ezComponentHandle>::value ||
                       std::is_same<T, ezTypedPointer>::value)
    {
      bIsValid = inout_context.GetPointerData(dataOffset).m_pObject != nullptr;
    }
    else if constexpr (std::is_same<T, ezVariant>::value)
    {
      bIsValid = inout_context.GetData<ezVariant>(dataOffset).IsValid();
    }

    inout_context.SetData(node.GetOutputDataOffset(0), bIsValid);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_IsValid);

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static ExecResult NodeFunction_Builtin_Add(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
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
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a + b));
    }
    else
    {
      ezLog::Error("Add is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Add);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Sub(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
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
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a - b));
    }
    else
    {
      ezLog::Error("Subtract is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Sub);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Mul(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same<T, ezUInt8>::value ||
                  std::is_same<T, ezInt32>::value ||
                  std::is_same<T, ezInt64>::value ||
                  std::is_same<T, float>::value ||
                  std::is_same<T, double>::value ||
                  std::is_same<T, ezColor>::value ||
                  std::is_same<T, ezTime>::value)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a * b));
    }
    else if constexpr (std::is_same<T, ezVec3>::value)
    {
      const ezVec3& a = inout_context.GetData<ezVec3>(node.GetInputDataOffset(0));
      const ezVec3& b = inout_context.GetData<ezVec3>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), a.CompMul(b));
    }
    else if constexpr (std::is_same<T, ezAngle>::value)
    {
      const ezAngle& a = inout_context.GetData<ezAngle>(node.GetInputDataOffset(0));
      const ezAngle& b = inout_context.GetData<ezAngle>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), ezAngle(a * b.GetRadian()));
    }
    else
    {
      ezLog::Error("Multiply is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Mul);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Div(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same<T, ezUInt8>::value ||
                  std::is_same<T, ezInt32>::value ||
                  std::is_same<T, ezInt64>::value ||
                  std::is_same<T, float>::value ||
                  std::is_same<T, double>::value ||
                  std::is_same<T, ezTime>::value)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a / b));
    }
    else if constexpr (std::is_same<T, ezVec3>::value)
    {
      const ezVec3& a = inout_context.GetData<ezVec3>(node.GetInputDataOffset(0));
      const ezVec3& b = inout_context.GetData<ezVec3>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), a.CompDiv(b));
    }
    else if constexpr (std::is_same<T, ezAngle>::value)
    {
      const ezAngle& a = inout_context.GetData<ezAngle>(node.GetInputDataOffset(0));
      const ezAngle& b = inout_context.GetData<ezAngle>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), ezAngle(a / b.GetRadian()));
    }
    else
    {
      ezLog::Error("Divide is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Div);

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static ExecResult NodeFunction_Builtin_ToBool(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    bool bRes = false;
    if constexpr (std::is_same<T, bool>::value)
    {
      bRes = inout_context.GetData<T>(dataOffset);
    }
    else if constexpr (std::is_same<T, ezUInt8>::value ||
                       std::is_same<T, ezInt32>::value ||
                       std::is_same<T, ezInt64>::value ||
                       std::is_same<T, float>::value ||
                       std::is_same<T, double>::value)
    {
      bRes = inout_context.GetData<T>(dataOffset) != 0;
    }
    else if constexpr (std::is_same<T, ezGameObjectHandle>::value ||
                       std::is_same<T, ezComponentHandle>::value ||
                       std::is_same<T, ezTypedPointer>::value)
    {
      bRes = inout_context.GetPointerData(dataOffset).m_pObject != nullptr;
    }
    else
    {
      ezLog::Error("ToBool is not defined for type '{}'", GetTypeName<T>());
    }

    inout_context.SetData(node.GetOutputDataOffset(0), bRes);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToBool);

  template <typename NumberType, typename T>
  EZ_FORCE_INLINE static ExecResult NodeFunction_Builtin_ToNumber(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node, const char* szName)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    NumberType res = 0;
    if constexpr (std::is_same<T, bool>::value)
    {
      res = inout_context.GetData<T>(dataOffset) ? 1 : 0;
    }
    else if constexpr (std::is_same<T, ezUInt8>::value ||
                       std::is_same<T, ezInt32>::value ||
                       std::is_same<T, ezInt64>::value ||
                       std::is_same<T, float>::value ||
                       std::is_same<T, double>::value)
    {
      res = static_cast<NumberType>(inout_context.GetData<T>(dataOffset));
    }
    else
    {
      ezLog::Error("To{} is not defined for type '{}'", szName, GetTypeName<T>());
    }

    inout_context.SetData(node.GetOutputDataOffset(0), res);
    return ExecResult::RunNext(0);
  }

#define MAKE_TONUMBER_EXEC_FUNC(NumberType, Name)                                                                                                              \
  template <typename T>                                                                                                                                        \
  static ExecResult EZ_CONCAT(NodeFunction_Builtin_To, Name)(ezVisualScriptExecutionContext & inout_context, const ezVisualScriptGraphDescription::Node& node) \
  {                                                                                                                                                            \
    return NodeFunction_Builtin_ToNumber<NumberType, T>(inout_context, node, #Name);                                                                           \
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
  static ExecResult NodeFunction_Builtin_ToString(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    ezStringBuilder s;
    if constexpr (std::is_same<T, ezGameObjectHandle>::value ||
                  std::is_same<T, ezComponentHandle>::value ||
                  std::is_same<T, ezTypedPointer>::value)
    {
      ezTypedPointer p = inout_context.GetPointerData(node.GetInputDataOffset(0));
      s.Format("{} {}", p.m_pType->GetTypeName(), ezArgP(p.m_pObject));
    }
    else
    {
      ezConversionUtils::ToString(inout_context.GetData<T>(node.GetInputDataOffset(0)), s);
    }
    inout_context.SetData(node.GetOutputDataOffset(0), ezString(s));
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToString);

  template <typename T>
  static ExecResult NodeFunction_Builtin_ToVariant(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    ezVariant v;
    if constexpr (std::is_same<T, ezTypedPointer>::value)
    {
      ezTypedPointer p = inout_context.GetPointerData(node.GetInputDataOffset(0));
      v = ezVariant(p.m_pObject, p.m_pType);
    }
    else
    {
      v = inout_context.GetData<T>(node.GetInputDataOffset(0));
    }
    inout_context.SetData(node.GetOutputDataOffset(0), v);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToVariant);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Variant_ConvertTo(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    const ezVariant& v = inout_context.GetData<ezVariant>(node.GetInputDataOffset(0));
    if constexpr (std::is_same<T, ezTypedPointer>::value)
    {
      if (v.IsA<ezTypedPointer>())
      {
        ezTypedPointer typedPtr = v.Get<ezTypedPointer>();
        inout_context.SetPointerData(node.GetOutputDataOffset(0), typedPtr.m_pObject, typedPtr.m_pType);
        return ExecResult::RunNext(0);
      }

      inout_context.SetPointerData<void*>(node.GetOutputDataOffset(0), nullptr, nullptr);
      return ExecResult::RunNext(1);
    }
    else if constexpr (std::is_same<T, ezVariant>::value)
    {
      inout_context.SetData(node.GetOutputDataOffset(0), v);
      return ExecResult::RunNext(0);
    }
    else
    {
      ezResult conversionResult = EZ_SUCCESS;
      inout_context.SetData(node.GetOutputDataOffset(0), v.ConvertTo<T>(&conversionResult));
      return ExecResult::RunNext(conversionResult.Succeeded() ? 0 : 1);
    }
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Variant_ConvertTo);

  //////////////////////////////////////////////////////////////////////////

  static ExecResult NodeFunction_Builtin_MakeArray(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    ezVariantArray& a = inout_context.GetWritableData<ezVariantArray>(node.GetOutputDataOffset(0));
    a.Clear();
    a.Reserve(node.m_NumInputDataOffsets);

    for (ezUInt32 i = 0; i < node.m_NumInputDataOffsets; ++i)
    {
      auto dataOffset = node.GetInputDataOffset(i);

      if (dataOffset.IsConstant())
      {
        a.PushBack(inout_context.GetDataAsVariant(dataOffset, nullptr));
      }
      else
      {
        a.PushBack(inout_context.GetData<ezVariant>(dataOffset));
      }
    }

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  static ExecResult NodeFunction_Builtin_TryGetComponentOfBaseType(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_Type>();

    ezTypedPointer p = inout_context.GetPointerData(node.GetInputDataOffset(0));
    if (p.m_pType != ezGetStaticRTTI<ezGameObject>())
    {
      ezLog::Error("Visual script call TryGetComponentOfBaseType: Game object is not of type 'ezGameObject'");
      return ExecResult::RunNext(0);
    }

    ezComponent* pComponent = nullptr;
    static_cast<ezGameObject*>(p.m_pObject)->TryGetComponentOfBaseType(userData.m_pType, pComponent);
    inout_context.SetPointerData(node.GetOutputDataOffset(0), pComponent);

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  static ExecResult NodeFunction_Builtin_StartCoroutine(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    auto& userData = node.GetUserData<NodeUserData_StartCoroutine>();
    ezString sName = inout_context.GetData<ezString>(node.GetInputDataOffset(0));

    ezScriptCoroutine* pCoroutine = nullptr;
    auto hCoroutine = pModule->CreateCoroutine(userData.m_pType, sName, inout_context.GetInstance(), userData.m_CreationMode, pCoroutine);
    pModule->StartCoroutine(hCoroutine, ezArrayPtr<ezVariant>());

    inout_context.SetData(node.GetOutputDataOffset(0), hCoroutine);


    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_StopCoroutine(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    auto hCoroutine = inout_context.GetData<ezScriptCoroutineHandle>(node.GetInputDataOffset(0));
    if (pModule->IsCoroutineFinished(hCoroutine) == false)
    {
      pModule->StopAndDeleteCoroutine(hCoroutine);
    }
    else
    {
      auto sName = inout_context.GetData<ezString>(node.GetInputDataOffset(1));
      pModule->StopAndDeleteCoroutine(sName, &inout_context.GetInstance());
    }

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_StopAllCoroutines(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    pModule->StopAndDeleteAllCoroutines(&inout_context.GetInstance());

    return ExecResult::RunNext(0);
  }

  template <bool bWaitForAll>
  static ExecResult NodeFunction_Builtin_WaitForX(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    const ezUInt32 uiNumCoroutines = node.m_NumInputDataOffsets;
    ezUInt32 uiNumFinishedCoroutines = 0;

    for (ezUInt32 i = 0; i < uiNumCoroutines; ++i)
    {
      auto hCoroutine = inout_context.GetData<ezScriptCoroutineHandle>(node.GetInputDataOffset(i));
      if (pModule->IsCoroutineFinished(hCoroutine))
      {
        if constexpr (bWaitForAll == false)
        {
          return ExecResult::RunNext(0);
        }
        else
        {
          ++uiNumFinishedCoroutines;
        }
      }
    }

    if constexpr (bWaitForAll)
    {
      if (uiNumFinishedCoroutines == uiNumCoroutines)
      {
        return ExecResult::RunNext(0);
      }
    }

    return ExecResult::ContinueLater(ezTime::MakeZero());
  }

  static ExecResult NodeFunction_Builtin_Yield(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    ezScriptCoroutine* pCoroutine = inout_context.GetCurrentCoroutine();
    if (pCoroutine == nullptr)
    {
      // set marker value of 0x1 to indicate we are in a yield
      inout_context.SetCurrentCoroutine(reinterpret_cast<ezScriptCoroutine*>(0x1));

      return ExecResult::ContinueLater(ezTime::MakeZero());
    }

    inout_context.SetCurrentCoroutine(nullptr);

    return ExecResult::RunNext(0);
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
    {},                                // EntryCall_Coroutine,
    {},                                // MessageHandler,
    {},                                // MessageHandler_Coroutine,
    {&NodeFunction_ReflectedFunction}, // ReflectedFunction,
    {&NodeFunction_InplaceCoroutine},  // InplaceCoroutine,
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

    {&NodeFunction_Builtin_StartCoroutine},    // Builtin_StartCoroutine,
    {&NodeFunction_Builtin_StopCoroutine},     // Builtin_StopCoroutine,
    {&NodeFunction_Builtin_StopAllCoroutines}, // Builtin_StopAllCoroutines,
    {&NodeFunction_Builtin_WaitForX<true>},    // Builtin_WaitForAll,
    {&NodeFunction_Builtin_WaitForX<false>},   // Builtin_WaitForAny,
    {&NodeFunction_Builtin_Yield},             // Builtin_Yield,

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
