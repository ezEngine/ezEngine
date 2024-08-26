#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

ezHashTable<ezUInt32, ezTypeScriptBinding::FunctionBinding> ezTypeScriptBinding::s_BoundFunctions;

static int __CPP_ComponentFunction_Call(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_FunctionBinding()
{
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_ComponentFunction_Call", __CPP_ComponentFunction_Call);

  return EZ_SUCCESS;
}

ezUInt32 ezTypeScriptBinding::ComputeFunctionBindingHash(const ezRTTI* pType, const ezAbstractFunctionProperty* pFunc)
{
  ezStringBuilder sFuncName;

  sFuncName.Set(pType->GetTypeName(), "::", pFunc->GetPropertyName());

  return ezHashingUtils::StringHashTo32(ezHashingUtils::StringHash(sFuncName.GetData()));
}

void ezTypeScriptBinding::SetupRttiFunctionBindings()
{
  if (!s_BoundFunctions.IsEmpty())
    return;

  ezRTTI::ForEachDerivedType<ezComponent>(
    [&](const ezRTTI* pRtti)
    {
      for (const ezAbstractFunctionProperty* pFunc : pRtti->GetFunctions())
      {
        // TODO: static members ?
        if (pFunc->GetFunctionType() != ezFunctionType::Member)
          continue;

        const ezUInt32 uiHash = ComputeFunctionBindingHash(pRtti, pFunc);
        if (auto pExistingBinding = s_BoundFunctions.GetValue(uiHash))
        {
          EZ_ASSERT_DEV(ezStringUtils::IsEqual(pExistingBinding->m_pFunc->GetPropertyName(), pFunc->GetPropertyName()), "Hash collision for bound function name!");
        }

        s_BoundFunctions[uiHash].m_pFunc = pFunc;
      }
    });
}

const char* ezTypeScriptBinding::TsType(const ezRTTI* pRtti)
{
  if (pRtti == nullptr)
    return "void";

  static ezStringBuilder res;

  if (pRtti->IsDerivedFrom<ezEnumBase>())
  {
    s_RequiredEnums.Insert(pRtti);

    res = pRtti->GetTypeName();
    res.TrimWordStart("ez");
    res.Prepend("Enum.");

    return res;
  }

  if (pRtti->IsDerivedFrom<ezBitflagsBase>())
  {
    s_RequiredFlags.Insert(pRtti);

    res = pRtti->GetTypeName();
    res.TrimWordStart("ez");
    res.Prepend("Flags.");

    return res;
  }

  switch (pRtti->GetVariantType())
  {
    case ezVariant::Type::Invalid:
    {
      if (pRtti->GetTypeName() == "ezVariant")
        return "any";

      return nullptr;
    }

    case ezVariant::Type::Angle:
      return "number";

    case ezVariant::Type::Bool:
      return "boolean";

    case ezVariant::Type::Int8:
    case ezVariant::Type::UInt8:
    case ezVariant::Type::Int16:
    case ezVariant::Type::UInt16:
    case ezVariant::Type::Int32:
    case ezVariant::Type::UInt32:
    case ezVariant::Type::Int64:
    case ezVariant::Type::UInt64:
    case ezVariant::Type::Float:
    case ezVariant::Type::Double:
      return "number";

    case ezVariant::Type::Color:
    case ezVariant::Type::ColorGamma:
      return "Color";

    case ezVariant::Type::Vector2:
    case ezVariant::Type::Vector2I:
    case ezVariant::Type::Vector2U:
      return "Vec2";

    case ezVariant::Type::Vector3:
    case ezVariant::Type::Vector3I:
    case ezVariant::Type::Vector3U:
      return "Vec3";

    case ezVariant::Type::Quaternion:
      return "Quat";


    case ezVariant::Type::String:
    case ezVariant::Type::StringView:
    case ezVariant::Type::HashedString:
      return "string";

    case ezVariant::Type::Time:
      return "number";

    case ezVariant::Type::Transform:
      return "Transform";

    case ezVariant::Type::Matrix3:
      return "Mat3";

    case ezVariant::Type::Matrix4:
      return "Mat4";

      // TODO: implement these types
      // case ezVariant::Type::Vector4:
      // case ezVariant::Type::Vector4I:
      // case ezVariant::Type::Vector4U:

    default:
      return nullptr;
  }
}

void ezTypeScriptBinding::GenerateExposedFunctionsCode(ezStringBuilder& out_Code, const ezRTTI* pRtti)
{
  ezStringBuilder sFunc;

  for (const ezAbstractFunctionProperty* pFunc : pRtti->GetFunctions())
  {
    // TODO: static members ?
    if (pFunc->GetFunctionType() != ezFunctionType::Member)
      continue;

    // don't support functions with that many arguments
    if (pFunc->GetArgumentCount() > 16)
      continue;

    const ezScriptableFunctionAttribute* pAttr = pFunc->GetAttributeByType<ezScriptableFunctionAttribute>();

    if (pAttr == nullptr)
      goto ignore;

    sFunc.Set("  ", pFunc->GetPropertyName(), "(");

    for (ezUInt32 i = 0; i < pFunc->GetArgumentCount(); ++i)
    {
      const char* szType = TsType(pFunc->GetArgumentType(i));

      if (szType == nullptr)
        goto ignore;

      sFunc.Append(i > 0 ? ", " : "", pAttr->GetArgumentName(i), ": ", szType);
    }

    sFunc.Append("): ");

    {
      const bool bHasReturnValue = pFunc->GetReturnType() != nullptr;

      {
        const char* szType = TsType(pFunc->GetReturnType());

        if (szType == nullptr)
          goto ignore;

        sFunc.Append(szType);
      }

      // function body
      {
        ezUInt32 uiFuncHash = ComputeFunctionBindingHash(pRtti, pFunc);

        if (bHasReturnValue)
          sFunc.AppendFormat(" { return __CPP_ComponentFunction_Call(this, {0}", uiFuncHash);
        else
          sFunc.AppendFormat(" { __CPP_ComponentFunction_Call(this, {0}", uiFuncHash);

        for (ezUInt32 arg = 0; arg < pFunc->GetArgumentCount(); ++arg)
        {
          sFunc.Append(", ", pAttr->GetArgumentName(arg));
        }

        sFunc.Append("); }\n");
      }
    }

    out_Code.Append(sFunc.GetView());

  ignore:
    continue;
  }
}

const ezTypeScriptBinding::FunctionBinding* ezTypeScriptBinding::FindFunctionBinding(ezUInt32 uiFunctionHash)
{
  const FunctionBinding* pBinding = nullptr;
  s_BoundFunctions.TryGetValue(uiFunctionHash, pBinding);
  return pBinding;
}

int __CPP_ComponentFunction_Call(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk);

  const ezUInt32 uiFuncHash = duk.GetUIntValue(1);

  const ezTypeScriptBinding::FunctionBinding* pBinding = ezTypeScriptBinding::FindFunctionBinding(uiFuncHash);

  if (pBinding == nullptr)
  {
    ezLog::Error("Bound function with hash {} not found.", uiFuncHash);
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), +1);
  }

  const ezUInt32 uiNumArgs = pBinding->m_pFunc->GetArgumentCount();

  ezVariant ret0;
  ezStaticArray<ezVariant, 16> args;
  args.SetCount(uiNumArgs);

  for (ezUInt32 arg = 0; arg < uiNumArgs; ++arg)
  {
    args[arg] = ezTypeScriptBinding::GetVariant(duk, 2 + arg, pBinding->m_pFunc->GetArgumentType(arg));
  }

  pBinding->m_pFunc->Execute(pComponent, args, ret0);

  if (pBinding->m_pFunc->GetReturnType() != nullptr)
  {
    ezTypeScriptBinding::PushVariant(duk, ret0);
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
  }
  else
  {
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
  }
}
