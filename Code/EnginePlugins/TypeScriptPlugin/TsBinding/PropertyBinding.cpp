#include <TypeScriptPluginPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

ezHashTable<ezUInt32, ezTypeScriptBinding::PropertyBinding> ezTypeScriptBinding::s_BoundProperties;

static int __CPP_ComponentProperty_get(duk_context* pDuk);
static int __CPP_ComponentProperty_set(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_PropertyBinding()
{
  m_Duk.RegisterGlobalFunction("__CPP_ComponentProperty_get", __CPP_ComponentProperty_get, 2);
  m_Duk.RegisterGlobalFunction("__CPP_ComponentProperty_set", __CPP_ComponentProperty_set, 3);

  return EZ_SUCCESS;
}

ezUInt32 ezTypeScriptBinding::ComputePropertyBindingHash(const ezRTTI* pType, ezAbstractMemberProperty* pMember)
{
  ezStringBuilder sFuncName;

  sFuncName.Set(pType->GetTypeName(), "::", pMember->GetPropertyName());

  return ezTempHashedString::ComputeHash(sFuncName.GetData());
}

void ezTypeScriptBinding::SetupRttiPropertyBindings()
{
  if (!s_BoundProperties.IsEmpty())
    return;

  for (const ezRTTI* pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->IsDerivedFrom<ezComponent>())
      continue;

    for (ezAbstractProperty* pProp : pRtti->GetProperties())
    {
      if (pProp->GetCategory() != ezPropertyCategory::Member)
        continue;

      const ezUInt32 uiHash = ComputePropertyBindingHash(pRtti, static_cast<ezAbstractMemberProperty*>(pProp));
      EZ_ASSERT_DEV(!s_BoundProperties.Contains(uiHash), "Hash collision for bound property name!");

      s_BoundProperties[uiHash].m_pMember = static_cast<ezAbstractMemberProperty*>(pProp);
    }
  }
}

void ezTypeScriptBinding::GeneratePropertiesCode(ezStringBuilder& out_Code, const ezRTTI* pRtti)
{
  ezStringBuilder sProp;

  for (ezAbstractProperty* pProp : pRtti->GetProperties())
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member)
      continue;

    ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pProp);

    const char* szTypeName = TsType(pMember->GetSpecificType());
    if (szTypeName == nullptr)
      continue;

    const ezUInt32 uiHash = ComputePropertyBindingHash(pRtti, pMember);

    sProp.Format("  get {0}(): {1} { return __CPP_ComponentProperty_get(this, {2}); }\n", pMember->GetPropertyName(), szTypeName, uiHash);
    out_Code.Append(sProp.GetView());

    sProp.Format("  set {0}(value: {1}) { __CPP_ComponentProperty_set(this, {2}, value); }\n", pMember->GetPropertyName(), szTypeName, uiHash);
    out_Code.Append(sProp.GetView());
  }
}

const ezTypeScriptBinding::PropertyBinding* ezTypeScriptBinding::FindPropertyBinding(ezUInt32 uiHash)
{
  const PropertyBinding* pBinding = nullptr;
  s_BoundProperties.TryGetValue(uiHash, pBinding);
  return pBinding;
}

int __CPP_ComponentProperty_get(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, +1);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk);

  const ezUInt32 uiHash = duk.GetUIntValue(1);

  const ezTypeScriptBinding::PropertyBinding* pBinding = ezTypeScriptBinding::FindPropertyBinding(uiHash);

  if (pBinding == nullptr)
  {
    ezLog::Error("Bound property with hash {} not found.", uiHash);
    return duk.ReturnVoid();
  }

  const ezVariant value = ezReflectionUtils::GetMemberPropertyValue(pBinding->m_pMember, pComponent);

  switch (value.GetType())
  {
    case ezVariant::Type::Bool:
      return duk.ReturnBool(value.Get<bool>());

    case ezVariant::Type::Int8:
    case ezVariant::Type::Int16:
    case ezVariant::Type::Int32:
    case ezVariant::Type::Int64:
      return duk.ReturnInt(value.ConvertTo<ezInt32>());

    case ezVariant::Type::UInt8:
    case ezVariant::Type::UInt16:
    case ezVariant::Type::UInt32:
    case ezVariant::Type::UInt64:
      return duk.ReturnUInt(value.ConvertTo<ezUInt32>());

    case ezVariant::Type::Float:
    case ezVariant::Type::Double:
      return duk.ReturnNumber(value.ConvertTo<double>());

    case ezVariant::Type::String:
    case ezVariant::Type::StringView:
      return duk.ReturnString(value.ConvertTo<ezString>());

    case ezVariant::Type::Vector3:
      ezTypeScriptBinding::PushVec3(duk, value.Get<ezVec3>());
      return duk.ReturnCustom();

    case ezVariant::Type::Quaternion:
      ezTypeScriptBinding::PushQuat(duk, value.Get<ezQuat>());
      return duk.ReturnCustom();

    case ezVariant::Type::Color:
      ezTypeScriptBinding::PushColor(duk, value.Get<ezColor>());
      return duk.ReturnCustom();

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  duk.SetExpectedStackChange(0);
  return duk.ReturnVoid();
}

int __CPP_ComponentProperty_set(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, 0);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk);

  const ezUInt32 uiHash = duk.GetUIntValue(1);

  const ezTypeScriptBinding::PropertyBinding* pBinding = ezTypeScriptBinding::FindPropertyBinding(uiHash);

  if (pBinding == nullptr)
  {
    ezLog::Error("Bound property with hash {} not found.", uiHash);
    return duk.ReturnVoid();
  }

  ezVariant value;

  switch (pBinding->m_pMember->GetSpecificType()->GetVariantType())
  {
    case ezVariant::Type::Bool:
      value = duk.GetBoolValue(2);
      break;

    case ezVariant::Type::Int8:
    case ezVariant::Type::Int16:
    case ezVariant::Type::Int32:
    case ezVariant::Type::Int64:
      value = duk.GetIntValue(2);
      break;

    case ezVariant::Type::UInt8:
    case ezVariant::Type::UInt16:
    case ezVariant::Type::UInt32:
    case ezVariant::Type::UInt64:
      value = duk.GetUIntValue(2);
      break;

    case ezVariant::Type::Float:
    case ezVariant::Type::Double:
      value = duk.GetFloatValue(2);
      break;

    case ezVariant::Type::String:
    case ezVariant::Type::StringView:
      value = duk.GetStringValue(2);
      break;

    case ezVariant::Type::Vector3:
      value = ezTypeScriptBinding::GetVec3(duk, 2);
      break;

    case ezVariant::Type::Quaternion:
      value = ezTypeScriptBinding::GetQuat(duk, 2);
      break;

    case ezVariant::Type::Color:
      value = ezTypeScriptBinding::GetColor(duk, 2);
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  ezReflectionUtils::SetMemberPropertyValue(pBinding->m_pMember, pComponent, value);
  return duk.ReturnVoid();
}
