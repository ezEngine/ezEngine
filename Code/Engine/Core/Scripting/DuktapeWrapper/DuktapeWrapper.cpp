#include <CorePCH.h>

#include <Core/Scripting/DuktapeWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Duktape/duk_module_duktape.h>
#  include <Duktape/duktape.h>
#  include <Foundation/IO/FileSystem/FileReader.h>



ezDuktapeWrapper::ezDuktapeWrapper(const char* szWrapperName)
  : ezDuktapeHelper(nullptr)
  , m_Allocator(szWrapperName, ezFoundation::GetDefaultAllocator())

{
  InitializeContext();
}

ezDuktapeWrapper::ezDuktapeWrapper(duk_context* pExistingContext)
  : ezDuktapeHelper(pExistingContext)
  , m_Allocator("", nullptr)
{
  EZ_ASSERT_ALWAYS(pExistingContext != nullptr, "Duktape context must not be null");

  m_bReleaseOnExit = false;
}

ezDuktapeWrapper::~ezDuktapeWrapper()
{
  DestroyContext();
}

ezResult ezDuktapeWrapper::ExecuteString(const char* szString, const char* szDebugName /*= "eval"*/)
{
  duk_push_string(m_pContext, szDebugName);
  if (duk_pcompile_string_filename(m_pContext, 0, szString) != 0)
  {
    EZ_LOG_BLOCK("DukTape::ExecuteString", "Compilation failed");

    ezLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1));
    // TODO: print out line by line
    ezLog::Info("[duktape]Source: {0}", szString);

    duk_pop(m_pContext);
    return EZ_FAILURE;
  }


  if (duk_pcall(m_pContext, 0) != DUK_EXEC_SUCCESS)
  {
    EZ_LOG_BLOCK("DukTape::ExecuteString", "Execution failed");

    ezLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1));
    // TODO: print out line by line
    ezLog::Info("[duktape]Source: {0}", szString);

    duk_pop(m_pContext);
    return EZ_FAILURE;
  }

  duk_pop(m_pContext);
  return EZ_SUCCESS;
}

ezResult ezDuktapeWrapper::ExecuteStream(ezStreamReader& stream, const char* szDebugName)
{
  ezStringBuilder source;
  source.ReadAll(stream);

  return ExecuteString(source, szDebugName);
}

ezResult ezDuktapeWrapper::ExecuteFile(const char* szFile)
{
  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  return ExecuteStream(file, szFile);
}

void ezDuktapeWrapper::EnableModuleSupport(duk_c_function pModuleSearchFunction)
{
  if (!m_bInitializedModuleSupport)
  {
    // we need an 'exports' object for the transpiled TypeScript files to load
    // (they access this object to define the '__esModule' property on it)
    ExecuteString("var exports = {}");

    m_bInitializedModuleSupport = true;
    duk_module_duktape_init(m_pContext);
  }

  if (pModuleSearchFunction)
  {
    duk_get_global_string(m_pContext, "Duktape");
    duk_push_c_function(m_pContext, pModuleSearchFunction, 4);
    duk_put_prop_string(m_pContext, -2, "modSearch");
    duk_pop(m_pContext);
  }
}

ezResult ezDuktapeWrapper::BeginFunctionCall(const char* szFunctionName, bool bForceLocalObject /*= false*/)
{
  EZ_ASSERT_DEV(m_bIsInFunctionCall == false, "Function calls cannot be nested and must be finished with EndFunctionCall()");

  if (!bForceLocalObject && m_States.m_iOpenObjects == 0)
  {
    if (!duk_get_global_string(m_pContext, szFunctionName))
      goto failure;
  }
  else
  {
    if (!duk_get_prop_string(m_pContext, -1, szFunctionName))
      goto failure;
  }

  if (!duk_is_function(m_pContext, -1))
    goto failure;

  m_iPushedValues = 0;

  m_bIsInFunctionCall = true;
  return EZ_SUCCESS;

failure:
  duk_pop(m_pContext);
  return EZ_FAILURE;
}

ezResult ezDuktapeWrapper::BeginMethodCall(const char* szMethodName)
{
  EZ_ASSERT_DEV(m_bIsInFunctionCall == false, "Function calls cannot be nested and must be finished with EndFunctionCall()");

  if (!duk_get_prop_string(m_pContext, -1, szMethodName))
    return EZ_FAILURE;

  if (!duk_is_function(m_pContext, -1))
    goto failure;

  // assume 'this' is the top stack element
  duk_dup(m_pContext, -2);

  m_iPushedValues = 0;

  m_bIsInFunctionCall = true;
  return EZ_SUCCESS;

failure:
  duk_pop(m_pContext);
  return EZ_FAILURE;
}

