#pragma once

template<typename Type>
ezReloadableVariableWrapper<Type>::ezReloadableVariableWrapper(Type& Variable, const char* szVarName) : m_Variable(Variable)
{
  m_szVariableName = szVarName;

  RetrieveVariable(m_szVariableName, this);
}

template<typename Type>
void ezReloadableVariableWrapper<Type>::SaveState(ezIBinaryStreamWriter& Stream)
{
  Stream << m_Variable;
}

template<typename Type>
void ezReloadableVariableWrapper<Type>::LoadState(ezIBinaryStreamReader& Stream)
{
  Stream >> m_Variable;
}
