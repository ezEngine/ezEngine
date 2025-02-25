#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Foundation/Reflection/ReflectionUtils.h>

bool ezAngelScriptEngineSingleton::AppendType(ezStringBuilder& decl, const ezRTTI* pRtti, const ezScriptableFunctionAttribute* pFuncAttr, ezUInt32 uiArg, bool& inout_VarArgs)
{
  const bool bIsReturnValue = uiArg == ezInvalidIndex;

  if (pRtti == nullptr)
  {
    decl.Append("void");
    return bIsReturnValue;
  }

  if (pFuncAttr && pFuncAttr->GetArgumentType(uiArg) == ezScriptableFunctionAttribute::ArgType::Inout)
  {
    // not yet supported for most types
    return false;
  }

  if (const char* szTypeName = ezAngelScriptUtils::VariantTypeToString(pRtti->GetVariantType()); szTypeName != nullptr)
  {
    decl.Append(szTypeName);

    if (pFuncAttr && pFuncAttr->GetArgumentType(uiArg) == ezScriptableFunctionAttribute::ArgType::Out)
    {
      decl.Append("& out");
    }

    return true;
  }

  if (pRtti->GetTypeFlags().IsAnySet(ezTypeFlags::IsEnum | ezTypeFlags::Bitflags))
  {
    decl.Append(ezAngelScriptUtils::RegisterEnumType(m_pEngine, pRtti));

    if (pFuncAttr && pFuncAttr->GetArgumentType(uiArg) == ezScriptableFunctionAttribute::ArgType::Out)
    {
      decl.Append("& out");
    }
    return true;
  }

  if (!bIsReturnValue)
  {
    if (pRtti->GetTypeName() == "ezVariant")
    {
      decl.Append("?& in");
      return true;
    }

    if (pRtti->GetTypeName() == "ezVariantArray")
    {
      inout_VarArgs = true;
      return true;
    }

    if (pRtti->GetTypeName() == "ezWorld")
    {
      // skip
      return true;
    }
  }

  if (pRtti->GetTypeName() == "ezGameObjectHandle" || pRtti->GetTypeName() == "ezComponentHandle")
  {
    decl.Append(pRtti->GetTypeName());

    if (pFuncAttr && pFuncAttr->GetArgumentType(uiArg) == ezScriptableFunctionAttribute::ArgType::Out)
    {
      decl.Append("& out");
    }

    return true;
  }

  if (m_WhitelistedRefTypes.Contains(pRtti->GetTypeName()))
  {
    decl.Append(pRtti->GetTypeName(), "@");
    return true;
  }

  decl.Append(pRtti->GetTypeName());
  m_NotRegistered.Insert(decl);
  return false;
}

bool ezAngelScriptEngineSingleton::AppendFuncArgs(ezStringBuilder& decl, const ezAbstractFunctionProperty* pFunc, const ezScriptableFunctionAttribute* pFuncAttr, ezUInt32 uiArg, bool& inout_VarArgs)
{
  if (uiArg > 12)
  {
    EZ_ASSERT_DEBUG(false, "Too many function arguments");
    return false;
  }

  EZ_ASSERT_DEBUG(!inout_VarArgs, "VarArgs have to be the last argument");

  if (uiArg > 0 && !decl.EndsWith("("))
  {
    decl.Append(", ");
  }

  return AppendType(decl, pFunc->GetArgumentType(uiArg), pFuncAttr, uiArg, inout_VarArgs);
}

void ezAngelScriptEngineSingleton::Register_GlobalReflectedFunctions()
{
  EZ_LOG_BLOCK("Register_GlobalReflectedFunctions");

  ezRTTI::ForEachType([&](const ezRTTI* pRtti)
    {
      if (pRtti->GetParentType() != nullptr && pRtti->GetParentType() != ezGetStaticRTTI<ezNoBase>())
        return;

      for (auto pFunc : pRtti->GetFunctions())
      {
        auto pFuncAttr = pFunc->GetAttributeByType<ezScriptableFunctionAttribute>();
        if (!pFuncAttr)
          continue;

        if (pFunc->GetFunctionType() != ezFunctionType::StaticMember)
          continue;

        RegisterGenericFunction(pRtti->GetTypeName().GetStartPointer(), pFunc, pFuncAttr, false);
      }

      //
    });
}