ezResult ezDuktapeWrapper::ExecuteFunctionCall()
{
  EZ_ASSERT_DEV(m_bIsInFunctionCall == true, "Function calls must be successfully initiated with BeginFunctionCall()");

  if (duk_pcall(m_pContext, m_iPushedValues) == DUK_EXEC_SUCCESS)
  {
    // leaves return value on stack
    return EZ_SUCCESS;
  }
  else
  {
    // TODO: could also create a stack trace using duk_is_error + duk_get_prop_string(ctx, -1, "stack");
    ezLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1));

    return EZ_FAILURE;
  }
}

ezResult ezDuktapeWrapper::ExecuteMethodCall()
{
  EZ_ASSERT_DEV(m_bIsInFunctionCall == true, "Function calls must be successfully initiated with BeginFunctionCall()");

  if (duk_pcall_method(m_pContext, m_iPushedValues) == DUK_EXEC_SUCCESS)
  {
    // leaves return value on stack
    return EZ_SUCCESS;
  }
  else
  {
    // TODO: could also create a stack trace using duk_is_error + duk_get_prop_string(ctx, -1, "stack");
    ezLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1));

    return EZ_FAILURE;
  }
}

void ezDuktapeWrapper::EndFunctionCall()
{
  EZ_ASSERT_DEV(m_bIsInFunctionCall == true, "Function calls must be successfully initiated with BeginFunctionCall()");
  m_bIsInFunctionCall = false;

  // pop the return value / error object from the stack
  duk_pop(m_pContext);
}

void ezDuktapeWrapper::EndMethodCall()
{
  EZ_ASSERT_DEV(m_bIsInFunctionCall == true, "Function calls must be successfully initiated with BeginFunctionCall()");
  m_bIsInFunctionCall = false;

  // pop the return value / error object from the stack
  duk_pop(m_pContext);
}

void ezDuktapeWrapper::InitializeContext()
{
  EZ_ASSERT_ALWAYS(m_pContext == nullptr, "Duktape context should be null");

  m_bReleaseOnExit = true;

  m_pContext = duk_create_heap(DukAlloc, DukRealloc, DukFree, this, FatalErrorHandler);

  EZ_ASSERT_ALWAYS(m_pContext != nullptr, "Duktape context could not be created");
}

void ezDuktapeWrapper::DestroyContext()
{
  if (!m_bReleaseOnExit)
    return;

  duk_destroy_heap(m_pContext);
  m_pContext = nullptr;

  const auto stats = m_Allocator.GetStats();
  EZ_ASSERT_DEBUG(stats.m_uiAllocationSize == 0, "Duktape did not free all data");
  EZ_ASSERT_DEBUG(stats.m_uiNumAllocations == stats.m_uiNumDeallocations, "Duktape did not free all data");
}

void ezDuktapeWrapper::FatalErrorHandler(void* pUserData, const char* szMsg)
{
  EZ_REPORT_FAILURE(szMsg);
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

ezResult ezDuktapeWrapper::OpenObject(const char* szObjectName)
{
  if (m_States.m_iOpenObjects == 0)
  {
    EZ_ASSERT_DEV(m_States.m_bAutoOpenedGlobalObject == false, "Global Object is already open");

    OpenGlobalObject();
    m_States.m_bAutoOpenedGlobalObject = true;
  }

  m_States.m_iOpenObjects++;

  if (duk_get_prop_string(m_pContext, -1, szObjectName) == false)
  {
    CloseObject();
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezDuktapeWrapper::OpenGlobalObject()
{
  duk_push_global_object(m_pContext);
  m_States.m_iOpenObjects++;
}

void ezDuktapeWrapper::OpenGlobalStashObject()
{
  duk_push_global_stash(m_pContext);
  m_States.m_iOpenObjects++;
}

void ezDuktapeWrapper::CloseObject()
{
  EZ_ASSERT_DEBUG(m_States.m_iOpenObjects > 0, "No objects open at this time");

  duk_pop(m_pContext);
  m_States.m_iOpenObjects--;

  if (m_States.m_iOpenObjects == 1 && m_States.m_bAutoOpenedGlobalObject)
  {
    // also pop the global object
    m_States.m_bAutoOpenedGlobalObject = false;
    CloseObject();
  }
}

ezDuktapeStackValidator::ezDuktapeStackValidator(duk_context* pContext, ezInt32 iExpectedChange /*= 0*/)
{
  m_pContext = pContext;
  m_iStackTop = duk_get_top(m_pContext) + iExpectedChange;
}

ezDuktapeStackValidator::~ezDuktapeStackValidator()
{
  const int iCurTop = duk_get_top(m_pContext);
  EZ_ASSERT_DEBUG(iCurTop == m_iStackTop, "Stack top is not as expected");
}

void ezDuktapeStackValidator::AdjustExpected(ezInt32 iChange)
{
  m_iStackTop += iChange;
}

#endif
