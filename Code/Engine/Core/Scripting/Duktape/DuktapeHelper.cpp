#include <CorePCH.h>

#include <Core/Scripting/DuktapeHelper.h>
#include <Duktape/duktape.h>
#include <Foundation/IO/FileSystem/FileReader.h>

EZ_CHECK_AT_COMPILETIME(ezDuktapeTypeMask::None == DUK_TYPE_MASK_NONE);
EZ_CHECK_AT_COMPILETIME(ezDuktapeTypeMask::Undefined == DUK_TYPE_MASK_UNDEFINED);
EZ_CHECK_AT_COMPILETIME(ezDuktapeTypeMask::Null == DUK_TYPE_MASK_NULL);
EZ_CHECK_AT_COMPILETIME(ezDuktapeTypeMask::Bool == DUK_TYPE_MASK_BOOLEAN);
EZ_CHECK_AT_COMPILETIME(ezDuktapeTypeMask::Number == DUK_TYPE_MASK_NUMBER);
EZ_CHECK_AT_COMPILETIME(ezDuktapeTypeMask::String == DUK_TYPE_MASK_STRING);
EZ_CHECK_AT_COMPILETIME(ezDuktapeTypeMask::Object == DUK_TYPE_MASK_OBJECT);
EZ_CHECK_AT_COMPILETIME(ezDuktapeTypeMask::Buffer == DUK_TYPE_MASK_BUFFER);
EZ_CHECK_AT_COMPILETIME(ezDuktapeTypeMask::Pointer == DUK_TYPE_MASK_POINTER);

ezDuktapeHelper::ezDuktapeHelper(duk_context* pContext, ezInt32 iExpectedStackChange)
  : m_pContext(pContext)
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  m_iExpectedStackChange = iExpectedStackChange;
  if (m_pContext)
  {
    m_iStackTopAtStart = duk_get_top(m_pContext);
  }
#endif
}

ezDuktapeHelper::~ezDuktapeHelper()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (m_pContext && m_iExpectedStackChange > -1000 && m_iExpectedStackChange < 1000)
  {
    const ezInt32 iCurTop = duk_get_top(m_pContext);
    const ezInt32 iStackChange = iCurTop - m_iStackTopAtStart;
    EZ_ASSERT_DEBUG(iStackChange == m_iExpectedStackChange, "Stack change ({}) is not as expected ({})", iStackChange, m_iExpectedStackChange);
  }
#endif
}

void ezDuktapeHelper::SetExpectedStackChange(ezInt32 iExpectedStackChange)
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  m_iExpectedStackChange = iExpectedStackChange;
#endif
}

void ezDuktapeHelper::Error(ezFormatString text)
{
  ezStringBuilder tmp;
  text.GetText(tmp);

  duk_error(m_pContext, DUK_ERR_ERROR, tmp.GetData());
}

void ezDuktapeHelper::PopStack(ezUInt32 n /*= 1*/)
{
  duk_pop_n(m_pContext, n);
}

void ezDuktapeHelper::PushGlobalObject()
{
  duk_push_global_object(m_pContext); // [ global ]
}

void ezDuktapeHelper::PushGlobalStash()
{
  duk_push_global_stash(m_pContext); // [ stash ]
}

ezResult ezDuktapeHelper::PushLocalObject(const char* szName, ezInt32 iParentObjectIndex /* = -1*/)
{
  duk_require_top_index(m_pContext);

  if (duk_get_prop_string(m_pContext, iParentObjectIndex, szName) == false) // [ obj/undef ]
  {
    duk_pop(m_pContext); // [ ]
    return EZ_FAILURE;
  }

  // [ object ]
  return EZ_SUCCESS;
}

bool ezDuktapeHelper::HasProperty(const char* szPropertyName, ezInt32 iParentObjectIndex /*= -1*/) const
{
  return duk_has_prop_string(m_pContext, iParentObjectIndex, szPropertyName);
}

bool ezDuktapeHelper::GetBoolProperty(const char* szPropertyName, bool fallback, ezInt32 iParentObjectIndex /*= -1*/) const
{
  bool result = fallback;

  if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
  {
    result = duk_get_boolean_default(m_pContext, -1, fallback); // [ value ]
  }

  duk_pop(m_pContext); // [ ]
  return result;
}

ezInt32 ezDuktapeHelper::GetIntProperty(const char* szPropertyName, ezInt32 fallback, ezInt32 iParentObjectIndex /*= -1*/) const
{
  ezInt32 result = fallback;

  if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
  {
    result = duk_get_int_default(m_pContext, -1, fallback); // [ value ]
  }

  duk_pop(m_pContext); // [ ]
  return result;
}