static void CastToBase(asIScriptGeneric* pGen)
{
  int derivedTypeId = pGen->GetObjectTypeId();
  auto derivedTypeInfo = pGen->GetEngine()->GetTypeInfoById(derivedTypeId);
  const ezRTTI* pDerivedRtti = (const ezRTTI*)derivedTypeInfo->GetUserData(ezAsUserData::RttiPtr);

  const ezRTTI* pBaseRtti = (const ezRTTI*)pGen->GetAuxiliary();

  if (pDerivedRtti != nullptr && pBaseRtti != nullptr)
  {
    if (pDerivedRtti->IsDerivedFrom(pBaseRtti))
    {
      pGen->SetReturnObject(pGen->GetObject());
      return;
    }
  }

  pGen->SetReturnObject(nullptr);
}

static void CastToDerived(asIScriptGeneric* pGen)
{
  int baseTypeId = pGen->GetObjectTypeId();
  auto baseTypeInfo = pGen->GetEngine()->GetTypeInfoById(baseTypeId);
  const ezRTTI* pBaseRtti = (const ezRTTI*)baseTypeInfo->GetUserData(ezAsUserData::RttiPtr);

  const ezRTTI* pDerivedRtti = (const ezRTTI*)pGen->GetAuxiliary();

  if (pBaseRtti != nullptr && pDerivedRtti != nullptr)
  {
    if (pDerivedRtti->IsDerivedFrom(pBaseRtti))
    {
      pGen->SetReturnObject(pGen->GetObject());
      return;
    }
  }

  pGen->SetReturnObject(nullptr);
}

struct RefInstance
{
  ezUInt32 m_uiRefCount = 1;
  const ezRTTI* m_pRtti = nullptr;
};

static ezMutex s_RefCountMutex;
static ezMap<void*, RefInstance> s_RefCounts;

static void* ezRtti_Create(const ezRTTI* pRtti)
{
  auto inst = pRtti->GetAllocator()->Allocate<ezReflectedClass>();

  EZ_LOCK(s_RefCountMutex);
  auto& ref = s_RefCounts[inst.m_pInstance];
  ref.m_pRtti = pRtti;

  return inst.m_pInstance;
}

static void ezRtti_AddRef(void* pInstance)
{
  EZ_LOCK(s_RefCountMutex);
  ++s_RefCounts[pInstance].m_uiRefCount;
}

static void ezRtti_Release(void* pInstance)
{
  EZ_LOCK(s_RefCountMutex);
  auto it = s_RefCounts.Find(pInstance);
  RefInstance& ri = it.Value();
  if (--ri.m_uiRefCount == 0)
  {
    ri.m_pRtti->GetAllocator()->Deallocate(pInstance);
    s_RefCounts.Remove(it);
  }
}


