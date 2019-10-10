#include <TypeScriptPluginPCH.h>

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

ezHashTable<ezUInt32, ezTypeScriptBinding::FunctionBinding> ezTypeScriptBinding::s_BoundFunctions;

static int __CPP_ComponentFunction_Call0(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_FunctionBinding()
{
  m_Duk.RegisterFunction("__CPP_ComponentFunction_Call0", __CPP_ComponentFunction_Call0, 2);

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

      //case ezVariant::Type::Color:
      //case ezVariant::Type::Vector2:
      //case ezVariant::Type::Vector3:
      //case ezVariant::Type::Vector4:
      //case ezVariant::Type::Vector2I:
      //case ezVariant::Type::Vector3I:
      //case ezVariant::Type::Vector4I:
      //case ezVariant::Type::Vector2U:
      //case ezVariant::Type::Vector3U:
      //case ezVariant::Type::Vector4U:
      //case ezVariant::Type::Quaternion:
      //case ezVariant::Type::Matrix3:
      //case ezVariant::Type::Matrix4:
      //case ezVariant::Type::Transform:

    case ezVariant::Type::String:
    case ezVariant::Type::StringView:
      return "string";

      //case ezVariant::Type::Time:
      //case ezVariant::Type::Uuid:
      //case ezVariant::Type::ColorGamma:

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

    // TODO: currently only allows void functions
    if (pFunc->GetReturnType() != nullptr)
      goto ignore;

    {
      const char* szType = TsType(pFunc->GetReturnType());

      if (szType == nullptr)
        goto ignore;

      sFunc.Append(szType);
    }

    // function body
    {
      ezUInt32 uiFuncHash = ComputeFunctionBindingHash(pRtti, pFunc);
      sFunc.AppendFormat(" { __CPP_ComponentFunction_Call0(this, {0}); }\n", uiFuncHash);
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

int __CPP_ComponentFunction_Call0(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk);

  const ezUInt32 uiFuncHash = duk.GetUIntParameter(1);

  const ezTypeScriptBinding::FunctionBinding* pBinding = ezTypeScriptBinding::FindFunctionBinding(uiFuncHash);

  if (pBinding == nullptr)
  {
    ezLog::Error("Bound function with hash {} not found.", uiFuncHash);
    return duk.ReturnVoid();
  }

  ezHybridArray<ezVariant, 10> args;
  ezVariant ret;

  pBinding->m_pFunc->Execute(pComponent, args, ret);

  return duk.ReturnVoid();
}
