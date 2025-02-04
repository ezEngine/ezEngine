#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScript/source/add_on/scriptstdstring/scriptstdstring.h>
#include <AngelScriptPlugin/Runtime/AngelScriptEngineSingleton.h>
#include <AngelScriptPlugin/Runtime/AngelScriptInstance.h>
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

class ezAsStringFactory : public asIStringFactory
{
public:
  ezAsStringFactory() = default;
  ~ezAsStringFactory() = default;

  const void* GetStringConstant(const char* data, asUINT length) override
  {
    ezHashedString hs;
    hs.Assign(ezStringView(data, length));

    // we need to give out a pointer to a StringView that doesn't vanish
    EZ_LOCK(m_Mutex);
    auto it = m_Strings.Insert(hs.GetView());
    const ezStringView& view = it.Key();

    return &view;
  }


  int ReleaseStringConstant(const void* str) override
  {
    // we don't clean up the strings
    return 0;
  }

  int GetRawStringData(const void* str, char* data, asUINT* length) const override
  {
    const ezStringView* pView = (const ezStringView*)str;

    *length = pView->GetElementCount();

    if (data)
    {
      ezStringUtils::Copy(data, *length + 1, pView->GetStartPointer());
    }

    return 0;
  }

private:
  ezMutex m_Mutex;
  ezSet<ezStringView> m_Strings;
};

ezGameObject* GetAngelScriptOwnerObject(asIScriptObject* pSelf)
{
  if (pSelf)
  {
    ezAngelScriptInstance* pInstance = (ezAngelScriptInstance*)pSelf->GetUserData(ezAsUserData::ScriptInstancePtr);
    pSelf->Release();

    return pInstance->GetOwnerComponent()->GetOwner();
  }

  return nullptr;
}

ezWorld* GetAngelScriptOwnerWorld(asIScriptObject* pSelf)
{
  if (pSelf)
  {
    ezAngelScriptInstance* pInstance = (ezAngelScriptInstance*)pSelf->GetUserData(ezAsUserData::ScriptInstancePtr);
    pSelf->Release();

    return pInstance->GetOwnerComponent()->GetWorld();
  }

  return nullptr;
}

ezScriptComponent* GetAngelScriptOwnerComponent(asIScriptObject* pSelf)
{
  if (pSelf)
  {
    ezAngelScriptInstance* pInstance = (ezAngelScriptInstance*)pSelf->GetUserData(ezAsUserData::ScriptInstancePtr);
    pSelf->Release();

    return pInstance->GetOwnerComponent();
  }

  return nullptr;
}

static ezProxyAllocator* g_pAsAllocator = nullptr;

static void* ezAsMalloc(size_t size)
{
  return g_pAsAllocator->Allocate(size, 8);
}

static void ezAsFree(void* pPtr)
{
  g_pAsAllocator->Deallocate(pPtr);
}

ezAngelScriptEngineSingleton::ezAngelScriptEngineSingleton()
  : m_SingletonRegistrar(this)
{
  EZ_LOG_BLOCK("ezAngelScriptEngineSingleton");

  m_pAllocator = EZ_DEFAULT_NEW(ezProxyAllocator, "AngelScript", ezFoundation::GetDefaultAllocator());
  g_pAsAllocator = m_pAllocator.Borrow();

  asSetGlobalMemoryFunctions(ezAsMalloc, ezAsFree);

  m_pEngine = asCreateScriptEngine();
  m_pEngine->SetEngineProperty(asEP_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE, 1);
  m_pEngine->SetEngineProperty(asEP_REQUIRE_ENUM_SCOPE, 1);
  m_pEngine->SetEngineProperty(asEP_DISALLOW_GLOBAL_VARS, 1);

  AS_CHECK(m_pEngine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL));

  m_pStringFactory = EZ_DEFAULT_NEW(ezAsStringFactory);

  RegisterStandardTypes();

  m_pEngine->RegisterStringFactory("ezStringView", m_pStringFactory);

  Register_GlobalReflectedFunctions();
  Register_ReflectedTypes();

  Register_ScriptClass();

  AddForbiddenType("ezStringBuilder");
}