void ezAngelScriptEngineSingleton::Register_ReflectedType(const ezRTTI* pBaseType, bool bCreatable)
{
  EZ_LOG_BLOCK("Register_ReflectedType", pBaseType->GetTypeName());

  // first register the type
  ezRTTI::ForEachDerivedType(pBaseType, [&](const ezRTTI* pRtti)
    {
      ezStringBuilder typeName = pRtti->GetTypeName();
      auto pTypeInfo = m_pEngine->GetTypeInfoByName(typeName);

      m_WhitelistedRefTypes.Insert(typeName);

      if (pTypeInfo == nullptr)
      {
        if (bCreatable)
        {
          const int typeId = m_pEngine->RegisterObjectType(typeName, 0, asOBJ_REF);
          AS_CHECK(typeId);
          pTypeInfo = m_pEngine->GetTypeInfoById(typeId);

          if (pRtti->GetAllocator() != nullptr && pRtti->GetAllocator()->CanAllocate())
          {
            const ezStringBuilder sFactoryOp(typeName, "@ f()");
            m_pEngine->RegisterObjectBehaviour(typeName, asBEHAVE_FACTORY, sFactoryOp, asFUNCTION(ezRtti_Create), asCALL_CDECL_OBJLAST, (void*)pRtti);
            m_pEngine->RegisterObjectBehaviour(typeName, asBEHAVE_ADDREF, "void f()", asFUNCTION(ezRtti_AddRef), asCALL_CDECL_OBJLAST, (void*)pRtti);
            m_pEngine->RegisterObjectBehaviour(typeName, asBEHAVE_RELEASE, "void f()", asFUNCTION(ezRtti_Release), asCALL_CDECL_OBJLAST, (void*)pRtti);
          }
        }
        else
        {

          const int typeId = m_pEngine->RegisterObjectType(typeName, 0, asOBJ_REF | asOBJ_NOCOUNT);
          AS_CHECK(typeId);

          pTypeInfo = m_pEngine->GetTypeInfoById(typeId);
        }
      }

      pTypeInfo->SetUserData((void*)pRtti, ezAsUserData::RttiPtr);

      AddForbiddenType(typeName);

      RegisterTypeFunctions(typeName, pRtti, false);
      ezAngelScriptUtils::RegisterTypeProperties(m_pEngine, typeName, pRtti, false);

      //
    },
    ezRTTI::ForEachOptions::None);

  // then register the type hierarchy
  ezRTTI::ForEachDerivedType(pBaseType, [&](const ezRTTI* pRtti)
    {
      if (pRtti == pBaseType)
        return;

      const ezStringBuilder typeName = pRtti->GetTypeName();

      const ezRTTI* pParentRtti = pRtti->GetParentType();

      ezStringBuilder parentName, castOp;

      while (pParentRtti)
      {
        parentName = pParentRtti->GetTypeName();
        castOp.Set(parentName, "@ opImplCast()");

        AS_CHECK(m_pEngine->RegisterObjectMethod(typeName, castOp, asFUNCTION(CastToBase), asCALL_GENERIC, (void*)pParentRtti));

        castOp.Set(typeName, "@ opCast()");
        AS_CHECK(m_pEngine->RegisterObjectMethod(parentName, castOp, asFUNCTION(CastToDerived), asCALL_GENERIC, (void*)pRtti));

        if (pParentRtti == pBaseType)
          break;

        pParentRtti = pParentRtti->GetParentType();
      }
      //
    },
    ezRTTI::ForEachOptions::None);
}

void ezAngelScriptEngineSingleton::RegisterTypeFunctions(const char* szTypeName, const ezRTTI* pRtti, bool bIsInherited)
{
  if (pRtti == nullptr || pRtti == ezGetStaticRTTI<ezReflectedClass>())
    return;

  for (auto pFunc : pRtti->GetFunctions())
  {
    auto pFuncAttr = pFunc->GetAttributeByType<ezScriptableFunctionAttribute>();

    if (!pFuncAttr)
      continue;

    RegisterGenericFunction(szTypeName, pFunc, pFuncAttr, bIsInherited);
  }

  RegisterTypeFunctions(szTypeName, pRtti->GetParentType(), true);
}

static void CollectFunctionArgumentAttributes(const ezAbstractFunctionProperty* pFuncProp, ezDynamicArray<const ezFunctionArgumentAttributes*>& out_attributes)
{
  for (auto pAttr : pFuncProp->GetAttributes())
  {
    if (auto pFuncArgAttr = ezDynamicCast<const ezFunctionArgumentAttributes*>(pAttr))
    {
      ezUInt32 uiArgIndex = pFuncArgAttr->GetArgumentIndex();
      out_attributes.EnsureCount(uiArgIndex + 1);
      EZ_ASSERT_DEV(out_attributes[uiArgIndex] == nullptr, "Multiple argument attributes for argument {}", uiArgIndex);
      out_attributes[uiArgIndex] = pFuncArgAttr;
    }
  }
}

static bool ExistsGlobalFunc(asIScriptEngine* pEngine, const char* szNamespace, const char* szName)
{
  for (ezUInt32 i = 0; i < pEngine->GetGlobalFunctionCount(); ++i)
  {
    auto pFunc = pEngine->GetGlobalFunctionByIndex(i);

    if (ezStringUtils::IsEqual(pFunc->GetNamespace(), szNamespace) && ezStringUtils::IsEqual(pFunc->GetName(), szName))
    {
      return true;
    }
  }

  return false;
}