ezUInt32 ezDuktapeHelper::GetUIntProperty(const char* szPropertyName, ezUInt32 fallback, ezInt32 iParentObjectIndex /*= -1*/) const
{
  ezUInt32 result = fallback;

  if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
  {
    result = duk_get_uint_default(m_pContext, -1, fallback); // [ value ]
  }

  duk_pop(m_pContext); // [ ]
  return result;
}

float ezDuktapeHelper::GetFloatProperty(const char* szPropertyName, float fallback, ezInt32 iParentObjectIndex /*= -1*/) const
{
  return static_cast<float>(GetNumberProperty(szPropertyName, fallback, iParentObjectIndex));
}

double ezDuktapeHelper::GetNumberProperty(const char* szPropertyName, double fallback, ezInt32 iParentObjectIndex /*= -1*/) const
{
  double result = fallback;

  if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
  {
    result = duk_get_number_default(m_pContext, -1, fallback); // [ value ]
  }

  duk_pop(m_pContext); // [ ]
  return result;
}

const char* ezDuktapeHelper::GetStringProperty(const char* szPropertyName, const char* fallback, ezInt32 iParentObjectIndex /*= -1*/) const
{
  const char* result = fallback;

  if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
  {
    result = duk_get_string_default(m_pContext, -1, fallback); // [ value ]
  }

  duk_pop(m_pContext); // [ ]
  return result;
}

void ezDuktapeHelper::StorePointerInStash(const char* szKey, void* pPointer)
{
  duk_push_global_stash(m_pContext);                                                      // [ stash ]
  *reinterpret_cast<void**>(duk_push_fixed_buffer(m_pContext, sizeof(void*))) = pPointer; // [ stash buffer ]
  duk_put_prop_string(m_pContext, -2, szKey);                                             // [ stash ]
  duk_pop(m_pContext);                                                                    // [ ]
}

void* ezDuktapeHelper::RetrievePointerFromStash(const char* szKey) const
{
  void* pPointer = nullptr;

  duk_push_global_stash(m_pContext); // [ stash ]

  if (duk_get_prop_string(m_pContext, -1, szKey)) // [ stash obj/undef ]
  {
    EZ_ASSERT_DEBUG(duk_is_buffer(m_pContext, -1), "Object '{}' in stash is not a buffer", szKey);

    pPointer = *reinterpret_cast<void**>(duk_get_buffer(m_pContext, -1, nullptr)); // [ stash obj/undef ]
  }

  duk_pop_2(m_pContext); // [ ]

  return pPointer;
}

void ezDuktapeHelper::StoreStringInStash(const char* szKey, const char* value)
{
  duk_push_global_stash(m_pContext);          // [ stash ]
  duk_push_string(m_pContext, value);         // [ stash value ]
  duk_put_prop_string(m_pContext, -2, szKey); // [ stash ]
  duk_pop(m_pContext);                        // [ ]
}

const char* ezDuktapeHelper::RetrieveStringFromStash(const char* szKey, const char* szFallback /*= nullptr*/) const
{
  duk_push_global_stash(m_pContext); // [ stash ]

  if (!duk_get_prop_string(m_pContext, -1, szKey)) // [ stash string/undef ]
  {
    duk_pop_2(m_pContext); // [ ]
    return szFallback;
  }

  szFallback = duk_get_string_default(m_pContext, -1, szFallback); // [ stash string ]
  duk_pop_2(m_pContext);                                           // [ ]

  return szFallback;
}

bool ezDuktapeHelper::IsOfType(ezBitflags<ezDuktapeTypeMask> mask, ezInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, mask.GetValue());
}

bool ezDuktapeHelper::IsBool(ezInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_BOOLEAN);
}

bool ezDuktapeHelper::IsNumber(ezInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_NUMBER);
}

bool ezDuktapeHelper::IsString(ezInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_STRING);
}

bool ezDuktapeHelper::IsNull(ezInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_NULL);
}

bool ezDuktapeHelper::IsUndefined(ezInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_UNDEFINED);
}

bool ezDuktapeHelper::IsObject(ezInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_OBJECT);
}

bool ezDuktapeHelper::IsBuffer(ezInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_BUFFER);
}

bool ezDuktapeHelper::IsPointer(ezInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_POINTER);
}

bool ezDuktapeHelper::IsNullOrUndefined(ezInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_NULL | DUK_TYPE_MASK_UNDEFINED);
}

