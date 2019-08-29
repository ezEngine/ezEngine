#include <CorePCH.h>

#include <Core/Scripting/DuktapeWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Duktape/duktape.h>
#  include <Foundation/IO/FileSystem/FileReader.h>

EZ_CHECK_AT_COMPILETIME(ezDuktapeTypeMask::None == DUK_TYPE_MASK_NONE);
EZ_CHECK_AT_COMPILETIME(ezDuktapeTypeMask::Undefined == DUK_TYPE_MASK_UNDEFINED);
EZ_CHECK_AT_COMPILETIME(ezDuktapeTypeMask::Null == DUK_TYPE_MASK_NULL);
EZ_CHECK_AT_COMPILETIME(ezDuktapeTypeMask::Bool == DUK_TYPE_MASK_BOOLEAN);
EZ_CHECK_AT_COMPILETIME(ezDuktapeTypeMask::Number == DUK_TYPE_MASK_NUMBER);
EZ_CHECK_AT_COMPILETIME(ezDuktapeTypeMask::String == DUK_TYPE_MASK_STRING);
EZ_CHECK_AT_COMPILETIME(ezDuktapeTypeMask::Object == DUK_TYPE_MASK_OBJECT);

ezDuktapeWrapper::ezDuktapeWrapper(const char* szWrapperName)
  : m_Allocator(szWrapperName, ezFoundation::GetDefaultAllocator())
{
  InitializeContext();
}

ezDuktapeWrapper::ezDuktapeWrapper(duk_context* pExistingContext)
  : m_Allocator("", nullptr)
{
  EZ_ASSERT_ALWAYS(pExistingContext != nullptr, "Duktape context must not be null");

  m_pContext = pExistingContext;
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
    EZ_LOG_BLOCK("ezDuktapeWrapper::ExecuteString", "Compilation failed");

    ezLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1));
    // TODO: print out line by line
    ezLog::Info("[duktape]Source: {0}", szString);

    duk_pop(m_pContext);
    return EZ_FAILURE;
  }


  if (duk_pcall(m_pContext, 0) != DUK_EXEC_SUCCESS)
  {
    EZ_LOG_BLOCK("ezDuktapeWrapper::ExecuteString", "Execution failed");

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

void ezDuktapeWrapper::RegisterFunction(const char* szFunctionName, duk_c_function pFunction, ezUInt8 uiNumArguments, ezInt16 iMagicValue /*= 0*/)
{
  duk_push_global_object(m_pContext);
  /*const int iFuncIdx =*/duk_push_c_function(m_pContext, pFunction, uiNumArguments);
  duk_set_magic(m_pContext, -1, iMagicValue);
  // TODO: could store iFuncIdx for faster function calls
  EZ_VERIFY(duk_put_prop_string(m_pContext, -2, szFunctionName), "Fail to register C function");
  duk_pop(m_pContext);
}

void ezDuktapeWrapper::RegisterFunctionWithVarArgs(const char* szFunctionName, duk_c_function pFunction, ezInt16 iMagicValue /*= 0*/)
{
  duk_push_global_object(m_pContext);
  /*const int iFuncIdx =*/duk_push_c_function(m_pContext, pFunction, DUK_VARARGS);
  duk_set_magic(m_pContext, -1, iMagicValue);
  // TODO: could store iFuncIdx for faster function calls
  EZ_VERIFY(duk_put_prop_string(m_pContext, -2, szFunctionName), "Fail to register C function");
  duk_pop(m_pContext);
}

ezResult ezDuktapeWrapper::BeginFunctionCall(const char* szFunctionName)
{
  EZ_ASSERT_DEV(m_bIsInFunctionCall == false, "Function calls cannot be nested and must be finished with EndFunctionCall()");

  if (m_States.m_iOpenObjects == 0)
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

  m_States.m_iPushedFunctionArguments = 0;

  m_bIsInFunctionCall = true;
  return EZ_SUCCESS;

failure:
  duk_pop(m_pContext);
  return EZ_FAILURE;
}

ezResult ezDuktapeWrapper::ExecuteFunctionCall()
{
  EZ_ASSERT_DEV(m_bIsInFunctionCall == true, "Function calls must be successfully initiated with BeginFunctionCall()");

  if (duk_pcall(m_pContext, m_States.m_iPushedFunctionArguments) == DUK_EXEC_SUCCESS)
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

bool ezDuktapeWrapper::GetBoolReturnValue(bool fallback /*= false*/) const
{
  return duk_get_boolean_default(GetContext(), -1, fallback);
}

ezInt32 ezDuktapeWrapper::GetIntReturnValue(ezInt32 fallback /*= 0*/) const
{
  return duk_get_int_default(GetContext(), -1, fallback);
}

float ezDuktapeWrapper::GetFloatReturnValue(float fallback /*= 0*/) const
{
  return static_cast<float>(duk_get_number_default(GetContext(), -1, fallback));
}

double ezDuktapeWrapper::GetNumberReturnValue(double fallback /*= 0*/) const
{
  return duk_get_number_default(GetContext(), -1, fallback);
}

const char* ezDuktapeWrapper::GetStringReturnValue(const char* fallback /*= ""*/) const
{
  return duk_get_string_default(GetContext(), -1, fallback);
}

bool ezDuktapeWrapper::IsReturnValueOfType(ezBitflags<ezDuktapeTypeMask> mask) const
{
  return duk_check_type_mask(GetContext(), -1, mask.GetValue());
}

bool ezDuktapeWrapper::IsReturnValueBool() const
{
  return duk_check_type_mask(GetContext(), -1, DUK_TYPE_MASK_BOOLEAN);
}

bool ezDuktapeWrapper::IsReturnValueNumber() const
{
  return duk_check_type_mask(GetContext(), -1, DUK_TYPE_MASK_NUMBER);
}

bool ezDuktapeWrapper::IsReturnValueString() const
{
  return duk_check_type_mask(GetContext(), -1, DUK_TYPE_MASK_STRING);
}

bool ezDuktapeWrapper::IsReturnValueNull() const
{
  return duk_check_type_mask(GetContext(), -1, DUK_TYPE_MASK_NULL);
}

bool ezDuktapeWrapper::IsReturnValueUndefined() const
{
  return duk_check_type_mask(GetContext(), -1, DUK_TYPE_MASK_UNDEFINED);
}

bool ezDuktapeWrapper::IsReturnValueObject() const
{
  return duk_check_type_mask(GetContext(), -1, DUK_TYPE_MASK_OBJECT);
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
  ezDuktapeWrapper* pDukWrapper = reinterpret_cast<ezDuktapeWrapper*>(pUserData);

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
    duk_push_global_object(m_pContext);
  }

  m_States.m_iOpenObjects++;

  if (duk_get_prop_string(m_pContext, -1, szObjectName) == false)
  {
    CloseObject();
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezDuktapeWrapper::CloseObject()
{
  EZ_ASSERT_DEBUG(m_States.m_iOpenObjects > 0, "No objects open at this time");

  duk_pop(m_pContext);
  m_States.m_iOpenObjects--;

  if (m_States.m_iOpenObjects == 0)
  {
    // also pop the global object
    duk_pop(m_pContext);
  }
}

bool ezDuktapeWrapper::HasProperty(const char* szPropertyName)
{
  return duk_has_prop_string(m_pContext, -1, szPropertyName);
}

#endif