ezAngelScriptEngineSingleton::~ezAngelScriptEngineSingleton()
{
  m_pEngine->Release();

  if (m_pStringFactory)
  {
    ezAsStringFactory* pFactor = (ezAsStringFactory*)m_pStringFactory;
    EZ_DEFAULT_DELETE(pFactor);
  }

  g_pAsAllocator = nullptr;
  m_pAllocator.Clear();
}


asIScriptModule* ezAngelScriptEngineSingleton::SetModuleCode(ezStringView sModuleName, ezStringView sCode, bool bAddExternalSection)
{
  ezStringBuilder tmp;

  asIScriptModule* pModule = m_pEngine->GetModule(sModuleName.GetData(tmp), asGM_ALWAYS_CREATE);

  if (bAddExternalSection)
  {
    const char* szExternal = R"(
external shared class ezAngelScriptClass;
)";

    pModule->AddScriptSection("External", szExternal);
  }

  pModule->AddScriptSection("Main", sCode.GetStartPointer(), sCode.GetElementCount());

  if (int r = pModule->Build(); r < 0)
  {
    // The build failed. The message stream will have received
    // compiler errors that shows what needs to be fixed

    pModule->Discard();
    return nullptr;
  }

  return pModule;
}

class ezAsPreprocessor
{
public:
  ezStringView m_sRefFilePath;
  ezStringView m_sMainCode;

  ezAsPreprocessor()
  {
    m_Processor.SetFileOpenFunction(ezMakeDelegate(&ezAsPreprocessor::PreProc_OpenFile, this));
    m_Processor.m_ProcessingEvents.AddEventHandler(ezMakeDelegate(&ezAsPreprocessor::PreProc_Event, this));
  }

  ezResult Process(ezStringBuilder& result)
  {
    return m_Processor.Process(m_sRefFilePath, result);
  };

private:
  ezResult PreProc_OpenFile(ezStringView sAbsFile, ezDynamicArray<ezUInt8>& out_Content, ezTimestamp& out_FileModification)
  {
    if (sAbsFile == m_sRefFilePath)
    {
      out_Content.SetCount(m_sMainCode.GetElementCount());
      ezMemoryUtils::RawByteCopy(out_Content.GetData(), m_sMainCode.GetStartPointer(), m_sMainCode.GetElementCount());
      return EZ_SUCCESS;
    }

    ezFileReader file;
    if (file.Open(sAbsFile).Failed())
      return EZ_FAILURE;

    out_Content.SetCountUninitialized((ezUInt32)file.GetFileSize());
    file.ReadBytes(out_Content.GetData(), out_Content.GetCount());
    return EZ_SUCCESS;
  }

  void PreProc_Event(const ezPreprocessor::ProcessingEvent& event)
  {
    switch (event.m_Type)
    {
      case ezPreprocessor::ProcessingEvent::Error:
        ezLog::Error("{0}: Line {1} [{2}]: {}", event.m_pToken->m_File.GetString(), event.m_pToken->m_uiLine, event.m_pToken->m_uiColumn, event.m_sInfo);
        break;
      case ezPreprocessor::ProcessingEvent::Warning:
        ezLog::Warning("{0}: Line {1} [{2}]: {}", event.m_pToken->m_File.GetString(), event.m_pToken->m_uiLine, event.m_pToken->m_uiColumn, event.m_sInfo);
        break;
      default:
        break;
    }
  }

  ezPreprocessor m_Processor;
};

