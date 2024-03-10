#include <Core/CorePCH.h>

#include <Core/Scripting/LuaWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

bool ezLuaWrapper::IsVariableAvailable(const char* szName) const
{
  if (m_States.m_iOpenTables == 0)
    lua_getglobal(m_pState, szName);
  else
  {
    lua_pushstring(m_pState, szName);
    lua_gettable(m_pState, -2);
  }

  bool bRet = !lua_isnil(m_pState, -1);

  lua_pop(m_pState, 1);
  return (bRet);
}

bool ezLuaWrapper::IsFunctionAvailable(const char* szFunction) const
{
  if (m_States.m_iOpenTables == 0)
    lua_getglobal(m_pState, szFunction);
  else
  {
    lua_pushstring(m_pState, szFunction);
    lua_gettable(m_pState, -2);
  }

  bool bRet = (lua_isfunction(m_pState, -1) != 0);

  lua_pop(m_pState, 1);

  return (bRet);
}



ezInt32 ezLuaWrapper::GetIntVariable(const char* szName, ezInt32 iDefault) const
{
  if (m_States.m_iOpenTables == 0)
    lua_getglobal(m_pState, szName);
  else
  {
    lua_pushstring(m_pState, szName);
    lua_gettable(m_pState, -2);
  }

  int ret = iDefault;

  if (lua_isnumber(m_pState, -1) != 0)
    ret = (int)lua_tonumber(m_pState, -1);

  lua_pop(m_pState, 1);

  return ret;
}

bool ezLuaWrapper::GetBoolVariable(const char* szName, bool bDefault) const
{
  if (m_States.m_iOpenTables == 0)
    lua_getglobal(m_pState, szName);
  else
  {
    lua_pushstring(m_pState, szName);
    lua_gettable(m_pState, -2);
  }

  bool ret = bDefault;

  if (!!lua_isboolean(m_pState, -1))
    ret = (lua_toboolean(m_pState, -1) != 0);

  lua_pop(m_pState, 1);

  return ret;
}

float ezLuaWrapper::GetFloatVariable(const char* szName, float fDefault) const
{
  if (m_States.m_iOpenTables == 0)
    lua_getglobal(m_pState, szName);
  else
  {
    lua_pushstring(m_pState, szName);
    lua_gettable(m_pState, -2);
  }

  float ret = fDefault;

  if (lua_isnumber(m_pState, -1) != 0)
    ret = (float)lua_tonumber(m_pState, -1);

  lua_pop(m_pState, 1);

  return ret;
}

const char* ezLuaWrapper::GetStringVariable(const char* szName, const char* szDefault) const
{
  if (m_States.m_iOpenTables == 0)
    lua_getglobal(m_pState, szName);
  else
  {
    lua_pushstring(m_pState, szName);
    lua_gettable(m_pState, -2);
  }

  const char* ret = szDefault;

  // non strict conversion
  // if (lua_isstring(m_pState, -1) != 0)
  //  ret = lua_tostring(m_pState, -1);

  if (lua_type(m_pState, -1) == LUA_TSTRING) // don't want to convert numbers to strings, so be strict about it
    ret = (lua_tostring(m_pState, -1));

  // pop the variable and if necessary also the global table from the stack
  lua_pop(m_pState, 1);

  return ret;
}

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT
