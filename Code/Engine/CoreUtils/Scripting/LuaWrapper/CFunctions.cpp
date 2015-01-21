#include <CoreUtils/PCH.h>
#include <CoreUtils/Scripting/LuaWrapper.h>
#include <Foundation/Logging/Log.h>

void ezLuaWrapper::RegisterCFunction(const char* szFunctionName, lua_CFunction pFunction, void* pLightUserData) const
{
  lua_pushlightuserdata(m_pState, pLightUserData);
  lua_pushcclosure(m_pState, pFunction, 1);
  lua_setglobal(m_pState, szFunctionName);
}

void* ezLuaWrapper::GetFunctionLightUserData() const
{
  return lua_touserdata(m_pState, lua_upvalueindex(1));
}

bool ezLuaWrapper::PrepareFunctionCall(const char* szFunctionName)
{
  EZ_ASSERT_DEV(m_States.m_iLuaReturnValues == 0, "ezLuaWrapper::PrepareFunctionCall: You didn't discard the return-values of the previous script call. %d Return-values were expected.", m_States.m_iLuaReturnValues);

  m_States.m_iParametersPushed = 0;

  if (m_States.m_iOpenTables == 0)
    lua_getglobal(m_pState, szFunctionName);
  else
  {
    lua_pushstring(m_pState, szFunctionName);
    lua_gettable(m_pState, -2);
  }

  if (lua_isfunction(m_pState, -1) == 0)
  {
    lua_pop(m_pState, 1);
    return false;
  }

  return true;
}

ezResult ezLuaWrapper::CallPreparedFunction(ezUInt32 iExpectedReturnValues, ezLogInterface* pLogInterface)
{
  m_States.m_iLuaReturnValues = iExpectedReturnValues;

  // save the current states on a cheap stack
  const ezScriptStates StackedStates = m_States;
  m_States = ezScriptStates();

  if (pLogInterface == nullptr)
    pLogInterface = ezGlobalLog::GetInstance();

  if (lua_pcall(m_pState, StackedStates.m_iParametersPushed, iExpectedReturnValues, 0) != 0)
  {
    // restore the states to their previous values
    m_States = StackedStates;

    m_States.m_iLuaReturnValues = 0;

    ezLog::Error(pLogInterface, "Script-function Call: %s", lua_tostring(m_pState, -1));

    lua_pop(m_pState, 1);  /* pop error message from the stack */
    return EZ_FAILURE;
  }

  // before resetting the state, make sure the returned state has no stuff left
  EZ_ASSERT_DEV((m_States.m_iLuaReturnValues == 0) && (m_States.m_iOpenTables == 0), "After ezLuaWrapper::CallPreparedFunction: Return values: %d, Open Tables: %d", m_States.m_iLuaReturnValues, m_States.m_iOpenTables);

  m_States = StackedStates;
  return EZ_SUCCESS;
}

void ezLuaWrapper::DiscardReturnValues()
{
  if (m_States.m_iLuaReturnValues == 0)
    return;

  lua_pop(m_pState, m_States.m_iLuaReturnValues);
  m_States.m_iLuaReturnValues = 0;
}

bool ezLuaWrapper::IsReturnValueInt (ezUInt32 iReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (iReturnValue + s_ParamOffset) - 1) == LUA_TNUMBER);
}

bool ezLuaWrapper::IsReturnValueBool (ezUInt32 iReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (iReturnValue + s_ParamOffset) - 1) == LUA_TBOOLEAN);
}

bool ezLuaWrapper::IsReturnValueFloat (ezUInt32 iReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (iReturnValue + s_ParamOffset) - 1) == LUA_TNUMBER);
}

bool ezLuaWrapper::IsReturnValueString (ezUInt32 iReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (iReturnValue + s_ParamOffset) - 1) == LUA_TSTRING);
}

bool ezLuaWrapper::IsReturnValueNil(ezUInt32 iReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (iReturnValue + s_ParamOffset) - 1) == LUA_TNIL);
}

ezInt32 ezLuaWrapper::GetIntReturnValue(ezUInt32 iReturnValue) const
{
  return ((int) (lua_tointeger(m_pState, -m_States.m_iLuaReturnValues + (iReturnValue + s_ParamOffset) - 1)));
}

bool ezLuaWrapper::GetBoolReturnValue(ezUInt32 iReturnValue) const
{
  return (lua_toboolean(m_pState, -m_States.m_iLuaReturnValues + (iReturnValue + s_ParamOffset) - 1) != 0);
}

float ezLuaWrapper::GetFloatReturnValue(ezUInt32 iReturnValue) const
{
  return ((float) (lua_tonumber(m_pState, -m_States.m_iLuaReturnValues + (iReturnValue + s_ParamOffset) - 1)));
}

const char* ezLuaWrapper::GetStringReturnValue(ezUInt32 iReturnValue) const
{
  return (lua_tostring(m_pState, -m_States.m_iLuaReturnValues + (iReturnValue + s_ParamOffset) - 1));
}







EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Scripting_LuaWrapper_CFunctions);

