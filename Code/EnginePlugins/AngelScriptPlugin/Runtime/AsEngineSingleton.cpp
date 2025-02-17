#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScript/source/add_on/scriptarray/scriptarray.h>
#include <AngelScript/source/add_on/scriptdictionary/scriptdictionary.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Runtime/AsInstance.h>
#include <AngelScriptPlugin/Runtime/AsStringFactory.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Core/Scripting/ScriptComponent.h>
#include <Core/World/Component.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Types/Variant.h>

EZ_IMPLEMENT_SINGLETON(ezAngelScriptEngineSingleton);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(AngelScriptPlugin, AngelScriptEngineSingleton)

BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
END_SUBSYSTEM_DEPENDENCIES

ON_HIGHLEVELSYSTEMS_STARTUP
{
  EZ_DEFAULT_NEW(ezAngelScriptEngineSingleton);
}

ON_HIGHLEVELSYSTEMS_SHUTDOWN
{
  ezAngelScriptEngineSingleton* pDummy = ezAngelScriptEngineSingleton::GetSingleton();
  EZ_DEFAULT_DELETE(pDummy);
}

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on


static ezAsAllocatorType* g_pAsAllocator = nullptr;

static void* ezAsMalloc(size_t uiSize)
{
  return g_pAsAllocator->Allocate(uiSize, 8);
}

static void ezAsFree(void* pPtr)
{
  g_pAsAllocator->Deallocate(pPtr);
}

static void AsThrow(ezStringView sMsg)
{
  if (asIScriptContext* ctx = asGetActiveContext())
  {
    ezStringBuilder tmp;
    ctx->SetException(sMsg.GetData(tmp), false);
  }
}

ezAngelScriptEngineSingleton::ezAngelScriptEngineSingleton()
  : m_SingletonRegistrar(this)
{
  EZ_LOG_BLOCK("ezAngelScriptEngineSingleton");

  m_pAllocator = EZ_DEFAULT_NEW(ezAsAllocatorType, "AngelScript", ezFoundation::GetDefaultAllocator());
  g_pAsAllocator = m_pAllocator.Borrow();

  asSetGlobalMemoryFunctions(ezAsMalloc, ezAsFree);

  m_pEngine = asCreateScriptEngine();
  // m_pEngine->SetEngineProperty(asEP_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE, 1); // means we can't copy messages during PostMessage
  m_pEngine->SetEngineProperty(asEP_REQUIRE_ENUM_SCOPE, 1);
  m_pEngine->SetEngineProperty(asEP_DISALLOW_GLOBAL_VARS, 1);

  AS_CHECK(m_pEngine->SetMessageCallback(asMETHOD(ezAngelScriptEngineSingleton, CompilerMessageCallback), this, asCALL_THISCALL));
  AS_CHECK(m_pEngine->SetTranslateAppExceptionCallback(asMETHOD(ezAngelScriptEngineSingleton, ExceptionCallback), this, asCALL_THISCALL));

  m_pStringFactory = EZ_DEFAULT_NEW(ezAsStringFactory);

  AS_CHECK(m_pEngine->RegisterInterface("ezAngelScriptMessage"));

  RegisterScriptArray(m_pEngine, true);
  // RegisterScriptDictionary(m_pEngine);

  RegisterStandardTypes();

  AS_CHECK(m_pEngine->RegisterGlobalFunction("void throw(ezStringView)", asFUNCTION(AsThrow), asCALL_CDECL));

  m_pEngine->RegisterStringFactory("ezStringView", m_pStringFactory);

  Register_ReflectedTypes();
  Register_GlobalReflectedFunctions();

  Register_ezAngelScriptClass();

  AddForbiddenType("ezStringView");
  AddForbiddenType("ezStringBuilder");
}

ezAngelScriptEngineSingleton::~ezAngelScriptEngineSingleton()
{
  m_pEngine->ShutDownAndRelease();

  if (m_pStringFactory)
  {
    ezAsStringFactory* pFactor = (ezAsStringFactory*)m_pStringFactory;
    EZ_DEFAULT_DELETE(pFactor);
  }

  g_pAsAllocator = nullptr;
  m_pAllocator.Clear();
}

void ezAngelScriptEngineSingleton::AddForbiddenType(const char* szTypeName)
{
  asITypeInfo* pTypeInfo = m_pEngine->GetTypeInfoByName(szTypeName);
  EZ_ASSERT_DEV(pTypeInfo != nullptr, "Type '{}' not found", szTypeName);

  m_ForbiddenTypes.PushBack(pTypeInfo);
}

bool ezAngelScriptEngineSingleton::IsTypeForbidden(const asITypeInfo* pType) const
{
  return m_ForbiddenTypes.Contains(pType);
}

void ezAngelScriptEngineSingleton::ExceptionCallback(asIScriptContext* pContext)
{
  ezLog::Error("AngelScript: App-Exception: {}", pContext->GetExceptionString());
}

