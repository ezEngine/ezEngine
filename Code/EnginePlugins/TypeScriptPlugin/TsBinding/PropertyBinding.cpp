#include <TypeScriptPluginPCH.h>

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

ezHashTable<ezUInt32, ezTypeScriptBinding::PropertyBinding> ezTypeScriptBinding::s_BoundProperties;

//static int __CPP_ComponentFunction_Call(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_PropertyBinding()
{
  //m_Duk.RegisterFunctionWithVarArgs("__CPP_ComponentFunction_Call", __CPP_ComponentFunction_Call);

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

    sProp.Format("  get {0}(): {1} { return bla(); }\n", pMember->GetPropertyName(), szTypeName);
    out_Code.Append(sProp.GetView());

    sProp.Format("  set {0}(value: {1}) { blub(value); }\n", pMember->GetPropertyName(), szTypeName);
    out_Code.Append(sProp.GetView());
  }
}

const ezTypeScriptBinding::PropertyBinding* ezTypeScriptBinding::FindPropertyBinding(ezUInt32 uiHash)
{
  const PropertyBinding* pBinding = nullptr;
  s_BoundProperties.TryGetValue(uiHash, pBinding);
  return pBinding;
}
