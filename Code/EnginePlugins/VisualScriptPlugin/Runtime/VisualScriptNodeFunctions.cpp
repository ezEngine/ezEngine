#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptWorldModule.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>
#include <VisualScriptPlugin/Runtime/VisualScriptNodeUserData.h>

using ExecResult = ezVisualScriptGraphDescription::ExecResult;
using ExecuteFunctionGetter = ezVisualScriptGraphDescription::ExecuteFunction (*)(ezVisualScriptDataType::Enum dataType);

#define MAKE_EXEC_FUNC_GETTER(funcName)                                                                                  \
  ezVisualScriptGraphDescription::ExecuteFunction EZ_PP_CONCAT(funcName, _Getter)(ezVisualScriptDataType::Enum dataType) \
  {                                                                                                                      \
    static ezVisualScriptGraphDescription::ExecuteFunction functionTable[] = {                                           \
      nullptr, /* Invalid*/                                                                                              \
      &funcName<bool>,                                                                                                   \
      &funcName<ezUInt8>,                                                                                                \
      &funcName<ezInt32>,                                                                                                \
      &funcName<ezInt64>,                                                                                                \
      &funcName<float>,                                                                                                  \
      &funcName<double>,                                                                                                 \
      &funcName<ezColor>,                                                                                                \
      &funcName<ezVec3>,                                                                                                 \
      &funcName<ezQuat>,                                                                                                 \
      &funcName<ezTransform>,                                                                                            \
      &funcName<ezTime>,                                                                                                 \
      &funcName<ezAngle>,                                                                                                \
      &funcName<ezString>,                                                                                               \
      &funcName<ezHashedString>,                                                                                         \
      &funcName<ezGameObjectHandle>,                                                                                     \
      &funcName<ezComponentHandle>,                                                                                      \
      &funcName<ezTypedPointer>,                                                                                         \
      &funcName<ezVariant>,                                                                                              \
      &funcName<ezVariantArray>,                                                                                         \
      &funcName<ezVariantDictionary>,                                                                                    \
      &funcName<ezScriptCoroutineHandle>,                                                                                \
    };                                                                                                                   \
                                                                                                                         \
    static_assert(EZ_ARRAY_SIZE(functionTable) == ezVisualScriptDataType::Count);                                        \
    if (dataType >= 0 && dataType < EZ_ARRAY_SIZE(functionTable))                                                        \
      return functionTable[dataType];                                                                                    \
                                                                                                                         \
    ezLog::Error("Invalid data type for deducted type {}. Script needs re-transform.", dataType);                        \
    return nullptr;                                                                                                      \
  }