void ezAngelScriptEngineSingleton::RegisterGenericFunction(const char* szTypeName, const ezAbstractFunctionProperty* const pFunc, const ezScriptableFunctionAttribute* pFuncAttr, bool bIsInherited)
{
  ezStringBuilder sFuncName = pFunc->GetPropertyName();
  sFuncName.TrimWordStart("Reflection_");

  if (pFunc->GetReturnType() == ezGetStaticRTTI<ezVariant>())
  {
    ezStringBuilder sFuncName2;

    sFuncName2.Set(sFuncName, "_asBool");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, ezGetStaticRTTI<bool>());

    sFuncName2.Set(sFuncName, "_asInt32");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, ezGetStaticRTTI<ezInt32>());

    sFuncName2.Set(sFuncName, "_asFloat");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, ezGetStaticRTTI<float>());

    sFuncName2.Set(sFuncName, "_asTime");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, ezGetStaticRTTI<ezTime>());

    sFuncName2.Set(sFuncName, "_asAngle");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, ezGetStaticRTTI<ezAngle>());

    sFuncName2.Set(sFuncName, "_asVec2");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, ezGetStaticRTTI<ezVec2>());

    sFuncName2.Set(sFuncName, "_asVec3");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, ezGetStaticRTTI<ezVec3>());

    sFuncName2.Set(sFuncName, "_asVec4");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, ezGetStaticRTTI<ezVec4>());

    sFuncName2.Set(sFuncName, "_asQuat");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, ezGetStaticRTTI<ezQuat>());

    sFuncName2.Set(sFuncName, "_asColor");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, ezGetStaticRTTI<ezColor>());

    sFuncName2.Set(sFuncName, "_asString");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, ezGetStaticRTTI<ezString>());

    sFuncName2.Set(sFuncName, "_asGameObjectHandle");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, ezGetStaticRTTI<ezGameObjectHandle>());

    sFuncName2.Set(sFuncName, "_asComponentHandle");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, ezGetStaticRTTI<ezComponentHandle>());
  }
  else
  {
    RegisterSingleGenericFunction(sFuncName, szTypeName, pFunc, pFuncAttr, bIsInherited, pFunc->GetReturnType());
  }
}