void ezAngelScriptEngineSingleton::RegisterStandardTypes()
{
  EZ_LOG_BLOCK("AS::RegisterStandardTypes");

  AS_CHECK(m_pEngine->RegisterTypedef("ezInt8", "int8"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezInt16", "int16"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezInt32", "int32"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezInt64", "int64"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezUInt8", "uint8"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezUInt16", "uint16"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezUInt32", "uint32"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezUInt64", "uint64"));

  AS_CHECK(m_pEngine->RegisterObjectType("ezRTTI", 0, asOBJ_REF | asOBJ_NOCOUNT));

  // TODO AngelScript: ezResult ?

  RegisterPodValueType<ezVec2>();
  RegisterPodValueType<ezVec3>();
  RegisterPodValueType<ezVec4>();
  RegisterPodValueType<ezAngle>();
  RegisterPodValueType<ezQuat>();
  RegisterPodValueType<ezMat3>();
  RegisterPodValueType<ezMat4>();
  RegisterPodValueType<ezTransform>();
  RegisterPodValueType<ezTime>();
  RegisterPodValueType<ezColor>();
  RegisterPodValueType<ezColorGammaUB>();
  RegisterPodValueType<ezStringView>();
  RegisterPodValueType<ezGameObjectHandle>();
  RegisterPodValueType<ezComponentHandle>();
  RegisterPodValueType<ezTempHashedString>();
  RegisterPodValueType<ezHashedString>();

  RegisterNonPodValueType<ezString>();
  RegisterNonPodValueType<ezStringBuilder>();

  RegisterRefType<ezGameObject>();
  RegisterRefType<ezComponent>();
  RegisterRefType<ezWorld>();
  RegisterRefType<ezMessage>();
  RegisterRefType<ezClock>();

  {
    AS_CHECK(m_pEngine->RegisterObjectType("ezRandom", 0, asOBJ_REF | asOBJ_NOCOUNT));
    AddForbiddenType("ezRandom");
  }

  Register_RTTI();
  Register_Vec2();
  Register_Vec3();
  Register_Vec4();
  Register_Angle();
  Register_Quat();
  Register_Transform();
  Register_GameObject();
  Register_Time();
  Register_Mat3();
  Register_Mat4();
  Register_World();
  Register_Clock();
  Register_StringView();
  Register_String();
  Register_StringBuilder();
  Register_HashedString();
  Register_TempHashedString();
  Register_Color();
  Register_ColorGammaUB();
  Register_Random();
  Register_Math();
  Register_Spatial();

  // TODO AngelScript: register these standard types
  // ezBoundingBox
  // ezBoundingSphere
  // ezPlane
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

void ezAngelScriptEngineSingleton::Register_ReflectedTypes()
{
  EZ_LOG_BLOCK("Register_ReflectedTypes");

  Register_ReflectedType(ezGetStaticRTTI<ezComponent>(), false);
  Register_ReflectedType(ezGetStaticRTTI<ezMessage>(), true);

  Register_ExtraComponentFuncs();
}

void ezAngelScriptEngineSingleton::Register_ExtraComponentFuncs()
{
  ezRTTI::ForEachDerivedType(ezGetStaticRTTI<ezComponent>(), [&](const ezRTTI* pRtti)
    {
      const char* compName = pRtti->GetTypeName().GetStartPointer();

      AS_CHECK(m_pEngine->RegisterObjectMethod(compName, "bool SendMessage(ezMessage& inout ref_msg)", asMETHODPR(ezComponent, SendMessage, (ezMessage&), bool), asCALL_THISCALL));
      AS_CHECK(m_pEngine->RegisterObjectMethod(compName, "bool SendMessage(ezMessage& inout ref_msg) const", asMETHODPR(ezComponent, SendMessage, (ezMessage&) const, bool), asCALL_THISCALL));
      AS_CHECK(m_pEngine->RegisterObjectMethod(compName, "void PostMessage(const ezMessage& in msg, ezTime delay = ezTime::MakeZero(), ezObjectMsgQueueType queueType = ezObjectMsgQueueType::NextFrame) const", asMETHOD(ezComponent, PostMessage), asCALL_THISCALL));
      AS_CHECK(m_pEngine->RegisterObjectMethod(compName, "ezComponentHandle GetHandle() const", asMETHOD(ezComponent, GetHandle), asCALL_THISCALL));
      //
    });
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////



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
    decl.Append(Register_EnumType(pRtti));

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

  if (uiArg > 0)
  {
    decl.Append(", ");
  }

  return AppendType(decl, pFunc->GetArgumentType(uiArg), pFuncAttr, uiArg, inout_VarArgs);
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
  ezStringBuilder decl;
  bool bVarArgs = false;

  if (!AppendType(decl, pFunc->GetReturnType(), nullptr, ezInvalidIndex, bVarArgs))
  {
    return;
  }

  ezStringBuilder sNamespace;
  ezStringBuilder sFuncName = pFunc->GetPropertyName();
  decl.Append(" ");
  sFuncName.TrimWordStart("Reflection_");

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

  decl.Append(sFuncName, "(");

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
      if (uiVarArgOpt > 0 || !ExistsGlobalFunc(m_pEngine, sNamespace, sFuncName))
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



static void SetPropertyGeneric(asIScriptGeneric* pGen)
{
  const ezAbstractMemberProperty* pMember = static_cast<const ezAbstractMemberProperty*>(pGen->GetAuxiliary());

  ezVariant value;
  ezAngelScriptUtils::ReadFromAsTypeAtLocation(pGen->GetEngine(), pGen->GetArgTypeId(0), pGen->GetAddressOfArg(0), value).AssertSuccess();

  ezReflectionUtils::SetMemberPropertyValue(pMember, pGen->GetObject(), value);
}

static void GetPropertyGeneric(asIScriptGeneric* pGen)
{
  const ezAbstractMemberProperty* pMember = static_cast<const ezAbstractMemberProperty*>(pGen->GetAuxiliary());

  const ezVariant value = ezReflectionUtils::GetMemberPropertyValue(pMember, pGen->GetObject());

  const ezRTTI* pRtti = ezAngelScriptUtils::MapToRTTI(pGen->GetReturnTypeId(), pGen->GetEngine());
  ezAngelScriptUtils::DefaultConstructInPlace(pGen->GetAddressOfReturnLocation(), pRtti);

  ezAngelScriptUtils::WriteToAsTypeAtLocation(pGen->GetEngine(), pGen->GetReturnTypeId(), pGen->GetAddressOfReturnLocation(), value).AssertSuccess();
}

void ezAngelScriptEngineSingleton::RegisterTypeProperties(const char* szTypeName, const ezRTTI* pRtti, bool bIsInherited)
{
  if (pRtti == nullptr)
    return;

  intptr_t flags = 0;
  if (bIsInherited)
  {
    flags |= 0x01;
  }

  ezArrayPtr<const ezAbstractProperty* const> properties = pRtti->GetProperties();

  ezStringBuilder funcName, sVarTypeName;

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() == ezPropertyCategory::Member)
    {
      const ezAbstractMemberProperty* pMember = static_cast<const ezAbstractMemberProperty*>(pProp);

      sVarTypeName.Clear();

      if (pMember->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
      {
        const ezAbstractEnumerationProperty* pEnumProp = static_cast<const ezAbstractEnumerationProperty*>(pMember);

        sVarTypeName = Register_EnumType(pEnumProp->GetSpecificType());
      }
      else
      {
        const ezRTTI* pPropRtti = pMember->GetSpecificType();

        sVarTypeName = ezAngelScriptUtils::VariantTypeToString(pPropRtti->GetVariantType());

        if (sVarTypeName.IsEmpty())
        {
          if (pPropRtti->GetTypeName() == "ezGameObjectHandle" || pPropRtti->GetTypeName() == "ezComponentHandle")
          {
            sVarTypeName = pPropRtti->GetTypeName();
          }
        }
      }

      if (!sVarTypeName.IsEmpty())
      {
        if (!pMember->GetFlags().IsAnySet(ezPropertyFlags::Const | ezPropertyFlags::ReadOnly))
        {
          funcName.Set("void set_", pMember->GetPropertyName(), "(", sVarTypeName, ") property ");
          const int funcID = m_pEngine->RegisterObjectMethod(szTypeName, funcName, asFUNCTION(SetPropertyGeneric), asCALL_GENERIC, (void*)pMember);
          AS_CHECK(funcID);
          m_pEngine->GetFunctionById(funcID)->SetUserData(reinterpret_cast<void*>(flags), ezAsUserData::FuncFlags);
        }

        {
          funcName.Set(sVarTypeName, " get_", pMember->GetPropertyName(), "() const property");
          const int funcID = m_pEngine->RegisterObjectMethod(szTypeName, funcName, asFUNCTION(GetPropertyGeneric), asCALL_GENERIC, (void*)pMember);
          AS_CHECK(funcID);
          m_pEngine->GetFunctionById(funcID)->SetUserData(reinterpret_cast<void*>(flags), ezAsUserData::FuncFlags);
        }
      }
    }
  }

  RegisterTypeProperties(szTypeName, pRtti->GetParentType(), true);
}

ezString ezAngelScriptEngineSingleton::Register_EnumType(const ezRTTI* pEnumType)
{
  ezStringBuilder enumName = pEnumType->GetTypeName();
  enumName.ReplaceAll("::", "_");

  asITypeInfo* pEnumTypeInfo = m_pEngine->GetTypeInfoByName(enumName);
  if (pEnumTypeInfo != nullptr)
    return enumName;

  m_pEngine->RegisterEnum(enumName);

  pEnumTypeInfo = m_pEngine->GetTypeInfoByName(enumName);
  pEnumTypeInfo->SetUserData((void*)pEnumType, ezAsUserData::RttiPtr);

  ezHybridArray<ezReflectionUtils::EnumKeyValuePair, 16> enumValues;
  ezReflectionUtils::GetEnumKeysAndValues(pEnumType, enumValues, ezReflectionUtils::EnumConversionMode::ValueNameOnly);
  for (auto& enumValue : enumValues)
  {
    m_pEngine->RegisterEnumValue(enumName, enumValue.m_sKey, enumValue.m_iValue);
  }

  return enumName;
}
