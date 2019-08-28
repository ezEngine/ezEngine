#include <CorePCH.h>

#include <Core/Scripting/DuktapeWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Duktape/duktape.h>

ezDuktapeWrapper::ezDuktapeWrapper(const char* szWrapperName)
  : m_Allocator(szWrapperName, ezFoundation::GetDefaultAllocator())
{
  m_sWrapperName = szWrapperName;

  InitializeContext();
}

ezDuktapeWrapper::~ezDuktapeWrapper()
{
  DestroyContext();
}

duk_context* ezDuktapeWrapper::GetContext()
{
  return m_pContext;
}

const duk_context* ezDuktapeWrapper::GetContext() const
{
  return m_pContext;
}

ezResult ezDuktapeWrapper::ExecuteString(const char* szString)
{
  if (duk_peval_string(m_pContext, szString) == 0)
  {
    duk_pop(m_pContext);
    return EZ_SUCCESS;
  }

  // TODO: distinguish between compile and eval errors
  // duk_pcompile_string / duk_pcompile_string_filename
  // duk_pcall

  {
    EZ_LOG_BLOCK("ezDuktapeWrapper::ExecuteString");

    ezLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1));
    ezLog::Info("[duktape]Script: {0}", szString);
  }

  return EZ_FAILURE;
}

void ezDuktapeWrapper::RegisterFunction(const char* szFunctionName, duk_c_function pFunction, ezUInt8 uiNumArguments)
{
  duk_push_global_object(m_pContext);
  /*const int iFuncIdx =*/duk_push_c_function(m_pContext, pFunction, uiNumArguments);
  // TODO: could store iFuncIdx for faster function calls
  EZ_VERIFY(duk_put_prop_string(m_pContext, -2, szFunctionName), "Fail to register C function");
  duk_pop(m_pContext);
}

bool ezDuktapeWrapper::PrepareFunctionCall(const char* szFunctionName)
{
  if (!duk_get_global_string(m_pContext, szFunctionName))
    return false;

  if (!duk_is_function(m_pContext, -1))
    return false;

  m_States.m_iPushedFunctionArguments = 0;

  return true;
}

ezResult ezDuktapeWrapper::CallPreparedFunction()
{
  if (duk_pcall(m_pContext, m_States.m_iPushedFunctionArguments) == DUK_EXEC_SUCCESS)
  {
    // TODO: leave return value on stack for inspection
    duk_pop(m_pContext);
    return EZ_SUCCESS;
  }
  else
  {
    // TODO: could also create a stack trace using duk_is_error + duk_get_prop_string(ctx, -1, "stack");
    ezLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1));
    duk_pop(m_pContext);
    return EZ_FAILURE;
  }
}

void ezDuktapeWrapper::PushParameter(ezInt32 iParam)
{
  duk_push_int(m_pContext, iParam);
  m_States.m_iPushedFunctionArguments++;
}

void ezDuktapeWrapper::PushParameter(bool bParam)
{
  duk_push_boolean(m_pContext, bParam);
  m_States.m_iPushedFunctionArguments++;
}

void ezDuktapeWrapper::PushParameter(double fParam)
{
  duk_push_number(m_pContext, fParam);
  m_States.m_iPushedFunctionArguments++;
}

void ezDuktapeWrapper::PushParameter(const char* szParam)
{
  duk_push_string(m_pContext, szParam);
  m_States.m_iPushedFunctionArguments++;
}

void ezDuktapeWrapper::PushParameter(const char* szParam, ezUInt32 length)
{
  duk_push_lstring(m_pContext, szParam, length);
  m_States.m_iPushedFunctionArguments++;
}

void ezDuktapeWrapper::PushParameterNull()
{
  duk_push_null(m_pContext);
  m_States.m_iPushedFunctionArguments++;
}

void ezDuktapeWrapper::PushParameterUndefined()
{
  duk_push_undefined(m_pContext);
  m_States.m_iPushedFunctionArguments++;
}

void ezDuktapeWrapper::InitializeContext()
{
  m_pContext = duk_create_heap(DukAlloc, DukRealloc, DukFree, this, FatalErrorHandler);

  EZ_ASSERT_ALWAYS(m_pContext != nullptr, "Duktape context could not be created");
}

void ezDuktapeWrapper::DestroyContext()
{
  duk_destroy_heap(m_pContext);
  m_pContext = nullptr;

  const auto stats = m_Allocator.GetStats();
  EZ_ASSERT_DEBUG(stats.m_uiAllocationSize == 0, "Duktape did not free all data");
  EZ_ASSERT_DEBUG(stats.m_uiNumAllocations == stats.m_uiNumDeallocations, "Duktape did not free all data");
}

void ezDuktapeWrapper::FatalErrorHandler(void* pUserData, const char* szMsg)
{
  ezDuktapeWrapper* pDukWrapper = reinterpret_cast<ezDuktapeWrapper*>(pUserData);

  EZ_REPORT_FAILURE("{}: {}", pDukWrapper->m_sWrapperName, szMsg);
}

void* ezDuktapeWrapper::DukAlloc(void* pUserData, size_t size)
{
  EZ_ASSERT_DEBUG(size > 0, "Invalid allocation");

  ezDuktapeWrapper* pDukWrapper = reinterpret_cast<ezDuktapeWrapper*>(pUserData);

  return pDukWrapper->m_Allocator.Allocate(size, 8);
}

void* ezDuktapeWrapper::DukRealloc(void* pUserData, void* pPointer, size_t size)
{
  ezDuktapeWrapper* pDukWrapper = reinterpret_cast<ezDuktapeWrapper*>(pUserData);

  if (size == 0)
  {
    if (pPointer != nullptr)
    {
      pDukWrapper->m_Allocator.Deallocate(pPointer);
    }

    return nullptr;
  }

  if (pPointer == nullptr)
  {
    return pDukWrapper->m_Allocator.Allocate(size, 8);
  }
  else
  {
    return pDukWrapper->m_Allocator.Reallocate(pPointer, pDukWrapper->m_Allocator.AllocatedSize(pPointer), size, 8);
  }
}

void ezDuktapeWrapper::DukFree(void* pUserData, void* pPointer)
{
  if (pPointer == nullptr)
    return;

  ezDuktapeWrapper* pDukWrapper = reinterpret_cast<ezDuktapeWrapper*>(pUserData);

  pDukWrapper->m_Allocator.Deallocate(pPointer);
}

#endif
