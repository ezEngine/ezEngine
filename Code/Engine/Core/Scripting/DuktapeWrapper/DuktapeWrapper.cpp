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
  m_sWrapperName = szWrapperName;

  InitializeContext();
}

ezDuktapeWrapper::ezDuktapeWrapper(duk_context* pExistingContext)
  : m_Allocator("", ezFoundation::GetDefaultAllocator())
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

bool ezDuktapeWrapper::PrepareFunctionCall(const char* szFunctionName)
{
  if (m_States.m_iOpenObjects == 0)
  {
    if (!duk_get_global_string(m_pContext, szFunctionName))
      return false;
  }
  else
  {
    // TODO
  }

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

    // TODO
    //if (m_States.m_iOpenObjects == 0)
    //{
    //  duk_pop(m_pContext);
    //}

    return EZ_SUCCESS;
  }
  else
  {
    // TODO: could also create a stack trace using duk_is_error + duk_get_prop_string(ctx, -1, "stack");
    ezLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1));
    duk_pop(m_pContext);

    // TODO
    //if (m_States.m_iOpenObjects == 0)
    //{
    //  duk_pop(m_pContext);
    //}

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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezDuktapeFunction::ezDuktapeFunction(duk_context* pExistingContext)
  : ezDuktapeWrapper(pExistingContext)
{
}


ezUInt32 ezDuktapeFunction::GetNumVarArgFunctionParameters() const
{
  return duk_get_top(GetContext());
}

ezInt16 ezDuktapeFunction::GetFunctionMagicValue() const
{
  return duk_get_current_magic(GetContext());
}

bool ezDuktapeFunction::GetBoolParameter(ezUInt32 uiArgIdx, bool fallback /*= false*/) const
{
  return duk_get_boolean_default(GetContext(), uiArgIdx, fallback);
}

ezInt32 ezDuktapeFunction::GetIntParameter(ezUInt32 uiArgIdx, ezInt32 fallback /*= 0*/) const
{
  return duk_get_int_default(GetContext(), uiArgIdx, fallback);
}

float ezDuktapeFunction::GetFloatParameter(ezUInt32 uiArgIdx, float fallback /*= 0*/) const
{
  return static_cast<float>(duk_get_number_default(GetContext(), uiArgIdx, fallback));
}

double ezDuktapeFunction::GetNumberParameter(ezUInt32 uiArgIdx, double fallback /*= 0*/) const
{
  return duk_get_number_default(GetContext(), uiArgIdx, fallback);
}

const char* ezDuktapeFunction::GetStringParameter(ezUInt32 uiArgIdx, const char* fallback /*= ""*/) const
{
  return duk_get_string_default(GetContext(), uiArgIdx, fallback);
}

bool ezDuktapeFunction::IsParameterOfType(ezUInt32 uiArgIdx, ezBitflags<ezDuktapeTypeMask> mask) const
{
  return duk_check_type_mask(GetContext(), uiArgIdx, mask.GetValue());
}

bool ezDuktapeFunction::IsParameterBool(ezUInt32 uiArgIdx) const
{
  return duk_check_type_mask(GetContext(), uiArgIdx, DUK_TYPE_MASK_BOOLEAN);
}

bool ezDuktapeFunction::IsParameterNumber(ezUInt32 uiArgIdx) const
{
  return duk_check_type_mask(GetContext(), uiArgIdx, DUK_TYPE_MASK_NUMBER);
}

bool ezDuktapeFunction::IsParameterString(ezUInt32 uiArgIdx) const
{
  return duk_check_type_mask(GetContext(), uiArgIdx, DUK_TYPE_MASK_STRING);
}

bool ezDuktapeFunction::IsParameterNull(ezUInt32 uiArgIdx) const
{
  return duk_check_type_mask(GetContext(), uiArgIdx, DUK_TYPE_MASK_NULL);
}

bool ezDuktapeFunction::IsParameterUndefined(ezUInt32 uiArgIdx) const
{
  return duk_check_type_mask(GetContext(), uiArgIdx, DUK_TYPE_MASK_UNDEFINED);
}

bool ezDuktapeFunction::IsParameterObject(ezUInt32 uiArgIdx) const
{
  return duk_check_type_mask(GetContext(), uiArgIdx, DUK_TYPE_MASK_OBJECT);
}

ezInt32 ezDuktapeFunction::ReturnVoid()
{
  return 0;
}

ezInt32 ezDuktapeFunction::ReturnNull()
{
  duk_push_null(GetContext());
  return 1;
}

ezInt32 ezDuktapeFunction::ReturnUndefined()
{
  duk_push_undefined(GetContext());
  return 1;
}

ezInt32 ezDuktapeFunction::ReturnBool(bool value)
{
  duk_push_boolean(GetContext(), value);
  return 1;
}

ezInt32 ezDuktapeFunction::ReturnInt(ezInt32 value)
{
  duk_push_int(GetContext(), value);
  return 1;
}

ezInt32 ezDuktapeFunction::ReturnFloat(float value)
{
  duk_push_number(GetContext(), value);
  return 1;
}

ezInt32 ezDuktapeFunction::ReturnNumber(double value)
{
  duk_push_number(GetContext(), value);
  return 1;
}

ezInt32 ezDuktapeFunction::ReturnString(const char* value)
{
  duk_push_string(GetContext(), value);
  return 1;
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