asIScriptModule* ezAngelScriptEngineSingleton::CompileModule(ezStringView sModuleName, ezStringView sMainClass, ezStringView sRefFilePath, ezStringView sCode)
{
  ezStringBuilder tmp;

  ezAsPreprocessor asPP;
  asPP.m_sRefFilePath = sRefFilePath;
  asPP.m_sMainCode = sCode;

  ezStringBuilder fullCode;
  if (asPP.Process(fullCode).Failed())
  {
    ezLog::Error("Failed to pre-process AngelScript");
    return nullptr;
  }

  asIScriptModule* pModule = SetModuleCode(sModuleName, fullCode, true);

  if (pModule == nullptr)
    return nullptr;

  const asITypeInfo* pClassType = pModule->GetTypeInfoByName(sMainClass.GetData(tmp));

  if (pClassType == nullptr)
  {
    ezLog::Error("AngelScript code doesn't contain class '{}'", sMainClass);
    return nullptr;
  }

  if (ValidateModule(pModule).Failed())
  {
    return nullptr;
  }

  return pModule;
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

void ezAngelScriptEngineSingleton::MessageCallback(const asSMessageInfo* msg, void* param)
{
  switch (msg->type)
  {
    case asMSGTYPE_ERROR:
      ezLog::Error("AngelScript: {} ({}, {}) : {}", msg->section, msg->row, msg->col, msg->message);
      break;
    case asMSGTYPE_WARNING:
      ezLog::Warning("AngelScript: {} ({}, {}) : {}", msg->section, msg->row, msg->col, msg->message);
      break;
    case asMSGTYPE_INFORMATION:
      ezLog::Info("AngelScript: {} ({}, {}) : {}", msg->section, msg->row, msg->col, msg->message);
      break;
  }
}

void CastToBase(asIScriptGeneric* gen)
{
  int derivedTypeId = gen->GetObjectTypeId();
  auto derivedTypeInfo = gen->GetEngine()->GetTypeInfoById(derivedTypeId);
  const ezRTTI* pDerivedRtti = (const ezRTTI*)derivedTypeInfo->GetUserData(ezAsUserData::RttiPtr);

  const ezRTTI* pBaseRtti = (const ezRTTI*)gen->GetAuxiliary();

  if (pDerivedRtti != nullptr && pBaseRtti != nullptr)
  {
    if (pDerivedRtti->IsDerivedFrom(pBaseRtti))
    {
      gen->SetReturnObject(gen->GetObject());
      return;
    }
  }

  gen->SetReturnObject(nullptr);
}

void CastToDerived(asIScriptGeneric* gen)
{
  int baseTypeId = gen->GetObjectTypeId();
  auto baseTypeInfo = gen->GetEngine()->GetTypeInfoById(baseTypeId);
  const ezRTTI* pBaseRtti = (const ezRTTI*)baseTypeInfo->GetUserData(ezAsUserData::RttiPtr);

  const ezRTTI* pDerivedRtti = (const ezRTTI*)gen->GetAuxiliary();

  if (pBaseRtti != nullptr && pDerivedRtti != nullptr)
  {
    if (pDerivedRtti->IsDerivedFrom(pBaseRtti))
    {
      gen->SetReturnObject(gen->GetObject());
      return;
    }
  }

  gen->SetReturnObject(nullptr);
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

static void ezRtti_AddRef(void* instance)
{
  EZ_LOCK(s_RefCountMutex);
  ++s_RefCounts[instance].m_uiRefCount;
}

static void ezRtti_Release(void* instance)
{
  EZ_LOCK(s_RefCountMutex);
  auto it = s_RefCounts.Find(instance);
  RefInstance& ri = it.Value();
  if (--ri.m_uiRefCount == 0)
  {
    ri.m_pRtti->GetAllocator()->Deallocate(instance);
    s_RefCounts.Remove(it);
  }
}

void ezAngelScriptEngineSingleton::Register_ReflectedType(const ezRTTI* pBaseType, bool bCreatable)
{
  EZ_LOG_BLOCK("Register_ReflectedType", pBaseType->GetTypeName());

  ezStringBuilder typeName, parentName, op;

  // first register the type
  ezRTTI::ForEachDerivedType(pBaseType, [&](const ezRTTI* pRtti)
    {
      if (pRtti == pBaseType)
        return;

      typeName = pRtti->GetTypeName();

      if (bCreatable)
      {
        const int typeId = m_pEngine->RegisterObjectType(typeName, 0, asOBJ_REF);
        AS_CHECK(typeId);
        m_pEngine->GetTypeInfoById(typeId)->SetUserData((void*)pRtti, ezAsUserData::RttiPtr);

        if (pRtti->GetAllocator() != nullptr && pRtti->GetAllocator()->CanAllocate())
        {
          op.Set(typeName, "@ f()");
          m_pEngine->RegisterObjectBehaviour(typeName, asBEHAVE_FACTORY, op, asFUNCTION(ezRtti_Create), asCALL_CDECL_OBJLAST, (void*)pRtti);
          m_pEngine->RegisterObjectBehaviour(typeName, asBEHAVE_ADDREF, "void f()", asFUNCTION(ezRtti_AddRef), asCALL_CDECL_OBJLAST, (void*)pRtti);
          m_pEngine->RegisterObjectBehaviour(typeName, asBEHAVE_RELEASE, "void f()", asFUNCTION(ezRtti_Release), asCALL_CDECL_OBJLAST, (void*)pRtti);
        }
      }
      else
      {
        const int typeId = m_pEngine->RegisterObjectType(typeName, 0, asOBJ_REF | asOBJ_NOCOUNT);
        AS_CHECK(typeId);
        m_pEngine->GetTypeInfoById(typeId)->SetUserData((void*)pRtti, ezAsUserData::RttiPtr);
      }

      AddForbiddenType(typeName);

      RegisterTypeFunctions(typeName, pRtti);
      RegisterTypeProperties(typeName, pRtti);

      //
    },
    ezRTTI::ForEachOptions::None);

  // then register the type hierarchy
  ezRTTI::ForEachDerivedType(pBaseType, [&](const ezRTTI* pRtti)
    {
      if (pRtti == pBaseType)
        return;

      typeName = pRtti->GetTypeName();

      const ezRTTI* pParentRtti = pRtti->GetParentType();

      while (pParentRtti)
      {
        parentName = pParentRtti->GetTypeName();
        op.Set(parentName, "@ opImplCast()");

        AS_CHECK(m_pEngine->RegisterObjectMethod(typeName, op, asFUNCTION(CastToBase), asCALL_GENERIC, (void*)pParentRtti));

        op.Set(typeName, "@ opCast()");
        AS_CHECK(m_pEngine->RegisterObjectMethod(parentName, op, asFUNCTION(CastToDerived), asCALL_GENERIC, (void*)pRtti));

        if (pParentRtti == pBaseType)
          break;

        pParentRtti = pParentRtti->GetParentType();
      }
      //
    },
    ezRTTI::ForEachOptions::None);
}

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

  decl.Append(pRtti->GetTypeName());

  if (pRtti->GetTypeName() == "ezGameObjectHandle" || pRtti->GetTypeName() == "ezComponentHandle")
  {
    if (pFuncAttr && pFuncAttr->GetArgumentType(uiArg) == ezScriptableFunctionAttribute::ArgType::Out)
    {
      decl.Append("& out");
    }

    return true;
  }


  if (m_WhitelistedRefTypes.Contains(pRtti->GetTypeName()))
  {
    decl.Append("@");
    return true;
  }

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

void ezAngelScriptEngineSingleton::RegisterGenericFunction(const char* szTypeName, const ezAbstractFunctionProperty* const pFunc, const ezScriptableFunctionAttribute* pFuncAttr)
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
      if (const char* szName = pFuncAttr->GetArgumentName(uiArg))
      {
        decl.Append(" ", szName);
      }

      if (bHasDefaultArgs)
      {
        defaultValue = ezReflectionUtils::GetDefaultVariantFromType(pFunc->GetArgumentType(uiArg));
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
        const bool bIsEnum = pFunc->GetArgumentType(uiArg)->GetTypeFlags().IsAnySet(ezTypeFlags::IsEnum | ezTypeFlags::Bitflags);

        decl.Append(" = ");

        if (bIsEnum)
        {
          decl.Append(pFunc->GetArgumentType(uiArg)->GetTypeName(), "(");
        }

        if (defaultValue.IsValid())
          decl.Append(ezAngelScriptUtils::DefaultValueToString(defaultValue));
        else
          decl.Append("0"); // fallback for enums

        if (bIsEnum)
        {
          decl.Append(")");
        }
      }
    }
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
        AS_CHECK(m_pEngine->RegisterObjectMethod(szTypeName, decl, asFUNCTION(ezAngelScriptUtils::MakeGenericFunctionCall), asCALL_GENERIC, (void*)pFunc));
      }
    }
    else if (pFunc->GetFunctionType() == ezFunctionType::StaticMember)
    {
      m_pEngine->SetDefaultNamespace(sNamespace);

      // only register functions that have not been registered before
      // this allows us to register more optimized versions first
      if (m_pEngine->GetGlobalFunctionByDecl(decl) == nullptr)
      {
        AS_CHECK(m_pEngine->RegisterGlobalFunction(decl, asFUNCTION(ezAngelScriptUtils::MakeGenericFunctionCall), asCALL_GENERIC, (void*)pFunc));
      }

      m_pEngine->SetDefaultNamespace("");
    }

    if (!bVarArgs)
      break;

    decl.Shrink(0, 1);
    decl.AppendFormat(", ?& in VarArg{}", uiVarArgOpt + 1);
  }
}