void ezAngelScriptEngineSingleton::RegisterSingleGenericFunction(const char* szFuncName, const char* szTypeName, const ezAbstractFunctionProperty* const pFunc, const ezScriptableFunctionAttribute* pFuncAttr, bool bIsInherited, const ezRTTI* pReturnType)
{
  ezStringBuilder decl;
  bool bVarArgs = false;

  if (!AppendType(decl, pReturnType, nullptr, ezInvalidIndex, bVarArgs))
  {
    return;
  }

  ezStringBuilder sNamespace;
  decl.Append(" ");

  if (pFunc->GetFunctionType() == ezFunctionType::StaticMember)
  {
    // turn things like 'ezScriptExtensionClass_CVar' into 'ezCVar'
    if (const char* szUnderScore = ezStringUtils::FindLastSubString(szTypeName, "_"))
    {
      if (ezStringUtils::StartsWith(szTypeName, "ez"))
      {
        sNamespace.Append("ez");
      }

      sNamespace.Append(szUnderScore + 1);
    }
    else
    {
      sNamespace.Append(szTypeName);
    }
  }

  decl.Append(szFuncName, "(");

  ezHybridArray<const ezFunctionArgumentAttributes*, 8> argAttributes;
  if (const ezFunctionArgumentAttributes* pArgAttr = pFunc->GetAttributeByType<ezFunctionArgumentAttributes>())
  {
    argAttributes.SetCount(pFunc->GetArgumentCount());
    CollectFunctionArgumentAttributes(pFunc, argAttributes);
  }

  bool bHasDefaultArgs = false;
  ezVariant defaultValue;

  for (ezUInt32 uiArg = 0; uiArg < pFunc->GetArgumentCount(); ++uiArg)
  {
    if (!AppendFuncArgs(decl, pFunc, pFuncAttr, uiArg, bVarArgs))
      return;

    if (bVarArgs)
    {
      // start with 0 arguments
      decl.TrimRight(" ,");
    }
    else
    {
      if (decl.EndsWith("(") || decl.EndsWith(", "))
      {
        decl.TrimRight(" ,");
        continue;
      }

      const ezRTTI* pArgType = pFunc->GetArgumentType(uiArg);

      if (const char* szName = pFuncAttr->GetArgumentName(uiArg))
      {
        decl.Append(" ", szName);
      }

      if (bHasDefaultArgs)
      {
        defaultValue = ezReflectionUtils::GetDefaultVariantFromType(pArgType);
      }

      if (!argAttributes.IsEmpty() && argAttributes[uiArg])
      {
        for (auto pArgAttr : argAttributes[uiArg]->GetArgumentAttributes())
        {
          if (const ezDefaultValueAttribute* pDef = ezDynamicCast<const ezDefaultValueAttribute*>(pArgAttr))
          {
            bHasDefaultArgs = true;
            defaultValue = pDef->GetValue();
          }
        }
      }

      if (bHasDefaultArgs)
      {
        const bool bIsEnum = pArgType->GetTypeFlags().IsAnySet(ezTypeFlags::IsEnum | ezTypeFlags::Bitflags);

        decl.Append(" = ");

        if (bIsEnum)
        {
          decl.Append(pArgType->GetTypeName(), "(");
        }

        if (defaultValue.IsValid())
        {
          if (pArgType->GetVariantType() == ezVariantType::UInt32)
          {
            // special case to make the default value unsigned
            defaultValue = defaultValue.ConvertTo<ezUInt32>();
          }

          decl.Append(ezAngelScriptUtils::DefaultValueToString(defaultValue));
        }
        else
        {
          decl.Append("0"); // fallback for enums
        }

        if (bIsEnum)
        {
          decl.Append(")");
        }
      }
    }
  }

  intptr_t flags = 0;
  if (bIsInherited)
  {
    flags |= 0x01;
  }

  for (ezUInt32 uiVarArgOpt = 0; uiVarArgOpt < 9; ++uiVarArgOpt)
  {
    decl.Append(")");

    if (pFunc->GetFunctionType() == ezFunctionType::Member)
    {
      if (pFunc->GetFlags().IsSet(ezPropertyFlags::Const))
        decl.Append(" const");

      // only register methods that have not been registered before
      // this allows us to register more optimized versions first
      if (m_pEngine->GetTypeInfoByName(szTypeName)->GetMethodByDecl(decl) == nullptr)
      {
        const int funcID = m_pEngine->RegisterObjectMethod(szTypeName, decl, asFUNCTION(ezAngelScriptUtils::MakeGenericFunctionCall), asCALL_GENERIC, (void*)pFunc);
        AS_CHECK(funcID);

        m_pEngine->GetFunctionById(funcID)->SetUserData(reinterpret_cast<void*>(flags), ezAsUserData::FuncFlags);
      }
    }
    else if (pFunc->GetFunctionType() == ezFunctionType::StaticMember)
    {
      m_pEngine->SetDefaultNamespace(sNamespace);

      // only register functions that have not been registered before
      // this allows us to register more optimized versions first
      if (uiVarArgOpt > 0 || !ExistsGlobalFunc(m_pEngine, sNamespace, szFuncName))
      {
        const int funcID = m_pEngine->RegisterGlobalFunction(decl, asFUNCTION(ezAngelScriptUtils::MakeGenericFunctionCall), asCALL_GENERIC, (void*)pFunc);
        AS_CHECK(funcID);

        m_pEngine->GetFunctionById(funcID)->SetUserData(reinterpret_cast<void*>(flags), ezAsUserData::FuncFlags);
      }

      m_pEngine->SetDefaultNamespace("");
    }

    if (!bVarArgs)
      break;

    decl.Shrink(0, 1);
    decl.AppendFormat(", ?& in VarArg{}", uiVarArgOpt + 1);
  }
}