template <typename T>
ezStringView GetTypeName()
{
  if constexpr (std::is_same_v<T, ezTypedPointer>)
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

    auto dataOffsetR = node.GetOutputDataOffset(0);
    if (dataOffsetR.IsValid())
    {
      inout_context.SetDataFromVariant(dataOffsetR, returnValue);
    }

    return ExecResult::RunNext(0);
  }

  template <typename T>
  static ExecResult NodeFunction_GetReflectedProperty(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_TypeAndProperty>();
    auto pProperty = userData.m_pProperty;

    ezTypedPointer pInstance;
    pInstance = inout_context.GetPointerData(node.GetInputDataOffset(0));

    if (pInstance.m_pObject == nullptr)
    {
      ezLog::Error("Visual script get property '{}': Target object is invalid (nullptr)", pProperty->GetPropertyName());
      return ExecResult::Error();
    }

    if (pInstance.m_pType->IsDerivedFrom(userData.m_pType) == false)
    {
      ezLog::Error("Visual script get property '{}': Target object is not of expected type '{}'", pProperty->GetPropertyName(), userData.m_pType->GetTypeName());
      return ExecResult::Error();
    }

    if (pProperty->GetCategory() == ezPropertyCategory::Member)
    {
      auto pMemberProperty = static_cast<const ezAbstractMemberProperty*>(pProperty);

      if constexpr (std::is_same_v<T, ezGameObjectHandle> ||
                    std::is_same_v<T, ezComponentHandle> ||
                    std::is_same_v<T, ezTypedPointer>)
      {
        EZ_ASSERT_NOT_IMPLEMENTED;
      }
      else
      {
        if (pProperty->GetSpecificType() == ezGetStaticRTTI<T>())
        {
          T value;
          pMemberProperty->GetValuePtr(pInstance.m_pObject, &value);
          inout_context.SetData(node.GetOutputDataOffset(0), value);
        }
        else
        {
          ezVariant value = ezReflectionUtils::GetMemberPropertyValue(pMemberProperty, pInstance.m_pObject);
          inout_context.SetDataFromVariant(node.GetOutputDataOffset(0), value);
        }
      }
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_GetReflectedProperty);

  template <typename T>
  static ExecResult NodeFunction_SetReflectedProperty(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_TypeAndProperty>();
    auto pProperty = userData.m_pProperty;

    ezTypedPointer pInstance;
    pInstance = inout_context.GetPointerData(node.GetInputDataOffset(0));

    if (pInstance.m_pObject == nullptr)
    {
      ezLog::Error("Visual script set property '{}': Target object is invalid (nullptr)", pProperty->GetPropertyName());
      return ExecResult::Error();
    }

    if (pInstance.m_pType->IsDerivedFrom(userData.m_pType) == false)
    {
      ezLog::Error("Visual script set property '{}': Target object is not of expected type '{}'", pProperty->GetPropertyName(), userData.m_pType->GetTypeName());
      return ExecResult::Error();
    }

    if (pProperty->GetCategory() == ezPropertyCategory::Member)
    {
      auto pMemberProperty = static_cast<const ezAbstractMemberProperty*>(pProperty);

      if constexpr (std::is_same_v<T, ezGameObjectHandle> ||
                    std::is_same_v<T, ezComponentHandle> ||
                    std::is_same_v<T, ezTypedPointer>)
      {
        EZ_ASSERT_NOT_IMPLEMENTED;
      }
      else
      {
        if (pProperty->GetSpecificType() == ezGetStaticRTTI<T>())
        {
          const T& value = inout_context.GetData<T>(node.GetInputDataOffset(1));
          pMemberProperty->SetValuePtr(pInstance.m_pObject, &value);
        }
        else
        {
          ezVariant value = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), pProperty->GetSpecificType());
          ezReflectionUtils::SetMemberPropertyValue(pMemberProperty, pInstance.m_pObject, value);
        }
      }
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_SetReflectedProperty);

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

      pCoroutine->StartWithVarargs(args);

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

  static ExecResult NodeFunction_SendMessage(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_TypeAndProperties>();
    auto targetObjectDataOffset = node.GetInputDataOffset(0);
    auto targetComponentDataOffset = node.GetInputDataOffset(1);

    auto pTargetObject = targetObjectDataOffset.IsValid() ? static_cast<ezGameObject*>(inout_context.GetPointerData(targetObjectDataOffset).m_pObject) : nullptr;
    auto pTargetComponent = targetComponentDataOffset.IsValid() ? static_cast<ezComponent*>(inout_context.GetPointerData(targetComponentDataOffset).m_pObject) : nullptr;
    if (pTargetObject == nullptr && pTargetComponent == nullptr)
    {
      ezLog::Error("Visual script send '{}': Invalid target game object and component.", userData.m_pType->GetTypeName());
      return ExecResult::Error();
    }

    auto mode = static_cast<ezVisualScriptSendMessageMode::Enum>(inout_context.GetData<ezInt64>(node.GetInputDataOffset(2)));
    ezTime delay = inout_context.GetData<ezTime>(node.GetInputDataOffset(3));

    ezScriptComponent* pSenderComponent = nullptr;
    if (mode == ezVisualScriptSendMessageMode::Event)
    {
      pSenderComponent = ezDynamicCast<ezScriptComponent*>(&inout_context.GetInstance().GetOwner());
    }

    const ezUInt32 uiStartSlot = 4;

    ezUniquePtr<ezMessage> pMessage = userData.m_pType->GetAllocator()->Allocate<ezMessage>(ezFrameAllocator::GetCurrentAllocator());
    for (ezUInt32 i = 0; i < userData.m_uiNumProperties; ++i)
    {
      auto pProp = userData.m_Properties[i];
      const ezRTTI* pPropType = pProp->GetSpecificType();
      ezVariant value = inout_context.GetDataAsVariant(node.GetInputDataOffset(uiStartSlot + i), pPropType);

      if (pProp->GetCategory() == ezPropertyCategory::Member)
      {
        ezReflectionUtils::SetMemberPropertyValue(static_cast<const ezAbstractMemberProperty*>(pProp), pMessage.Borrow(), value);
      }
      else
      {
        EZ_ASSERT_NOT_IMPLEMENTED;
      }
    }

    bool bWriteOutputs = false;
    if (pTargetComponent != nullptr)
    {
      if (delay.IsPositive())
      {
        pTargetComponent->PostMessage(*pMessage, delay);
      }
      else
      {
        bWriteOutputs = pTargetComponent->SendMessage(*pMessage);
      }
    }
    else if (pTargetObject != nullptr)
    {
      if (delay.IsPositive())
      {
        if (mode == ezVisualScriptSendMessageMode::Direct)
          pTargetObject->PostMessage(*pMessage, delay);
        else if (mode == ezVisualScriptSendMessageMode::Recursive)
          pTargetObject->PostMessageRecursive(*pMessage, delay);
        else
          pTargetObject->PostEventMessage(*pMessage, pSenderComponent, delay);
      }
      else
      {
        if (mode == ezVisualScriptSendMessageMode::Direct)
          bWriteOutputs = pTargetObject->SendMessage(*pMessage);
        else if (mode == ezVisualScriptSendMessageMode::Recursive)
          bWriteOutputs = pTargetObject->SendMessageRecursive(*pMessage);
        else
          bWriteOutputs = pTargetObject->SendEventMessage(*pMessage, pSenderComponent);
      }
    }

    if (bWriteOutputs)
    {
      for (ezUInt32 i = 0; i < userData.m_uiNumProperties; ++i)
      {
        auto dataOffset = node.GetOutputDataOffset(i);
        if (dataOffset.IsValid() == false)
          continue;

        auto pProp = userData.m_Properties[i];
        ezVariant value;

        if (pProp->GetCategory() == ezPropertyCategory::Member)
        {
          value = ezReflectionUtils::GetMemberPropertyValue(static_cast<const ezAbstractMemberProperty*>(pProp), pMessage.Borrow());
        }
        else
        {
          EZ_ASSERT_NOT_IMPLEMENTED;
        }

        inout_context.SetDataFromVariant(dataOffset, value);
      }
    }

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static ExecResult NodeFunction_Builtin_SetVariable(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, ezGameObjectHandle> ||
                  std::is_same_v<T, ezComponentHandle> ||
                  std::is_same_v<T, ezTypedPointer>)
    {
      ezTypedPointer ptr = inout_context.GetPointerData(node.GetInputDataOffset(0));
      inout_context.SetPointerData(node.GetOutputDataOffset(0), ptr.m_pObject, ptr.m_pType);
    }
    else
    {
      inout_context.SetData(node.GetOutputDataOffset(0), inout_context.GetData<T>(node.GetInputDataOffset(0)));
    }
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_SetVariable);

  template <typename T>
  static ExecResult NodeFunction_Builtin_IncVariable(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, ezUInt8> ||
                  std::is_same_v<T, ezInt32> ||
                  std::is_same_v<T, ezInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double>)
    {
      T a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      inout_context.SetData(node.GetOutputDataOffset(0), ++a);
    }
    else
    {
      ezLog::Error("Increment is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_IncVariable);

  template <typename T>
  static ExecResult NodeFunction_Builtin_DecVariable(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, ezUInt8> ||
                  std::is_same_v<T, ezInt32> ||
                  std::is_same_v<T, ezInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double>)
    {
      T a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      inout_context.SetData(node.GetOutputDataOffset(0), --a);
    }
    else
    {
      ezLog::Error("Decrement is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_DecVariable);

  static ExecResult NodeFunction_Builtin_Branch(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    bool bCondition = inout_context.GetData<bool>(node.GetInputDataOffset(0));
    return ExecResult::RunNext(bCondition ? 0 : 1);
  }

  template <typename T>
  static ExecResult NodeFunction_Builtin_Switch(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    ezInt64 iValue = 0;
    if constexpr (std::is_same_v<T, ezInt64>)
    {
      iValue = inout_context.GetData<ezInt64>(node.GetInputDataOffset(0));
    }
    else if constexpr (std::is_same_v<T, ezHashedString>)
    {
      iValue = inout_context.GetData<ezHashedString>(node.GetInputDataOffset(0)).GetHash();
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }

    auto& userData = node.GetUserData<NodeUserData_Switch>();
    for (ezUInt32 i = 0; i < userData.m_uiNumCases; ++i)
    {
      if (iValue == userData.m_Cases[i])
      {
        return ExecResult::RunNext(i);
      }
    }

    return ExecResult::RunNext(userData.m_uiNumCases);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Switch);

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

    if constexpr (std::is_same_v<T, bool> ||
                  std::is_same_v<T, ezUInt8> ||
                  std::is_same_v<T, ezInt32> ||
                  std::is_same_v<T, ezInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, ezColor> ||
                  std::is_same_v<T, ezVec3> ||
                  std::is_same_v<T, ezTime> ||
                  std::is_same_v<T, ezAngle> ||
                  std::is_same_v<T, ezString> ||
                  std::is_same_v<T, ezHashedString>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      bRes = ezComparisonOperator::Compare(userData.m_ComparisonOperator, a, b);
    }
    else if constexpr (std::is_same_v<T, ezGameObjectHandle> ||
                       std::is_same_v<T, ezComponentHandle> ||
                       std::is_same_v<T, ezTypedPointer>)
    {
      ezTypedPointer a = inout_context.GetPointerData(node.GetInputDataOffset(0));
      ezTypedPointer b = inout_context.GetPointerData(node.GetInputDataOffset(1));
      bRes = ezComparisonOperator::Compare(userData.m_ComparisonOperator, a.m_pObject, b.m_pObject);
    }
    else if constexpr (std::is_same_v<T, ezVariant>)
    {
      ezVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      ezVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);

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
    else if constexpr (std::is_same_v<T, ezQuat> ||
                       std::is_same_v<T, ezTransform> ||
                       std::is_same_v<T, ezVariantArray> ||
                       std::is_same_v<T, ezVariantDictionary>)
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
    if constexpr (std::is_same_v<T, float>)
    {
      bIsValid = ezMath::IsFinite(inout_context.GetData<float>(dataOffset));
    }
    else if constexpr (std::is_same_v<T, double>)
    {
      bIsValid = ezMath::IsFinite(inout_context.GetData<double>(dataOffset));
    }
    else if constexpr (std::is_same_v<T, ezColor>)
    {
      bIsValid = inout_context.GetData<ezColor>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same_v<T, ezVec3>)
    {
      bIsValid = inout_context.GetData<ezVec3>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same_v<T, ezQuat>)
    {
      bIsValid = inout_context.GetData<ezQuat>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same_v<T, ezString>)
    {
      bIsValid = inout_context.GetData<ezString>(dataOffset).IsEmpty() == false;
    }
    else if constexpr (std::is_same_v<T, ezHashedString>)
    {
      bIsValid = inout_context.GetData<ezHashedString>(dataOffset).IsEmpty() == false;
    }
    else if constexpr (std::is_same_v<T, ezGameObjectHandle> ||
                       std::is_same_v<T, ezComponentHandle> ||
                       std::is_same_v<T, ezTypedPointer>)
    {
      bIsValid = inout_context.GetPointerData(dataOffset).m_pObject != nullptr;
    }
    else if constexpr (std::is_same_v<T, ezVariant>)
    {
      bIsValid = inout_context.GetData<ezVariant>(dataOffset).IsValid();
    }

    inout_context.SetData(node.GetOutputDataOffset(0), bIsValid);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_IsValid);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Select(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    bool bCondition = inout_context.GetData<bool>(node.GetInputDataOffset(0));

    if constexpr (std::is_same_v<T, ezTypedPointer>)
    {
      ezTypedPointer a = inout_context.GetPointerData(node.GetInputDataOffset(1));
      ezTypedPointer b = inout_context.GetPointerData(node.GetInputDataOffset(2));
      ezTypedPointer res = bCondition ? a : b;
      inout_context.SetPointerData(node.GetOutputDataOffset(0), res.m_pObject, res.m_pType);
    }
    else
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(1));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(2));
      inout_context.SetData(node.GetOutputDataOffset(0), bCondition ? a : b);
    }
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Select);

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static ExecResult NodeFunction_Builtin_Add(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, ezUInt8> ||
                  std::is_same_v<T, ezInt32> ||
                  std::is_same_v<T, ezInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, ezColor> ||
                  std::is_same_v<T, ezVec3> ||
                  std::is_same_v<T, ezTime> ||
                  std::is_same_v<T, ezAngle>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a + b));
    }
    else if constexpr (std::is_same_v<T, ezString>)
    {
      auto& a = inout_context.GetData<ezString>(node.GetInputDataOffset(0));
      auto& b = inout_context.GetData<ezString>(node.GetInputDataOffset(1));

      ezStringBuilder s;
      s.Set(a, b);

      inout_context.SetData(node.GetOutputDataOffset(0), ezString(s.GetView()));
    }
    else if constexpr (std::is_same_v<T, ezHashedString>)
    {
      auto& a = inout_context.GetData<ezHashedString>(node.GetInputDataOffset(0));
      auto& b = inout_context.GetData<ezHashedString>(node.GetInputDataOffset(1));

      ezStringBuilder s;
      s.Set(a, b);
      ezHashedString sHashed;
      sHashed.Assign(s);

      inout_context.SetData(node.GetOutputDataOffset(0), sHashed);
    }
    else if constexpr (std::is_same_v<T, ezVariant>)
    {
      ezVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      ezVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
      inout_context.SetData(node.GetOutputDataOffset(0), a + b);
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
    if constexpr (std::is_same_v<T, ezUInt8> ||
                  std::is_same_v<T, ezInt32> ||
                  std::is_same_v<T, ezInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, ezColor> ||
                  std::is_same_v<T, ezVec3> ||
                  std::is_same_v<T, ezTime> ||
                  std::is_same_v<T, ezAngle>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a - b));
    }
    else if constexpr (std::is_same_v<T, ezVariant>)
    {
      ezVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      ezVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
      inout_context.SetData(node.GetOutputDataOffset(0), a - b);
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
    if constexpr (std::is_same_v<T, ezUInt8> ||
                  std::is_same_v<T, ezInt32> ||
                  std::is_same_v<T, ezInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, ezColor> ||
                  std::is_same_v<T, ezTime>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a * b));
    }
    else if constexpr (std::is_same_v<T, ezVec3>)
    {
      const ezVec3& a = inout_context.GetData<ezVec3>(node.GetInputDataOffset(0));
      const ezVec3& b = inout_context.GetData<ezVec3>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), a.CompMul(b));
    }
    else if constexpr (std::is_same_v<T, ezAngle>)
    {
      const ezAngle& a = inout_context.GetData<ezAngle>(node.GetInputDataOffset(0));
      const ezAngle& b = inout_context.GetData<ezAngle>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), ezAngle(a * b.GetRadian()));
    }
    else if constexpr (std::is_same_v<T, ezVariant>)
    {
      ezVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      ezVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
      inout_context.SetData(node.GetOutputDataOffset(0), a * b);
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
    if constexpr (std::is_same_v<T, ezUInt8> ||
                  std::is_same_v<T, ezInt32> ||
                  std::is_same_v<T, ezInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, ezTime>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a / b));
    }
    else if constexpr (std::is_same_v<T, ezVec3>)
    {
      const ezVec3& a = inout_context.GetData<ezVec3>(node.GetInputDataOffset(0));
      const ezVec3& b = inout_context.GetData<ezVec3>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), a.CompDiv(b));
    }
    else if constexpr (std::is_same_v<T, ezAngle>)
    {
      const ezAngle& a = inout_context.GetData<ezAngle>(node.GetInputDataOffset(0));
      const ezAngle& b = inout_context.GetData<ezAngle>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), ezAngle(a / b.GetRadian()));
    }
    else if constexpr (std::is_same_v<T, ezVariant>)
    {
      ezVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      ezVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
      inout_context.SetData(node.GetOutputDataOffset(0), a / b);
    }
    else
    {
      ezLog::Error("Divide is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Div);

  static ExecResult NodeFunction_Builtin_Expression(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    static ezHashedString sStream = ezMakeHashedString("VsStream");

    int iDummy = 0;
    ezHybridArray<ezProcessingStream, 8> inputStreams;
    for (ezUInt32 i = 0; i < node.m_NumInputDataOffsets; ++i)
    {
      auto dataOffset = node.GetInputDataOffset(i);

      ezTypedPointer ptr;
      if (dataOffset.IsConstant())
      {
        ptr.m_pObject = &iDummy;
      }
      else
      {
        ptr = inout_context.GetPointerData(dataOffset);
      }

      const ezUInt32 uiDataSize = ezVisualScriptDataType::GetStorageSize(dataOffset.GetType());
      auto streamDataType = ezVisualScriptDataType::GetStreamDataType(dataOffset.GetType());

      inputStreams.PushBack(ezProcessingStream(sStream, ezMakeArrayPtr(static_cast<ezUInt8*>(ptr.m_pObject), uiDataSize), streamDataType));
    }

    ezHybridArray<ezProcessingStream, 8> outputStreams;
    for (ezUInt32 i = 0; i < node.m_NumOutputDataOffsets; ++i)
    {
      auto dataOffset = node.GetOutputDataOffset(i);
      ezTypedPointer ptr = inout_context.GetPointerData(dataOffset);

      const ezUInt32 uiDataSize = ezVisualScriptDataType::GetStorageSize(dataOffset.GetType());
      auto streamDataType = ezVisualScriptDataType::GetStreamDataType(dataOffset.GetType());

      outputStreams.PushBack(ezProcessingStream(sStream, ezMakeArrayPtr(static_cast<ezUInt8*>(ptr.m_pObject), uiDataSize), streamDataType));
    }

    auto& userData = node.GetUserData<NodeUserData_Expression>();
    if (pModule->GetSharedExpressionVM().Execute(userData.m_ByteCode, inputStreams, outputStreams, 1, ezExpression::GlobalData(), ezExpressionVM::Flags::ScalarizeStreams).Failed())
    {
      ezLog::Error("Visual script expression execution failed");
      return ExecResult::Error();
    }

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static ExecResult NodeFunction_Builtin_ToBool(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    bool bRes = false;
    if constexpr (std::is_same_v<T, bool>)
    {
      bRes = inout_context.GetData<T>(dataOffset);
    }
    else if constexpr (std::is_same_v<T, ezUInt8> ||
                       std::is_same_v<T, ezInt32> ||
                       std::is_same_v<T, ezInt64> ||
                       std::is_same_v<T, float> ||
                       std::is_same_v<T, double>)
    {
      bRes = inout_context.GetData<T>(dataOffset) != 0;
    }
    else if constexpr (std::is_same_v<T, ezGameObjectHandle> ||
                       std::is_same_v<T, ezComponentHandle> ||
                       std::is_same_v<T, ezTypedPointer>)
    {
      bRes = inout_context.GetPointerData(dataOffset).m_pObject != nullptr;
    }
    else if constexpr (std::is_same_v<T, ezVariant>)
    {
      bRes = inout_context.GetData<ezVariant>(dataOffset).ConvertTo<bool>();
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
    if constexpr (std::is_same_v<T, bool>)
    {
      res = inout_context.GetData<T>(dataOffset) ? NumberType(1) : NumberType(0);
    }
    else if constexpr (std::is_same_v<T, ezUInt8> ||
                       std::is_same_v<T, ezInt32> ||
                       std::is_same_v<T, ezInt64> ||
                       std::is_same_v<T, float> ||
                       std::is_same_v<T, double>)
    {
      res = static_cast<NumberType>(inout_context.GetData<T>(dataOffset));
    }
    else if constexpr (std::is_same_v<T, ezVariant>)
    {
      res = inout_context.GetData<ezVariant>(dataOffset).ConvertTo<NumberType>();
    }
    else
    {
      ezLog::Error("To{} is not defined for type '{}'", szName, GetTypeName<T>());
    }

    inout_context.SetData(node.GetOutputDataOffset(0), res);
    return ExecResult::RunNext(0);
  }

#define MAKE_TONUMBER_EXEC_FUNC(NumberType, Name)                                                                                                                 \
  template <typename T>                                                                                                                                           \
  static ExecResult EZ_PP_CONCAT(NodeFunction_Builtin_To, Name)(ezVisualScriptExecutionContext & inout_context, const ezVisualScriptGraphDescription::Node& node) \
  {                                                                                                                                                               \
    return NodeFunction_Builtin_ToNumber<NumberType, T>(inout_context, node, #Name);                                                                              \
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
    ezStringBuilder sb;
    ezStringView s;
    if constexpr (std::is_same_v<T, ezGameObjectHandle> ||
                  std::is_same_v<T, ezComponentHandle> ||
                  std::is_same_v<T, ezTypedPointer>)
    {
      ezTypedPointer p = inout_context.GetPointerData(node.GetInputDataOffset(0));
      sb.SetFormat("{} {}", p.m_pType->GetTypeName(), ezArgP(p.m_pObject));
      s = sb;
    }
    else if constexpr (std::is_same_v<T, ezString>)
    {
      s = inout_context.GetData<ezString>(node.GetInputDataOffset(0));
    }
    else
    {
      s = ezConversionUtils::ToString(inout_context.GetData<T>(node.GetInputDataOffset(0)), sb);
    }

    inout_context.SetData(node.GetOutputDataOffset(0), s);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToString);

  static ExecResult NodeFunction_Builtin_String_Format(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    auto& sText = inout_context.GetData<ezString>(node.GetInputDataOffset(0));

    ezHybridArray<ezString, 12> stringStorage;
    stringStorage.Reserve(node.m_NumInputDataOffsets - 1);
    for (ezUInt32 i = 1; i < node.m_NumInputDataOffsets; ++i)
    {
      stringStorage.PushBack(inout_context.GetDataAsVariant(node.GetInputDataOffset(i), nullptr).ConvertTo<ezString>());
    }

    ezHybridArray<ezStringView, 12> stringViews;
    stringViews.Reserve(stringStorage.GetCount());
    for (auto& s : stringStorage)
    {
      stringViews.PushBack(s);
    }

    ezFormatString fs(sText.GetView());
    ezStringBuilder sStorage;
    ezStringView sFormatted = fs.BuildFormattedText(sStorage, stringViews.GetData(), stringViews.GetCount());

    inout_context.SetData(node.GetOutputDataOffset(0), sFormatted);
    return ExecResult::RunNext(0);
  }

  template <typename T>
  static ExecResult NodeFunction_Builtin_ToHashedString(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    ezStringBuilder sb;
    ezStringView s;
    if constexpr (std::is_same_v<T, ezGameObjectHandle> ||
                  std::is_same_v<T, ezComponentHandle> ||
                  std::is_same_v<T, ezTypedPointer>)
    {
      ezTypedPointer p = inout_context.GetPointerData(dataOffset);
      sb.SetFormat("{} {}", p.m_pType->GetTypeName(), ezArgP(p.m_pObject));
      s = sb;
    }
    else if constexpr (std::is_same_v<T, ezString>)
    {
      s = inout_context.GetData<ezString>(dataOffset);
    }
    else if constexpr (std::is_same_v<T, ezHashedString>)
    {
      inout_context.SetData(node.GetOutputDataOffset(0), inout_context.GetData<ezHashedString>(dataOffset));
      return ExecResult::RunNext(0);
    }
    else if constexpr (std::is_same_v<T, ezVariant>)
    {
      inout_context.SetData(node.GetOutputDataOffset(0), inout_context.GetData<ezVariant>(dataOffset).ConvertTo<ezHashedString>());
      return ExecResult::RunNext(0);
    }
    else
    {
      s = ezConversionUtils::ToString(inout_context.GetData<T>(dataOffset), sb);
    }

    ezHashedString sHashed;
    sHashed.Assign(s);
    inout_context.SetData(node.GetOutputDataOffset(0), sHashed);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToHashedString);

  template <typename T>
  static ExecResult NodeFunction_Builtin_ToVariant(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    ezVariant v;
    if constexpr (std::is_same_v<T, ezTypedPointer>)
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
    if constexpr (std::is_same_v<T, ezTypedPointer>)
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
    else if constexpr (std::is_same_v<T, ezVariant>)
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

  static ExecResult NodeFunction_Builtin_Array_GetElement(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    const ezVariantArray& a = inout_context.GetData<ezVariantArray>(node.GetInputDataOffset(0));
    int iIndex = inout_context.GetData<int>(node.GetInputDataOffset(1));
    if (iIndex >= 0 && iIndex < int(a.GetCount()))
    {
      inout_context.SetData(node.GetOutputDataOffset(0), a[iIndex]);
    }
    else
    {
      inout_context.SetData(node.GetOutputDataOffset(0), ezVariant());
    }

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_SetElement(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    ezVariantArray& a = inout_context.GetWritableData<ezVariantArray>(node.GetInputDataOffset(0));
    int iIndex = inout_context.GetData<int>(node.GetInputDataOffset(1));
    if (iIndex >= 0 && iIndex < int(a.GetCount()))
    {
      a[iIndex] = inout_context.GetDataAsVariant(node.GetInputDataOffset(2), nullptr);
      return ExecResult::RunNext(0);
    }

    ezLog::Error("Visual script Array::SetElement: Index '{}' is out of bounds. Valid range is [0, {}).", iIndex, a.GetCount());
    return ExecResult::Error();
  }

  static ExecResult NodeFunction_Builtin_Array_GetCount(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    const ezVariantArray& a = inout_context.GetData<ezVariantArray>(node.GetInputDataOffset(0));
    inout_context.SetData<int>(node.GetOutputDataOffset(0), a.GetCount());

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_Clear(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    ezVariantArray& a = inout_context.GetWritableData<ezVariantArray>(node.GetInputDataOffset(0));
    a.Clear();

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_IsEmpty(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    const ezVariantArray& a = inout_context.GetData<ezVariantArray>(node.GetInputDataOffset(0));
    inout_context.SetData<bool>(node.GetOutputDataOffset(0), a.IsEmpty());

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_Contains(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    const ezVariantArray& a = inout_context.GetData<ezVariantArray>(node.GetInputDataOffset(0));
    const ezVariant& element = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
    inout_context.SetData<bool>(node.GetOutputDataOffset(0), a.Contains(element));

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_IndexOf(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    const ezVariantArray& a = inout_context.GetData<ezVariantArray>(node.GetInputDataOffset(0));
    const ezVariant& element = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
    ezUInt32 uiStartIndex = inout_context.GetData<int>(node.GetInputDataOffset(2));

    ezUInt32 uiIndex = a.IndexOf(element, uiStartIndex);
    inout_context.SetData<int>(node.GetOutputDataOffset(0), uiIndex == ezInvalidIndex ? -1 : int(uiIndex));

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_Insert(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    ezVariantArray& a = inout_context.GetWritableData<ezVariantArray>(node.GetInputDataOffset(0));
    const ezVariant& element = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
    int iIndex = inout_context.GetData<int>(node.GetInputDataOffset(2));
    if (iIndex >= 0 && iIndex <= int(a.GetCount()))
    {
      a.InsertAt(iIndex, element);
      return ExecResult::RunNext(0);
    }

    ezLog::Error("Visual script Array::Insert: Index '{}' is out of bounds. Valid range is [0, {}].", iIndex, a.GetCount());
    return ExecResult::Error();
  }

  static ExecResult NodeFunction_Builtin_Array_PushBack(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    ezVariantArray& a = inout_context.GetWritableData<ezVariantArray>(node.GetInputDataOffset(0));
    const ezVariant& element = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
    a.PushBack(element);

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_Remove(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    ezVariantArray& a = inout_context.GetWritableData<ezVariantArray>(node.GetInputDataOffset(0));
    const ezVariant& element = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
    a.RemoveAndCopy(element);

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_RemoveAt(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    ezVariantArray& a = inout_context.GetWritableData<ezVariantArray>(node.GetInputDataOffset(0));
    int iIndex = inout_context.GetData<int>(node.GetInputDataOffset(1));
    if (iIndex >= 0 && iIndex < int(a.GetCount()))
    {
      a.RemoveAtAndCopy(iIndex);
      return ExecResult::RunNext(0);
    }

    ezLog::Error("Visual script Array::RemoveAt: Index '{}' is out of bounds. Valid range is [0, {}).", iIndex, a.GetCount());
    return ExecResult::Error();
  }

  //////////////////////////////////////////////////////////////////////////

  static ExecResult NodeFunction_Builtin_TryGetComponentOfBaseType(ezVisualScriptExecutionContext& inout_context, const ezVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_Type>();

    ezTypedPointer p = inout_context.GetPointerData(node.GetInputDataOffset(0));
    if (p.m_pType != ezGetStaticRTTI<ezGameObject>())
    {
      ezLog::Error("Visual script call TryGetComponentOfBaseType: Game object is not of type 'ezGameObject'");
      return ExecResult::Error();
    }

    if (p.m_pObject == nullptr)
    {
      ezLog::Error("Visual script call TryGetComponentOfBaseType: Game object is null");
      return ExecResult::Error();
    }

    ezComponent* pComponent = nullptr;
    if (static_cast<ezGameObject*>(p.m_pObject)->TryGetComponentOfBaseType(userData.m_pType, pComponent))
    {
      inout_context.SetPointerData(node.GetOutputDataOffset(0), pComponent);
    }

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
    {},                                                        // Invalid,
    {},                                                        // EntryCall,
    {},                                                        // EntryCall_Coroutine,
    {},                                                        // MessageHandler,
    {},                                                        // MessageHandler_Coroutine,
    {&NodeFunction_ReflectedFunction},                         // ReflectedFunction,
    {nullptr, &NodeFunction_GetReflectedProperty_Getter},      // GetReflectedProperty,
    {nullptr, &NodeFunction_SetReflectedProperty_Getter},      // SetReflectedProperty,
    {&NodeFunction_InplaceCoroutine},                          // InplaceCoroutine,
    {&NodeFunction_GetScriptOwner},                            // GetScriptOwner,
    {&NodeFunction_SendMessage},                               // SendMessage,

    {},                                                        // FirstBuiltin,

    {},                                                        // Builtin_Constant,
    {},                                                        // Builtin_GetVariable,
    {nullptr, &NodeFunction_Builtin_SetVariable_Getter},       // Builtin_SetVariable,
    {nullptr, &NodeFunction_Builtin_IncVariable_Getter},       // Builtin_IncVariable,
    {nullptr, &NodeFunction_Builtin_DecVariable_Getter},       // Builtin_DecVariable,
    {nullptr, &NodeFunction_Builtin_SetVariable_Getter},       // Builtin_TempVariable,

    {&NodeFunction_Builtin_Branch},                            // Builtin_Branch,
    {nullptr, &NodeFunction_Builtin_Switch_Getter},            // Builtin_Switch,
    {},                                                        // Builtin_WhileLoop,
    {},                                                        // Builtin_ForLoop,
    {},                                                        // Builtin_ForEachLoop,
    {},                                                        // Builtin_ReverseForEachLoop,
    {},                                                        // Builtin_Break,
    {},                                                        // Builtin_Jump,

    {&NodeFunction_Builtin_And},                               // Builtin_And,
    {&NodeFunction_Builtin_Or},                                // Builtin_Or,
    {&NodeFunction_Builtin_Not},                               // Builtin_Not,
    {nullptr, &NodeFunction_Builtin_Compare_Getter},           // Builtin_Compare,
    {},                                                        // Builtin_CompareExec,
    {nullptr, &NodeFunction_Builtin_IsValid_Getter},           // Builtin_IsValid,
    {nullptr, &NodeFunction_Builtin_Select_Getter},            // Builtin_Select,

    {nullptr, &NodeFunction_Builtin_Add_Getter},               // Builtin_Add,
    {nullptr, &NodeFunction_Builtin_Sub_Getter},               // Builtin_Subtract,
    {nullptr, &NodeFunction_Builtin_Mul_Getter},               // Builtin_Multiply,
    {nullptr, &NodeFunction_Builtin_Div_Getter},               // Builtin_Divide,
    {&NodeFunction_Builtin_Expression},                        // Builtin_Expression,

    {nullptr, &NodeFunction_Builtin_ToBool_Getter},            // Builtin_ToBool,
    {nullptr, &NodeFunction_Builtin_ToByte_Getter},            // Builtin_ToByte,
    {nullptr, &NodeFunction_Builtin_ToInt_Getter},             // Builtin_ToInt,
    {nullptr, &NodeFunction_Builtin_ToInt64_Getter},           // Builtin_ToInt64,
    {nullptr, &NodeFunction_Builtin_ToFloat_Getter},           // Builtin_ToFloat,
    {nullptr, &NodeFunction_Builtin_ToDouble_Getter},          // Builtin_ToDouble,
    {nullptr, &NodeFunction_Builtin_ToString_Getter},          // Builtin_ToString,
    {&NodeFunction_Builtin_String_Format},                     // Builtin_String_Format,
    {nullptr, &NodeFunction_Builtin_ToHashedString_Getter},    // Builtin_ToHashedString,
    {nullptr, &NodeFunction_Builtin_ToVariant_Getter},         // Builtin_ToVariant,
    {nullptr, &NodeFunction_Builtin_Variant_ConvertTo_Getter}, // Builtin_Variant_ConvertTo,

    {&NodeFunction_Builtin_MakeArray},                         // Builtin_MakeArray
    {&NodeFunction_Builtin_Array_GetElement},                  // Builtin_Array_GetElement,
    {&NodeFunction_Builtin_Array_SetElement},                  // Builtin_Array_SetElement,
    {&NodeFunction_Builtin_Array_GetCount},                    // Builtin_Array_GetCount,
    {&NodeFunction_Builtin_Array_IsEmpty},                     // Builtin_Array_IsEmpty,
    {&NodeFunction_Builtin_Array_Clear},                       // Builtin_Array_Clear,
    {&NodeFunction_Builtin_Array_Contains},                    // Builtin_Array_Contains,
    {&NodeFunction_Builtin_Array_IndexOf},                     // Builtin_Array_IndexOf,
    {&NodeFunction_Builtin_Array_Insert},                      // Builtin_Array_Insert,
    {&NodeFunction_Builtin_Array_PushBack},                    // Builtin_Array_PushBack,
    {&NodeFunction_Builtin_Array_Remove},                      // Builtin_Array_Remove,
    {&NodeFunction_Builtin_Array_RemoveAt},                    // Builtin_Array_RemoveAt,

    {&NodeFunction_Builtin_TryGetComponentOfBaseType},         // Builtin_TryGetComponentOfBaseType

    {&NodeFunction_Builtin_StartCoroutine},                    // Builtin_StartCoroutine,
    {&NodeFunction_Builtin_StopCoroutine},                     // Builtin_StopCoroutine,
    {&NodeFunction_Builtin_StopAllCoroutines},                 // Builtin_StopAllCoroutines,
    {&NodeFunction_Builtin_WaitForX<true>},                    // Builtin_WaitForAll,
    {&NodeFunction_Builtin_WaitForX<false>},                   // Builtin_WaitForAny,
    {&NodeFunction_Builtin_Yield},                             // Builtin_Yield,

    {},                                                        // LastBuiltin,
  };

  static_assert(EZ_ARRAY_SIZE(s_TypeToExecuteFunctions) == ezVisualScriptNodeDescription::Type::Count);
} // namespace

ezVisualScriptGraphDescription::ExecuteFunction GetExecuteFunction(ezVisualScriptNodeDescription::Type::Enum nodeType, ezVisualScriptDataType::Enum dataType)
{
  EZ_ASSERT_DEBUG(nodeType >= 0 && static_cast<ezUInt32>(nodeType) < EZ_ARRAY_SIZE(s_TypeToExecuteFunctions), "Out of bounds access");
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