void ezAngelScriptEngineSingleton::RegisterTypeFunctions(const char* szTypeName, const ezRTTI* pRtti)
{
  for (auto pFunc : pRtti->GetFunctions())
  {
    auto pFuncAttr = pFunc->GetAttributeByType<ezScriptableFunctionAttribute>();

    if (!pFuncAttr)
      continue;

    RegisterGenericFunction(szTypeName, pFunc, pFuncAttr);
  }

  if (pRtti == nullptr || pRtti == ezGetStaticRTTI<ezReflectedClass>())
    return;

  RegisterTypeFunctions(szTypeName, pRtti->GetParentType());
}

void ezAngelScriptEngineSingleton::Register_ScriptClass()
{
  AS_CHECK(m_pEngine->RegisterInterface("ezIAngelScriptClass"));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezGameObject@ GetScriptOwnerObject(ezIAngelScriptClass@ self)", asFUNCTION(GetAngelScriptOwnerObject), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezScriptComponent@ GetScriptOwnerComponent(ezIAngelScriptClass@ self)", asFUNCTION(GetAngelScriptOwnerComponent), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezWorld@ GetScriptOwnerWorld(ezIAngelScriptClass@ self)", asFUNCTION(GetAngelScriptOwnerWorld), asCALL_CDECL));

  const char* szClassCode = R"(
shared class ezAngelScriptClass : ezIAngelScriptClass
{
    ezScriptComponent@ GetOwnerComponent()
    {
        return GetScriptOwnerComponent(@this);
    }

    ezGameObject@ GetOwner()
    {
        return GetScriptOwnerObject(@this);
    }

    ezWorld@ GetWorld()
    {
        return GetScriptOwnerWorld(@this);
    }

    void SetUpdateInterval(ezTime interval)
    {
      GetScriptOwnerComponent(@this).SetUpdateInterval(interval);
    }
}
    )";

  if (SetModuleCode("Builtin_AngelScriptClass", szClassCode, false) == nullptr)
  {
    EZ_REPORT_FAILURE("Failed to register ezAngelScriptClass class");
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezScriptComponent", "void BroadcastEventMsg(const ezEventMessage& in msg)", asMETHOD(ezScriptComponent, BroadcastEventMsg), asCALL_THISCALL));
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

        RegisterGenericFunction(pRtti->GetTypeName().GetStartPointer(), pFunc, pFuncAttr);
      }

      //
    });
}

