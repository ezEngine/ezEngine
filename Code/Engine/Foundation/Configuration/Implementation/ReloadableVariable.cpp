#include <Foundation/PCH.h>
#include <Foundation/Configuration/ReloadableVariable.h>
#include <Foundation/Communication/GlobalEvent.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezReloadableVariableBase);

ezMap<ezString, ezMemoryStreamStorage>* ezReloadableVariableBase::s_StoredVariables = NULL;

EZ_ON_GLOBAL_EVENT(ezFoundation_Shutdown)
{
  ezReloadableVariableBase::Shutdown();
}


void ezReloadableVariableBase::Shutdown()
{
  EZ_DEFAULT_DELETE(s_StoredVariables);
  s_StoredVariables = NULL;
}

typedef ezMap<ezString, ezMemoryStreamStorage> MemStreamMap;

void ezReloadableVariableBase::StoreVariables()
{
  if (s_StoredVariables == NULL)
    s_StoredVariables = EZ_DEFAULT_NEW(MemStreamMap);

  ezReloadableVariableBase* pVar = GetFirstInstance();

  while (pVar)
  {
    ezMemoryStreamWriter Writer(&(*s_StoredVariables)[pVar->m_szVariableName]);

    pVar->SaveState(Writer);

    pVar = pVar->GetNextInstance();
  }
}

void ezReloadableVariableBase::RetrieveVariable(const char* szVarName, ezReloadableVariableBase* pVariable)
{
  if (s_StoredVariables)
  {
    if (s_StoredVariables->Find(szVarName).IsValid())
    {
      ezMemoryStreamReader Reader(&(*s_StoredVariables)[szVarName]);
      pVariable->LoadState(Reader);
    }
  }
}



EZ_STATICLINK_REFPOINT(Foundation_Configuration_Implementation_ReloadableVariable);

