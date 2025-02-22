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
#include <Foundation/Threading/Thread.h>
#include <Foundation/Types/Variant.h>

static ezAsAllocatorType* g_pAsAllocator = nullptr;
static void OnThreadEvent(const ezThreadEvent& e);

EZ_IMPLEMENT_SINGLETON(ezAngelScriptEngineSingleton);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(AngelScriptPlugin, AngelScriptEngineSingleton)

BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
END_SUBSYSTEM_DEPENDENCIES

ON_CORESYSTEMS_SHUTDOWN
{
  if (g_pAsAllocator)
  {
    ezThread::s_ThreadEvents.RemoveEventHandler(OnThreadEvent);
    EZ_DEFAULT_DELETE(g_pAsAllocator);
  }
}

ON_HIGHLEVELSYSTEMS_STARTUP
{
  g_pAsAllocator = EZ_DEFAULT_NEW(ezAsAllocatorType, "AngelScript", ezFoundation::GetDefaultAllocator());
  ezThread::s_ThreadEvents.AddEventHandler(OnThreadEvent);
  EZ_DEFAULT_NEW(ezAngelScriptEngineSingleton);
}

ON_HIGHLEVELSYSTEMS_SHUTDOWN
{
  ezAngelScriptEngineSingleton* pDummy = ezAngelScriptEngineSingleton::GetSingleton();
  EZ_DEFAULT_DELETE(pDummy);
}

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on



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

static void OnThreadEvent(const ezThreadEvent& e)
{
  if (e.m_Type == ezThreadEvent::Type::ClearThreadLocals)
  {
    asThreadCleanup();
  }
}

ezAngelScriptEngineSingleton::ezAngelScriptEngineSingleton()
  : m_SingletonRegistrar(this)
{
  EZ_LOG_BLOCK("ezAngelScriptEngineSingleton");

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
      intptr_t flags = 0;

      if (pRtti != ezGetStaticRTTI<ezComponent>())
      {
        // derived type
        flags = 0x01;
      }

      const char* compName = pRtti->GetTypeName().GetStartPointer();

      int funcID;

      funcID = m_pEngine->RegisterObjectMethod(compName, "bool SendMessage(ezMessage& inout ref_msg)", asMETHODPR(ezComponent, SendMessage, (ezMessage&), bool), asCALL_THISCALL);
      AS_CHECK(funcID);
      m_pEngine->GetFunctionById(funcID)->SetUserData(reinterpret_cast<void*>(flags), ezAsUserData::FuncFlags);

      funcID = m_pEngine->RegisterObjectMethod(compName, "bool SendMessage(ezMessage& inout ref_msg) const", asMETHODPR(ezComponent, SendMessage, (ezMessage&) const, bool), asCALL_THISCALL);
      AS_CHECK(funcID);
      m_pEngine->GetFunctionById(funcID)->SetUserData(reinterpret_cast<void*>(flags), ezAsUserData::FuncFlags);

      funcID = m_pEngine->RegisterObjectMethod(compName, "void PostMessage(const ezMessage& in msg, ezTime delay = ezTime::MakeZero(), ezObjectMsgQueueType queueType = ezObjectMsgQueueType::NextFrame) const", asMETHOD(ezComponent, PostMessage), asCALL_THISCALL);
      AS_CHECK(funcID);
      m_pEngine->GetFunctionById(funcID)->SetUserData(reinterpret_cast<void*>(flags), ezAsUserData::FuncFlags);

      funcID = m_pEngine->RegisterObjectMethod(compName, "ezComponentHandle GetHandle() const", asMETHOD(ezComponent, GetHandle), asCALL_THISCALL);
      AS_CHECK(funcID);
      m_pEngine->GetFunctionById(funcID)->SetUserData(reinterpret_cast<void*>(flags), ezAsUserData::FuncFlags);
      //
    });
}