static void SetPropertyGeneric(asIScriptGeneric* gen)
{
  const ezAbstractMemberProperty* pMember = static_cast<const ezAbstractMemberProperty*>(gen->GetAuxiliary());

  pMember->SetValuePtr(gen->GetObject(), gen->GetAddressOfArg(0));
}

static void GetPropertyGeneric(asIScriptGeneric* gen)
{
  const ezAbstractMemberProperty* pMember = static_cast<const ezAbstractMemberProperty*>(gen->GetAuxiliary());

  const ezRTTI* pRtti = ezAngelScriptUtils::MapToRTTI(gen->GetReturnTypeId(), gen->GetEngine());
  ezAngelScriptUtils::DefaultConstructInPlace(gen->GetAddressOfReturnLocation(), pRtti);

  pMember->GetValuePtr(gen->GetObject(), gen->GetAddressOfReturnLocation());
}

void ezAngelScriptEngineSingleton::RegisterTypeProperties(const char* szTypeName, const ezRTTI* pRtti)
{
  ezHybridArray<const ezAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

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
      }

      if (!sVarTypeName.IsEmpty())
      {
        if (!pMember->GetFlags().IsAnySet(ezPropertyFlags::Const | ezPropertyFlags::ReadOnly))
        {
          funcName.Set("void set_", pMember->GetPropertyName(), "(", sVarTypeName, ") property ");
          AS_CHECK(m_pEngine->RegisterObjectMethod(szTypeName, funcName, asFUNCTION(SetPropertyGeneric), asCALL_GENERIC, (void*)pMember));
        }

        funcName.Set(sVarTypeName, " get_", pMember->GetPropertyName(), "() const property");
        AS_CHECK(m_pEngine->RegisterObjectMethod(szTypeName, funcName, asFUNCTION(GetPropertyGeneric), asCALL_GENERIC, (void*)pMember));
      }
    }
  }
}

