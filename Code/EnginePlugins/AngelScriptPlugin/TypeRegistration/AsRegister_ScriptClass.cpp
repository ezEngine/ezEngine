#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Runtime/AsInstance.h>
#include <Core/Scripting/ScriptComponent.h>

static ezGameObject* GetAngelScriptOwnerObject(asIScriptObject* pSelf)
{
  if (pSelf)
  {
    ezAngelScriptInstance* pInstance = (ezAngelScriptInstance*)pSelf->GetUserData(ezAsUserData::ScriptInstancePtr);
    pSelf->Release();

    return pInstance->GetOwnerComponent()->GetOwner();
  }

  return nullptr;
}

static ezWorld* GetAngelScriptOwnerWorld(asIScriptObject* pSelf)
{
  if (pSelf)
  {
    ezAngelScriptInstance* pInstance = (ezAngelScriptInstance*)pSelf->GetUserData(ezAsUserData::ScriptInstancePtr);
    pSelf->Release();

    return pInstance->GetOwnerComponent()->GetWorld();
  }

  return nullptr;
}

static ezScriptComponent* GetAngelScriptOwnerComponent(asIScriptObject* pSelf)
{
  if (pSelf)
  {
    ezAngelScriptInstance* pInstance = (ezAngelScriptInstance*)pSelf->GetUserData(ezAsUserData::ScriptInstancePtr);
    pSelf->Release();

    return pInstance->GetOwnerComponent();
  }

  return nullptr;
}


void ezAngelScriptEngineSingleton::Register_ezAngelScriptClass()
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
      GetScriptOwnerComponent(@this).UpdateInterval = interval;
    }
}
    )";

  if (SetModuleCode("Builtin_AngelScriptClass", szClassCode, false) == nullptr)
  {
    EZ_REPORT_FAILURE("Failed to register ezAngelScriptClass class");
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezScriptComponent", "void BroadcastEventMsg(const ezEventMessage& in msg)", asMETHOD(ezScriptComponent, BroadcastEventMsg), asCALL_THISCALL));
}
