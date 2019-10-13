#include <CorePCH.h>

#include <Core/Scripting/DuktapeContext.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Duktape/duk_module_duktape.h>
#  include <Duktape/duktape.h>

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