void ezDuktapeHelper::RegisterGlobalFunction(const char* szFunctionName, duk_c_function pFunction, ezUInt8 uiNumArguments, ezInt16 iMagicValue /*= 0*/)
{
  // TODO: could store iFuncIdx for faster function calls

  duk_push_global_object(m_pContext);                                                 // [ global ]
  /*const int iFuncIdx =*/duk_push_c_function(m_pContext, pFunction, uiNumArguments); // [ global func ]
  duk_set_magic(m_pContext, -1, iMagicValue);                                         // [ global func ]
  duk_put_prop_string(m_pContext, -2, szFunctionName);                                // [ global ]
  duk_pop(m_pContext);                                                                // [ ]
}

void ezDuktapeHelper::RegisterGlobalFunctionWithVarArgs(const char* szFunctionName, duk_c_function pFunction, ezInt16 iMagicValue /*= 0*/)
{
  // TODO: could store iFuncIdx for faster function calls

  duk_push_global_object(m_pContext);                                              // [ global ]
  /*const int iFuncIdx =*/duk_push_c_function(m_pContext, pFunction, DUK_VARARGS); // [ global func ]
  duk_set_magic(m_pContext, -1, iMagicValue);                                      // [ global func ]
  duk_put_prop_string(m_pContext, -2, szFunctionName);                             // [ global ]
  duk_pop(m_pContext);                                                             // [ ]
}

void ezDuktapeHelper::RegisterObjectFunction(const char* szFunctionName, duk_c_function pFunction, ezUInt8 uiNumArguments, ezInt32 iParentObjectIndex /*= -1*/, ezInt16 iMagicValue /*= 0*/)
{
  /*const int iFuncIdx =*/duk_push_c_function(m_pContext, pFunction, uiNumArguments); // [ func ]
  duk_set_magic(m_pContext, -1, iMagicValue);                                         // [ func ]

  if (iParentObjectIndex < 0)
  {
    duk_put_prop_string(m_pContext, iParentObjectIndex - 1, szFunctionName); // [ ]
  }
  else
  {
    duk_put_prop_string(m_pContext, iParentObjectIndex, szFunctionName); // [ ]
  }
}

ezResult ezDuktapeHelper::PrepareGlobalFunctionCall(const char* szFunctionName)
{
  if (!duk_get_global_string(m_pContext, szFunctionName)) // [ func/undef ]
    goto failure;

  if (!duk_is_function(m_pContext, -1)) // [ func ]
    goto failure;

  m_iPushedValues = 0;
  return EZ_SUCCESS; // [ func ]

failure:
  duk_pop(m_pContext); // [ ]
  return EZ_FAILURE;
}

ezResult ezDuktapeHelper::PrepareObjectFunctionCall(const char* szFunctionName, ezInt32 iParentObjectIndex /*= -1*/)
{
  duk_require_top_index(m_pContext);

  if (!duk_get_prop_string(m_pContext, iParentObjectIndex, szFunctionName)) // [ func/undef ]
    goto failure;

  if (!duk_is_function(m_pContext, -1)) // [ func ]
    goto failure;

  m_iPushedValues = 0;
  return EZ_SUCCESS; // [ func ]

failure:
  duk_pop(m_pContext); // [ ]
  return EZ_FAILURE;
}

ezResult ezDuktapeHelper::CallPreparedFunction()
{
  if (duk_pcall(m_pContext, m_iPushedValues) == DUK_EXEC_SUCCESS) // [ func n-args ] -> [ result/error ]
  {
    return EZ_SUCCESS; // [ result ]
  }
  else
  {
    // TODO: could also create a stack trace using duk_is_error + duk_get_prop_string(ctx, -1, "stack");
    ezLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1));
    return EZ_FAILURE; // [ error ]
  }
}

ezResult ezDuktapeHelper::PrepareMethodCall(const char* szMethodName, ezInt32 iParentObjectIndex /*= -1*/)
{
  if (!duk_get_prop_string(m_pContext, iParentObjectIndex, szMethodName)) // [ func/undef ]
    goto failure;

  if (!duk_is_function(m_pContext, -1)) // [ func ]
    goto failure;

  if (iParentObjectIndex < 0)
  {
    duk_dup(m_pContext, iParentObjectIndex - 1); // [ func this ]
  }
  else
  {
    duk_dup(m_pContext, iParentObjectIndex); // [ func this ]
  }

  m_iPushedValues = 0;
  return EZ_SUCCESS; // [ func this ]

failure:
  duk_pop(m_pContext); // [ ]
  return EZ_FAILURE;
}

