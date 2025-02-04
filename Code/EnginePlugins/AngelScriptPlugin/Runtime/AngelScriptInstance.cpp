#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AngelScriptEngineSingleton.h>
#include <AngelScriptPlugin/Runtime/AngelScriptInstance.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Core/Scripting/ScriptComponent.h>

ezAngelScriptInstance::ezAngelScriptInstance(ezReflectedClass& inout_owner, ezWorld* pWorld, asIScriptModule* pModule, const char* szObjectTypeName)
  : ezScriptInstance(inout_owner, pWorld)
{
  EZ_ASSERT_DEBUG(inout_owner.GetDynamicRTTI()->IsDerivedFrom<ezComponent>(), "Invalid owner");

  m_pOwnerComponent = static_cast<ezScriptComponent*>(&inout_owner);

  auto pAsEngine = ezAngelScriptEngineSingleton::GetSingleton();

  m_pContext = pAsEngine->GetEngine()->CreateContext();

  if (asITypeInfo* pClassType = pModule->GetTypeInfoByDecl(szObjectTypeName))
  {
    if (auto pFactory = pClassType->GetFactoryByIndex(0))
    {
      AS_CHECK(m_pContext->Prepare(pFactory));
      AS_CHECK(m_pContext->Execute());

      m_pObject = (asIScriptObject*)m_pContext->GetReturnObject();
      EZ_ASSERT_DEBUG(m_pObject, "Failed to create script object");

      if (m_pObject)
      {
        m_pObject->AddRef();
        m_pObject->SetUserData(this, ezAsUserData::ScriptInstancePtr);
      }
    }
  }
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
    m_pContext->Release();
    m_pContext = nullptr;
  }
}

void ezAngelScriptInstance::SetInstanceVariable(const ezHashedString& sName, const ezVariant& value)
{
  if (m_pObject)
  {
    // TODO AngelScript: this could be a more efficient lookup instead of a search

    for (ezUInt32 i = 0; i < m_pObject->GetPropertyCount(); ++i)
    {
      if (sName == m_pObject->GetPropertyName(i))
      {
        const int typeId = m_pObject->GetPropertyTypeId(i);
        void* pProp = m_pObject->GetAddressOfProperty(i);

        ezAngelScriptUtils::WriteAsProperty(typeId, pProp, m_pObject->GetEngine(), value);
        return;
      }
    }

    ezLog::Error("The variable '{}' doesn't exist in the Angel Script.", sName);
  }
}

ezVariant ezAngelScriptInstance::GetInstanceVariable(const ezHashedString& sName)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return {};
}