ezString ezAngelScriptEngineSingleton::Register_EnumType(const ezRTTI* pEnumType)
{
  ezStringBuilder enumName = pEnumType->GetTypeName();
  enumName.ReplaceAll("::", "_");

  asITypeInfo* pEnumTypeInfo = m_pEngine->GetTypeInfoByName(enumName);
  if (pEnumTypeInfo != nullptr)
    return enumName;

  m_pEngine->RegisterEnum(enumName);

  ezHybridArray<ezReflectionUtils::EnumKeyValuePair, 16> enumValues;
  ezReflectionUtils::GetEnumKeysAndValues(pEnumType, enumValues, ezReflectionUtils::EnumConversionMode::ValueNameOnly);
  for (auto& enumValue : enumValues)
  {
    m_pEngine->RegisterEnumValue(enumName, enumValue.m_sKey, enumValue.m_iValue);
  }

  return enumName;
}

ezResult ezAngelScriptEngineSingleton::ValidateModule(asIScriptModule* pModule) const
{
  ezResult res = EZ_SUCCESS;

  for (ezUInt32 i = 0; i < pModule->GetGlobalVarCount(); ++i)
  {
    const char* szName;
    int typeId;

    if (pModule->GetGlobalVar(i, &szName, nullptr, &typeId) == asSUCCESS)
    {
      if (const asITypeInfo* pInfo = pModule->GetEngine()->GetTypeInfoById(typeId))
      {
        if (IsTypeForbidden(pInfo))
        {
          ezLog::Error("Global variable '{}' uses forbidden type '{}'", szName, pInfo->GetName());
          res = EZ_FAILURE;
        }
      }
    }
  }

  for (ezUInt32 i = 0; i < pModule->GetObjectTypeCount(); ++i)
  {
    const asITypeInfo* pType = pModule->GetObjectTypeByIndex(i);

    for (ezUInt32 i2 = 0; i2 < pType->GetPropertyCount(); ++i2)
    {
      const char* szName;
      int typeId;

      pType->GetProperty(i2, &szName, &typeId);

      if (const asITypeInfo* pInfo = pModule->GetEngine()->GetTypeInfoById(typeId))
      {
        if (IsTypeForbidden(pInfo))
        {
          ezLog::Error("Property '{}::{}' uses forbidden type '{}'", pType->GetName(), szName, pInfo->GetName());
          res = EZ_FAILURE;
        }
      }
    }
  }

  return res;
}

class BytecodeStream : public asIBinaryStream
{
public:
  int Write(const void* ptr, asUINT uiSize)
  {
    m_pBuffer->PushBackRange(ezConstByteArrayPtr((const ezUInt8*)ptr, uiSize));
    return uiSize;
  }

  int Read(void* pPtr, asUINT uiSize)
  {
    const ezUInt32 uiReadSize = ezMath::Min(uiSize, m_pBuffer->GetCount() - m_uiReadPos);

    ezMemoryUtils::RawByteCopy(pPtr, m_pBuffer->GetData() + m_uiReadPos, uiReadSize);

    m_uiReadPos += uiReadSize;
    return uiReadSize;
  }

  ezUInt32 m_uiReadPos = 0;
  ezDynamicArray<ezUInt8>* m_pBuffer = nullptr;
};

void ezAngelScriptEngineSingleton::SaveByteCode(asIScriptModule* pModule, ezDynamicArray<ezUInt8>& out_ByteCode)
{
  BytecodeStream stream;
  stream.m_pBuffer = &out_ByteCode;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  pModule->SaveByteCode(&stream, false); // TODO AS: strip debug info ? (flag?)
#else
  pModule->SaveByteCode(&stream, true);
#endif
}

asIScriptModule* ezAngelScriptEngineSingleton::LoadFromByteCode(ezStringView sModuleName, ezDynamicArray<ezUInt8>& out_ByteCode) const
{
  BytecodeStream stream;
  stream.m_pBuffer = &out_ByteCode;

  ezStringBuilder tmp;
  asIScriptModule* pModule = m_pEngine->GetModule(sModuleName.GetData(tmp), asGM_ALWAYS_CREATE);

  if (pModule->LoadByteCode(&stream) < 0)
  {
    return nullptr;
  }

  return pModule;
}
