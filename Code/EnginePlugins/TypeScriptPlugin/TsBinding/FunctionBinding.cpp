#include <TypeScriptPluginPCH.h>

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

ezHashTable<ezUInt32, ezTypeScriptBinding::FunctionBinding> ezTypeScriptBinding::s_BoundFunctions;

static int __CPP_ComponentFunction_Call(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_FunctionBinding()
{
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_ComponentFunction_Call", __CPP_ComponentFunction_Call);

  return EZ_SUCCESS;
}

ezUInt32 ezTypeScriptBinding::ComputeFunctionBindingHash(const ezRTTI* pType, ezAbstractFunctionProperty* pFunc)
{
  ezStringBuilder sFuncName;

  sFuncName.Set(pType->GetTypeName(), "::", pFunc->GetPropertyName());

  return ezTempHashedString::ComputeHash(sFuncName.GetData());
}

void ezTypeScriptBinding::SetupRttiFunctionBindings()
{
  if (!s_BoundFunctions.IsEmpty())
    return;

  for (const ezRTTI* pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->IsDerivedFrom<ezComponent>())
      continue;

    for (ezAbstractFunctionProperty* pFunc : pRtti->GetFunctions())
    {
      // TODO: static members ?
      if (pFunc->GetFunctionType() != ezFunctionType::Member)
        continue;

      const ezUInt32 uiHash = ComputeFunctionBindingHash(pRtti, pFunc);
      EZ_ASSERT_DEV(!s_BoundFunctions.Contains(uiHash), "Hash collision for bound function name!");

      s_BoundFunctions[uiHash].m_pFunc = pFunc;
    }
  }
}

const char* ezTypeScriptBinding::TsType(const ezRTTI* pRtti)
{
  if (pRtti == nullptr)
    return "void";

  switch (pRtti->GetVariantType())
  {
      //case ezVariant::Type::Angle:
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
      //case ezVariant::Type::ColorGamma:
      return "Color";

    //case ezVariant::Type::Vector2:
    case ezVariant::Type::Vector3:
      return "Vec3";

    //case ezVariant::Type::Vector4:
    //case ezVariant::Type::Vector2I:
    //case ezVariant::Type::Vector3I:
    //case ezVariant::Type::Vector4I:
    //case ezVariant::Type::Vector2U:
    //case ezVariant::Type::Vector3U:
    //case ezVariant::Type::Vector4U:
    case ezVariant::Type::Quaternion:
      return "Quat";

      //case ezVariant::Type::Matrix3:
      //case ezVariant::Type::Matrix4:
      //case ezVariant::Type::Transform:

    case ezVariant::Type::String:
    case ezVariant::Type::StringView:
      return "string";

      //case ezVariant::Type::Time:
      //case ezVariant::Type::Uuid:

    default:
      return nullptr;
  }
}

void ezTypeScriptBinding::GenerateExposedFunctionsCode(ezStringBuilder& out_Code, const ezRTTI* pRtti)
{
  ezStringBuilder sFunc;

  for (ezAbstractFunctionProperty* pFunc : pRtti->GetFunctions())
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
  ezDuktapeFunction duk(pDuk, +1);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk);

  const ezUInt32 uiFuncHash = duk.GetUIntValue(1);

  const ezTypeScriptBinding::FunctionBinding* pBinding = ezTypeScriptBinding::FindFunctionBinding(uiFuncHash);

  if (pBinding == nullptr)
  {
    ezLog::Error("Bound function with hash {} not found.", uiFuncHash);
    return duk.ReturnVoid();
  }

  const ezUInt32 uiNumArgs = pBinding->m_pFunc->GetArgumentCount();

  ezVariant ret;
  ezStaticArray<ezVariant, 16> args;
  args.SetCount(uiNumArgs);

  for (ezUInt32 arg = 0; arg < uiNumArgs; ++arg)
  {
    switch (pBinding->m_pFunc->GetArgumentType(arg)->GetVariantType())
    {
      case ezVariant::Type::Bool:
        args[arg] = duk.GetBoolValue(2 + arg);
        break;

      case ezVariant::Type::Int8:
      case ezVariant::Type::Int16:
      case ezVariant::Type::Int32:
      case ezVariant::Type::Int64:
        args[arg] = duk.GetIntValue(2 + arg);
        break;

      case ezVariant::Type::UInt8:
      case ezVariant::Type::UInt16:
      case ezVariant::Type::UInt32:
      case ezVariant::Type::UInt64:
        args[arg] = duk.GetUIntValue(2 + arg);
        break;

      case ezVariant::Type::Float:
      case ezVariant::Type::Double:
        args[arg] = duk.GetFloatValue(2 + arg);
        break;

      case ezVariant::Type::String:
      case ezVariant::Type::StringView:
        args[arg] = duk.GetStringValue(2 + arg);
        break;

      case ezVariant::Type::Vector3:
        args[arg] = ezTypeScriptBinding::GetVec3(duk, 2 + arg);
        break;

      case ezVariant::Type::Quaternion:
        args[arg] = ezTypeScriptBinding::GetQuat(duk, 2 + arg);
        break;

      case ezVariant::Type::Color:
        args[arg] = ezTypeScriptBinding::GetColor(duk, 2 + arg);
        break;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }

  pBinding->m_pFunc->Execute(pComponent, args, ret);

  if (pBinding->m_pFunc->GetReturnType() != nullptr)
  {
    switch (ret.GetType())
    {
      case ezVariant::Type::Bool:
        return duk.ReturnBool(ret.Get<bool>());

      case ezVariant::Type::Int8:
      case ezVariant::Type::Int16:
      case ezVariant::Type::Int32:
      case ezVariant::Type::Int64:
        return duk.ReturnInt(ret.ConvertTo<ezInt32>());

      case ezVariant::Type::UInt8:
      case ezVariant::Type::UInt16:
      case ezVariant::Type::UInt32:
      case ezVariant::Type::UInt64:
        return duk.ReturnUInt(ret.ConvertTo<ezUInt32>());

      case ezVariant::Type::Float:
      case ezVariant::Type::Double:
        return duk.ReturnNumber(ret.ConvertTo<double>());

      case ezVariant::Type::String:
      case ezVariant::Type::StringView:
        return duk.ReturnString(ret.ConvertTo<ezString>());

      case ezVariant::Type::Vector3:
        ezTypeScriptBinding::PushVec3(duk, ret.Get<ezVec3>());
        return duk.ReturnCustom();

      case ezVariant::Type::Quaternion:
        ezTypeScriptBinding::PushQuat(duk, ret.Get<ezQuat>());
        return duk.ReturnCustom();

      case ezVariant::Type::Color:
        ezTypeScriptBinding::PushColor(duk, ret.Get<ezColor>());
        return duk.ReturnCustom();

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }

  duk.SetExpectedStackChange(0);
  return duk.ReturnVoid();
}
