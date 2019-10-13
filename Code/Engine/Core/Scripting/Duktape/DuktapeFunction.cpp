#include <Core/CorePCH.h>

#include <Core/Scripting/DuktapeFunction.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Duktape/duk_module_duktape.h>
#  include <Duktape/duktape.h>

ezDuktapeFunction::ezDuktapeFunction(duk_context* pExistingContext)
  : ezDuktapeHelper(pExistingContext)
{
}

ezDuktapeFunction::~ezDuktapeFunction()
{
  EZ_ASSERT_DEV(m_bDidReturnValue, "You need to call one ezDuktapeFunction::ReturnXY() and return its result from your C function.");
}

ezUInt32 ezDuktapeFunction::GetNumVarArgFunctionParameters() const
{
  return duk_get_top(GetContext());
}

ezInt16 ezDuktapeFunction::GetFunctionMagicValue() const
{
  return duk_get_current_magic(GetContext());
}

ezInt32 ezDuktapeFunction::ReturnVoid()
{
  EZ_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  return 0;
}

ezInt32 ezDuktapeFunction::ReturnNull()
{
  EZ_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_null(GetContext());
  return 1;
}

ezInt32 ezDuktapeFunction::ReturnUndefined()
{
  EZ_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_undefined(GetContext());
  return 1;
}

ezInt32 ezDuktapeFunction::ReturnBool(bool value)
{
  EZ_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_boolean(GetContext(), value);
  return 1;
}

ezInt32 ezDuktapeFunction::ReturnInt(ezInt32 value)
{
  EZ_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_int(GetContext(), value);
  return 1;
}

ezInt32 ezDuktapeFunction::ReturnUInt(ezUInt32 value)
{
  EZ_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_uint(GetContext(), value);
  return 1;
}

ezInt32 ezDuktapeFunction::ReturnFloat(float value)
{
  EZ_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_number(GetContext(), value);
  return 1;
}

ezInt32 ezDuktapeFunction::ReturnNumber(double value)
{
  EZ_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_number(GetContext(), value);
  return 1;
}

ezInt32 ezDuktapeFunction::ReturnString(const char* value)
{
  EZ_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_string(GetContext(), value);
  return 1;
}

ezInt32 ezDuktapeFunction::ReturnCustom()
{
  EZ_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  // push nothing, the user calls this because he pushed something custom already
  return 1;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

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