ezResult ezDuktapeHelper::CallPreparedMethod()
{
  if (duk_pcall_method(m_pContext, m_iPushedValues) == DUK_EXEC_SUCCESS) // [ func this n-args ] -> [ result/error ]
  {
    return EZ_SUCCESS; // [ result ]
  }
  else
  {
    // TODO: could also create a stack trace using duk_is_error + duk_get_prop_string(ctx, -1, "stack");
    ezLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1));

    return EZ_FAILURE; // [ error ]
  }
}

void ezDuktapeHelper::PushInt(ezInt32 iParam)
{
  duk_push_int(m_pContext, iParam); // [ value ]
  ++m_iPushedValues;
}

void ezDuktapeHelper::PushUInt(ezUInt32 uiParam)
{
  duk_push_uint(m_pContext, uiParam); // [ value ]
  ++m_iPushedValues;
}

void ezDuktapeHelper::PushBool(bool bParam)
{
  duk_push_boolean(m_pContext, bParam); // [ value ]
  ++m_iPushedValues;
}

void ezDuktapeHelper::PushNumber(double fParam)
{
  duk_push_number(m_pContext, fParam); // [ value ]
  ++m_iPushedValues;
}

void ezDuktapeHelper::PushString(const ezStringView& sParam)
{
  duk_push_lstring(m_pContext, sParam.GetStartPointer(), sParam.GetElementCount()); // [ value ]
  ++m_iPushedValues;
}

void ezDuktapeHelper::PushNull()
{
  duk_push_null(m_pContext); // [ null ]
  ++m_iPushedValues;
}

void ezDuktapeHelper::PushUndefined()
{
  duk_push_undefined(m_pContext); // [ undefined ]
  ++m_iPushedValues;
}

bool ezDuktapeHelper::GetBoolValue(ezInt32 iStackElement, bool fallback /*= false*/) const
{
  return duk_get_boolean_default(m_pContext, iStackElement, fallback);
}

ezInt32 ezDuktapeHelper::GetIntValue(ezInt32 iStackElement, ezInt32 fallback /*= 0*/) const
{
  return duk_get_int_default(m_pContext, iStackElement, fallback);
}

ezUInt32 ezDuktapeHelper::GetUIntValue(ezInt32 iStackElement, ezUInt32 fallback /*= 0*/) const
{
  return duk_get_uint_default(m_pContext, iStackElement, fallback);
}

float ezDuktapeHelper::GetFloatValue(ezInt32 iStackElement, float fallback /*= 0*/) const
{
  return static_cast<float>(duk_get_number_default(m_pContext, iStackElement, fallback));
}

double ezDuktapeHelper::GetNumberValue(ezInt32 iStackElement, double fallback /*= 0*/) const
{
  return duk_get_number_default(m_pContext, iStackElement, fallback);
}

const char* ezDuktapeHelper::GetStringValue(ezInt32 iStackElement, const char* fallback /*= ""*/) const
{
  return duk_get_string_default(m_pContext, iStackElement, fallback);
}

ezResult ezDuktapeHelper::ExecuteString(const char* szString, const char* szDebugName /*= "eval"*/)
{
  duk_push_string(m_pContext, szDebugName);                       // [ filename ]
  if (duk_pcompile_string_filename(m_pContext, 0, szString) != 0) // [ function/error ]
  {
    EZ_LOG_BLOCK("DukTape::ExecuteString", "Compilation failed");

    ezLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1)); // [ error ]
    // TODO: print out line by line
    ezLog::Info("[duktape]Source: {0}", szString);

    duk_pop(m_pContext); // [ ]
    return EZ_FAILURE;
  }

  // [ function ]

  if (duk_pcall(m_pContext, 0) != DUK_EXEC_SUCCESS) // [ result/error ]
  {
    EZ_LOG_BLOCK("DukTape::ExecuteString", "Execution failed");

    ezLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1)); // [ error ]
    // TODO: print out line by line
    ezLog::Info("[duktape]Source: {0}", szString);

    duk_pop(m_pContext); // [ ]
    return EZ_FAILURE;
  }

  duk_pop(m_pContext); // [ ]
  return EZ_SUCCESS;
}

ezResult ezDuktapeHelper::ExecuteStream(ezStreamReader& stream, const char* szDebugName)
{
  ezStringBuilder source;
  source.ReadAll(stream);

  return ExecuteString(source, szDebugName);
}

ezResult ezDuktapeHelper::ExecuteFile(const char* szFile)
{
  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  return ExecuteStream(file, szFile);
}
