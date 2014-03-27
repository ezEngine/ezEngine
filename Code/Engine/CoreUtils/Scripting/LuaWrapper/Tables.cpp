#include <CoreUtils/PCH.h>
#include <CoreUtils/Scripting/LuaWrapper.h>

ezResult ezLuaWrapper::OpenTable(const char* szName)
{
  if (m_States.m_iOpenTables == 0)
    lua_getglobal(m_pState, szName);
  else
  {
    lua_pushstring(m_pState, szName);
    lua_gettable(m_pState, -2);
  }

  //failed, it's no table
  if (lua_istable(m_pState, -1) == 0)
  {
    lua_pop(m_pState, 1);
    return EZ_FAILURE;
  }

  m_States.m_iOpenTables++;
  return EZ_SUCCESS;
}

ezResult ezLuaWrapper::OpenTableFromParameter(ezUInt32 iFunctionParameter)
{
  lua_pushvalue(m_pState, iFunctionParameter + s_ParamOffset);

  //failed, it's no table
  if (lua_istable(m_pState, -1) == 0)
  {
    lua_pop(m_pState, 1);
    return EZ_FAILURE;
  }

  m_States.m_iOpenTables++;
  return EZ_SUCCESS;
}

void ezLuaWrapper::CloseTable()
{
  DiscardReturnValues();

  if (m_States.m_iOpenTables == 0)
    return;

  m_States.m_iOpenTables--;

  lua_pop(m_pState, 1);
}

void ezLuaWrapper::CloseAllTables()
{
  DiscardReturnValues();

  if (m_States.m_iOpenTables == 0)
    return;

  lua_pop(m_pState, m_States.m_iOpenTables);
  m_States.m_iOpenTables = 0;
}



EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Scripting_LuaWrapper_Tables);

