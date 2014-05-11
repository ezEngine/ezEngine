#include <Foundation/PCH.h>
#include <Foundation/Configuration/ReloadableVariable.h>
#include <Foundation/Communication/GlobalEvent.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezReloadableVariableBase);

ezMap<ezString, ezMemoryStreamStorage> ezReloadableVariableBase::s_StoredVariables;

void ezReloadableVariableBase::StoreVariables()
{
  ezReloadableVariableBase* pVar = GetFirstInstance();

  while (pVar)
  {
    ezMemoryStreamWriter Writer(&s_StoredVariables[pVar->m_szVariableName]);

    pVar->SaveState(Writer);

    pVar = pVar->GetNextInstance();
  }
}

void ezReloadableVariableBase::RetrieveVariable(const char* szVarName, ezReloadableVariableBase* pVariable)
{
  if (s_StoredVariables.Find(szVarName).IsValid())
  {
    ezMemoryStreamReader Reader(&s_StoredVariables[szVarName]);
    pVariable->LoadState(Reader);
  }
}



EZ_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_ReloadableVariable);

