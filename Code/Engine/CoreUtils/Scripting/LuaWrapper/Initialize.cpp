#include <CoreUtils/PCH.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <CoreUtils/Scripting/LuaWrapper.h>
#include <Foundation/Logging/Log.h>

ezLuaWrapper::ezLuaWrapper()
{
  m_bReleaseOnExit = true;
  m_pState = nullptr;

  Clear();
}

ezLuaWrapper::ezLuaWrapper(lua_State* s)
{
  m_pState = s;
  m_bReleaseOnExit = false;
}

ezLuaWrapper::~ezLuaWrapper()
{
  if (m_bReleaseOnExit)
    lua_close(m_pState);
}

void ezLuaWrapper::Clear()
{
  EZ_ASSERT_DEV(m_bReleaseOnExit, "Cannot clear a script that did not create the Lua state itself.");

  if (m_pState)
    lua_close(m_pState);

  m_pState = lua_newstate(lua_allocator, nullptr);

  luaL_openlibs(m_pState);
}

ezResult ezLuaWrapper::ExecuteString(const char* szString, const char* szDebugChunkName, ezLogInterface* pLogInterface) const
{
  EZ_ASSERT_DEV(m_States.m_iLuaReturnValues == 0, "ezLuaWrapper::ExecuteString: You didn't discard the return-values of the previous script call. %d Return-values were expected.", m_States.m_iLuaReturnValues);

  if (!pLogInterface)
    pLogInterface = ezGlobalLog::GetInstance();

  int error = luaL_loadbuffer(m_pState, szString, ezStringUtils::GetStringElementCount(szString), szDebugChunkName);
        
  if (error != LUA_OK)
  {
    EZ_LOG_BLOCK("ezLuaWrapper::ExecuteString");

    ezLog::Error(pLogInterface, "[lua]Lua compile error: %s", lua_tostring(m_pState, -1));
    ezLog::Info("[luascript]Script: %s", szString);

    return EZ_FAILURE;
  }

  error = lua_pcall(m_pState, 0, 0, 0);

  if (error != LUA_OK)
  {
    EZ_LOG_BLOCK("ezLuaWrapper::ExecuteString");

    ezLog::Error(pLogInterface, "[lua]Lua error: %s", lua_tostring(m_pState, -1));
    ezLog::Info("[luascript]Script: %s", szString);

    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void* ezLuaWrapper::lua_allocator(void* ud, void* ptr, size_t osize, size_t nsize)
{
  /// \todo Create optimized allocator.

  if (nsize == 0)
  {
    delete[] (ezUInt8*) ptr;
    return (nullptr);
  }

  ezUInt8* ucPtr = new ezUInt8[nsize];

  if (ptr != nullptr)
  {
    ezMemoryUtils::Copy(ucPtr, (ezUInt8*) ptr, ezUInt32 (osize < nsize ? osize : nsize));

    delete[] (ezUInt8*) ptr;
  }

  return ((void*) ucPtr);
}




EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Scripting_LuaWrapper_Initialize);

