#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

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

ezUInt32 ezTypeScriptBinding::ComputePropertyBindingHash(const ezRTTI* pType, const ezAbstractMemberProperty* pMember)
{
  ezStringBuilder sFuncName;

  sFuncName.Set(pType->GetTypeName(), "::", pMember->GetPropertyName());

  return ezHashingUtils::StringHashTo32(ezHashingUtils::StringHash(sFuncName.GetData()));
}

void ezTypeScriptBinding::SetupRttiPropertyBindings()
{
  if (!s_BoundProperties.IsEmpty())
    return;

  ezRTTI::ForEachDerivedType<ezComponent>(
    [&](const ezRTTI* pRtti)
    {
      for (const ezAbstractProperty* pProp : pRtti->GetProperties())
      {
        if (pProp->GetCategory() != ezPropertyCategory::Member)
          continue;

        const ezUInt32 uiHash = ComputePropertyBindingHash(pRtti, static_cast<const ezAbstractMemberProperty*>(pProp));
        EZ_ASSERT_DEV(!s_BoundProperties.Contains(uiHash), "Hash collision for bound property name!");

        s_BoundProperties[uiHash].m_pMember = static_cast<const ezAbstractMemberProperty*>(pProp);
      }
    });
}

void ezTypeScriptBinding::GeneratePropertiesCode(ezStringBuilder& out_Code, const ezRTTI* pRtti)
{
  ezStringBuilder sProp;

  for (const ezAbstractProperty* pProp : pRtti->GetProperties())
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member)
      continue;

    auto pMember = static_cast<const ezAbstractMemberProperty*>(pProp);
    const ezRTTI* pPropType = pProp->GetSpecificType();

    const ezUInt32 uiHash = ComputePropertyBindingHash(pRtti, pMember);

    ezStringBuilder sTypeName;

    sTypeName = TsType(pPropType);
    if (sTypeName.IsEmpty())
      continue;

    sProp.SetFormat("  get {0}(): {1} { return __CPP_ComponentProperty_get(this, {2}); }\n", pMember->GetPropertyName(), sTypeName, uiHash);
    out_Code.Append(sProp.GetView());

    sProp.SetFormat("  set {0}(value: {1}) { __CPP_ComponentProperty_set(this, {2}, value); }\n", pMember->GetPropertyName(), sTypeName, uiHash);
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
  ezDuktapeFunction duk(pDuk);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk);

  const ezUInt32 uiHash = duk.GetUIntValue(1);

  const ezTypeScriptBinding::PropertyBinding* pBinding = ezTypeScriptBinding::FindPropertyBinding(uiHash);

  if (pBinding == nullptr)
  {
    ezLog::Error("Bound property with hash {} not found.", uiHash);
    return duk.ReturnVoid();
  }

  const ezVariant value = ezReflectionUtils::GetMemberPropertyValue(pBinding->m_pMember, pComponent);
  ezTypeScriptBinding::PushVariant(duk, value);
  return duk.ReturnCustom();
}

int __CPP_ComponentProperty_set(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk);

  const ezUInt32 uiHash = duk.GetUIntValue(1);

  const ezTypeScriptBinding::PropertyBinding* pBinding = ezTypeScriptBinding::FindPropertyBinding(uiHash);

  if (pBinding == nullptr)
  {
    ezLog::Error("Bound property with hash {} not found.", uiHash);
    return duk.ReturnVoid();
  }

  const ezVariant value = ezTypeScriptBinding::GetVariant(duk, 2, pBinding->m_pMember->GetSpecificType());

  ezReflectionUtils::SetMemberPropertyValue(pBinding->m_pMember, pComponent, value);

  return duk.ReturnVoid();
}
