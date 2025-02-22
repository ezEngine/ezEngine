#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Runtime/AsInstance.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Core/Scripting/ScriptComponent.h>

ezAngelScriptInstance::ezAngelScriptInstance(ezReflectedClass& inout_owner, ezWorld* pWorld, asIScriptModule* pModule, const char* szObjectTypeName)
  : ezScriptInstance(inout_owner, pWorld)
{
  EZ_ASSERT_DEBUG(inout_owner.GetDynamicRTTI()->IsDerivedFrom<ezComponent>(), "Invalid owner");

  m_pOwnerComponent = static_cast<ezScriptComponent*>(&inout_owner);

  auto pAsEngine = ezAngelScriptEngineSingleton::GetSingleton();

  m_pContext = pAsEngine->GetEngine()->CreateContext();
  AS_CHECK(m_pContext->SetExceptionCallback(asMETHOD(ezAngelScriptInstance, ExceptionCallback), this, asCALL_THISCALL));

  if (asITypeInfo* pClassType = pModule->GetTypeInfoByName(szObjectTypeName))
  {
    if (auto pFactory = pClassType->GetFactoryByIndex(0))
    {
      AS_CHECK(m_pContext->Prepare(pFactory));
      AS_CHECK(m_pContext->Execute());

      m_pObject = (asIScriptObject*)m_pContext->GetReturnObject();

      if (m_pObject)
      {
        m_pObject->AddRef();
        m_pObject->SetUserData(this, ezAsUserData::ScriptInstancePtr);
        return;
      }
    }
  }

  ezLog::Error("Failed to create AngelScript object of type '{}'", szObjectTypeName);
}

ezAngelScriptInstance::~ezAngelScriptInstance()
{
  if (m_pObject)
  {
    m_pObject->Release();
    m_pObject = nullptr;
  }

  if (m_pContext)
  {
    EZ_ASSERT_DEBUG(!m_pContext->IsNested(), "Invalid time to release context!");

    m_pContext->Release();
    m_pContext = nullptr;
  }
}

void ezAngelScriptInstance::SetInstanceVariable(const ezHashedString& sName, const ezVariant& value)
{
  if (!m_pObject)
    return;

  // TODO AngelScript: this could be a more efficient lookup instead of a search

  for (ezUInt32 i = 0; i < m_pObject->GetPropertyCount(); ++i)
  {
    if (sName == m_pObject->GetPropertyName(i))
    {
      const int typeId = m_pObject->GetPropertyTypeId(i);
      void* pProp = m_pObject->GetAddressOfProperty(i);

      ezAngelScriptUtils::WriteToAsTypeAtLocation(m_pObject->GetEngine(), typeId, pProp, value).AssertSuccess();
      return;
    }
  }

  ezLog::Error("The variable '{}' doesn't exist in the Angel Script.", sName);
}

ezVariant ezAngelScriptInstance::GetInstanceVariable(const ezHashedString& sName)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return {};
}

void ezAngelScriptInstance::ExceptionCallback(asIScriptContext* pContext)
{
  EZ_LOG_BLOCK("AS Exception", m_pOwnerComponent->GetScriptClass().GetResourceIdOrDescription());

  ezLog::Error("AS Exception '{}' - in '{}'", pContext->GetExceptionString(), m_pOwnerComponent->GetScriptClass().GetResourceIdOrDescription());

  const ezUInt32 uiNumLevels = pContext->GetCallstackSize();

  for (ezUInt32 i = 0; i < uiNumLevels; ++i)
  {
    if (asIScriptFunction* pFunc = pContext->GetFunction(i))
    {
      ezStringBuilder line("  ");

      if (!ezStringUtils::IsNullOrEmpty(pFunc->GetNamespace()))
      {
        line.Append(pFunc->GetNamespace(), "::");
      }

      if (!ezStringUtils::IsNullOrEmpty(pFunc->GetObjectName()))
      {
        line.Append(pFunc->GetObjectName(), "::");
      }

      const char* szSection = nullptr;
      int lineNbr = pContext->GetLineNumber(i, nullptr, &szSection);

      line.AppendFormat("{}() - Line {} in '{}'", pFunc->GetName(), lineNbr, szSection);

      ezLog::Error(line);
    }
    else
    {
      ezLog::Error("  <nested call>");
    }
  }
}
