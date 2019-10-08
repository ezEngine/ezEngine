#include <CorePCH.h>

#include <Core/Scripting/DuktapeWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Duktape/duktape.h>

ezDuktapeFunction::ezDuktapeFunction(duk_context* pExistingContext)
  : ezDuktapeWrapper(pExistingContext)
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

bool ezDuktapeFunction::GetBoolParameter(ezUInt32 uiArgIdx, bool fallback /*= false*/) const
{
  return duk_get_boolean_default(GetContext(), uiArgIdx, fallback);
}

ezInt32 ezDuktapeFunction::GetIntParameter(ezUInt32 uiArgIdx, ezInt32 fallback /*= 0*/) const
{
  return duk_get_int_default(GetContext(), uiArgIdx, fallback);
}

ezUInt32 ezDuktapeFunction::GetUIntParameter(ezUInt32 uiArgIdx, ezUInt32 fallback /*= 0*/) const
{
  return duk_get_uint_default(GetContext(), uiArgIdx, fallback);
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

#endif
