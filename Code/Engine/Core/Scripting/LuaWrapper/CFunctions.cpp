#include <Core/CorePCH.h>

#include <Core/Scripting/LuaWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

void ezLuaWrapper::RegisterCFunction(const char* szFunctionName, lua_CFunction function, void* pLightUserData) const
{
  lua_pushlightuserdata(m_pState, pLightUserData);
  lua_pushcclosure(m_pState, function, 1);
  lua_setglobal(m_pState, szFunctionName);
}

void* ezLuaWrapper::GetFunctionLightUserData() const
{
  return lua_touserdata(m_pState, lua_upvalueindex(1));
}

bool ezLuaWrapper::PrepareFunctionCall(const char* szFunctionName)
{
  EZ_ASSERT_DEV(m_States.m_iLuaReturnValues == 0,
    "ezLuaWrapper::PrepareFunctionCall: You didn't discard the return-values of the previous script call. {0} Return-values "
    "were expected.",
    m_States.m_iLuaReturnValues);

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

ezResult ezLuaWrapper::CallPreparedFunction(ezUInt32 uiExpectedReturnValues, ezLogInterface* pLogInterface)
{
  m_States.m_iLuaReturnValues = uiExpectedReturnValues;

  // save the current states on a cheap stack
  const ezScriptStates StackedStates = m_States;
  m_States = ezScriptStates();

  if (pLogInterface == nullptr)
    pLogInterface = ezLog::GetThreadLocalLogSystem();

  if (lua_pcall(m_pState, StackedStates.m_iParametersPushed, uiExpectedReturnValues, 0) != 0)
  {
    // restore the states to their previous values
    m_States = StackedStates;

    m_States.m_iLuaReturnValues = 0;

    ezLog::Error(pLogInterface, "Script-function Call: {0}", lua_tostring(m_pState, -1));

    lua_pop(m_pState, 1); /* pop error message from the stack */
    return EZ_FAILURE;
  }

  // before resetting the state, make sure the returned state has no stuff left
  EZ_ASSERT_DEV((m_States.m_iLuaReturnValues == 0) && (m_States.m_iOpenTables == 0),
    "After ezLuaWrapper::CallPreparedFunction: Return values: {0}, Open Tables: {1}", m_States.m_iLuaReturnValues, m_States.m_iOpenTables);

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

bool ezLuaWrapper::IsReturnValueInt(ezUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TNUMBER);
}

bool ezLuaWrapper::IsReturnValueBool(ezUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TBOOLEAN);
}

bool ezLuaWrapper::IsReturnValueFloat(ezUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TNUMBER);
}

bool ezLuaWrapper::IsReturnValueString(ezUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TSTRING);
}

bool ezLuaWrapper::IsReturnValueNil(ezUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TNIL);
}

ezInt32 ezLuaWrapper::GetIntReturnValue(ezUInt32 uiReturnValue) const
{
  return ((int)(lua_tointeger(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1)));
}

bool ezLuaWrapper::GetBoolReturnValue(ezUInt32 uiReturnValue) const
{
  return (lua_toboolean(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) != 0);
}

float ezLuaWrapper::GetFloatReturnValue(ezUInt32 uiReturnValue) const
{
  return ((float)(lua_tonumber(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1)));
}

const char* ezLuaWrapper::GetStringReturnValue(ezUInt32 uiReturnValue) const
{
  return (lua_tostring(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1));
}


#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT


