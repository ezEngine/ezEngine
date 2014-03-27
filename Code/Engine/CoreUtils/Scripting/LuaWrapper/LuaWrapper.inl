#pragma once

inline lua_State* ezLuaWrapper::GetLuaState()
{
  return m_pState;
}

inline ezInt32 ezLuaWrapper::ReturnToScript() const 
{
  return (m_States.m_iParametersPushed);
}

inline ezUInt32 ezLuaWrapper::GetNumberOfFunctionParameters() const
{
  return ((int) lua_gettop(m_pState));
}

inline bool ezLuaWrapper::IsParameterBool(ezUInt32 iParameter) const
{
  return (lua_type(m_pState, iParameter + s_ParamOffset) == LUA_TBOOLEAN);
}

inline bool ezLuaWrapper::IsParameterFloat(ezUInt32 iParameter) const
{
  return (lua_type(m_pState, iParameter + s_ParamOffset) == LUA_TNUMBER);
}

inline bool ezLuaWrapper::IsParameterInt(ezUInt32 iParameter) const
{
  return (lua_type(m_pState, iParameter + s_ParamOffset) == LUA_TNUMBER);
}

inline bool ezLuaWrapper::IsParameterString(ezUInt32 iParameter) const
{
  return (lua_type(m_pState, iParameter + s_ParamOffset) == LUA_TSTRING);
}

inline bool ezLuaWrapper::IsParameterNil(ezUInt32 iParameter) const
{
  return (lua_type(m_pState, iParameter + s_ParamOffset) == LUA_TNIL);
}

inline bool ezLuaWrapper::IsParameterTable(ezUInt32 iParameter) const
{
  return (lua_type(m_pState, iParameter + s_ParamOffset) == LUA_TTABLE);
}

inline void ezLuaWrapper::PushParameter (ezInt32 iParameter)
{
  lua_pushinteger(m_pState, iParameter);
  m_States.m_iParametersPushed++;
}

inline void ezLuaWrapper::PushParameter (bool bParameter)
{
  lua_pushboolean(m_pState, bParameter);
  m_States.m_iParametersPushed++;
}

inline void ezLuaWrapper::PushParameter (float fParameter)
{
  lua_pushnumber(m_pState, fParameter);
  m_States.m_iParametersPushed++;
}

inline void ezLuaWrapper::PushParameter (const char* szParameter)
{
  lua_pushstring(m_pState, szParameter);
  m_States.m_iParametersPushed++;
}

inline void ezLuaWrapper::PushParameter (const char* szParameter, ezUInt32 length)
{
  lua_pushlstring(m_pState, szParameter, length);
  m_States.m_iParametersPushed++;
}

inline void ezLuaWrapper::PushParameterNil()
{
  lua_pushnil(m_pState);
  m_States.m_iParametersPushed++;
}

inline void ezLuaWrapper::PushReturnValue(ezInt32 iParameter)
{
  lua_pushinteger(m_pState, iParameter);
  m_States.m_iParametersPushed++;
}

inline void ezLuaWrapper::PushReturnValue(bool bParameter)
{
  lua_pushboolean(m_pState, bParameter);
  m_States.m_iParametersPushed++;
}

inline void ezLuaWrapper::PushReturnValue(float fParameter)
{
  lua_pushnumber(m_pState, fParameter);
  m_States.m_iParametersPushed++;
}

inline void ezLuaWrapper::PushReturnValue(const char* szParameter)
{
  lua_pushstring(m_pState, szParameter);
  m_States.m_iParametersPushed++;
}

inline void ezLuaWrapper::PushReturnValue(const char* szParameter, ezUInt32 length)
{
  lua_pushlstring(m_pState, szParameter, length);
  m_States.m_iParametersPushed++;
}

inline void ezLuaWrapper::PushReturnValueNil()
{
  lua_pushnil(m_pState);
  m_States.m_iParametersPushed++;
}

inline void ezLuaWrapper::SetVariableNil(const char* szName) const
{
  lua_pushnil(m_pState);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void ezLuaWrapper::SetVariable(const char* szName, ezInt32 iValue) const
{
  lua_pushinteger(m_pState, iValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void ezLuaWrapper::SetVariable(const char* szName, float fValue) const
{
  lua_pushnumber(m_pState, fValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void ezLuaWrapper::SetVariable(const char* szName, bool bValue) const
{
  lua_pushboolean(m_pState, bValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void ezLuaWrapper::SetVariable(const char* szName, const char* szValue) const
{
  lua_pushstring(m_pState, szValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void ezLuaWrapper::SetVariable(const char* szName, const char* szValue, ezUInt32 len) const
{
  lua_pushlstring(m_pState, szValue, len);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void ezLuaWrapper::PushTable(const char* szTableName, bool bGlobalTable)
{
  if (bGlobalTable || m_States.m_iOpenTables == 0)
    lua_getglobal(m_pState, szTableName);
  else
  {
    lua_pushstring(m_pState, szTableName);
    lua_gettable(m_pState, -2);
  }

  m_States.m_iParametersPushed++;
}

inline int ezLuaWrapper::GetIntParameter (ezUInt32 iParameter) const 
{
  return ((int) (lua_tointeger(m_pState, iParameter + s_ParamOffset)));
}

inline bool ezLuaWrapper::GetBoolParameter (ezUInt32 iParameter) const
{
  return (lua_toboolean(m_pState, iParameter + s_ParamOffset) != 0);
}

inline float ezLuaWrapper::GetFloatParameter (ezUInt32 iParameter) const
{
  return ((float) (lua_tonumber(m_pState, iParameter + s_ParamOffset)));
}

inline const char* ezLuaWrapper::GetStringParameter (ezUInt32 iParameter) const 
{
  return (lua_tostring(m_pState, iParameter + s_ParamOffset));
}

