#include <CoreTestPCH.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Core/Scripting/DuktapeWrapper.h>

#  include <Duktape/duktape.h>
#  include <TestFramework/Utilities/TestLogInterface.h>

static int CFuncPrint(duk_context* pContext)
{
  const char* szText = duk_require_string(pContext, 0);

  ezLog::Info("Print: '{}'", szText);

  return 0;
}

EZ_CREATE_SIMPLE_TEST(Scripting, DuktapeWrapper)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Basics")
  {
    ezDuktapeWrapper duk("DukTest");

    duk_eval_string(duk.GetContext(), "'testString'.toUpperCase()");
    ezStringBuilder sTestString = duk_get_string(duk.GetContext(), -1);
    duk_pop(duk.GetContext());
    EZ_TEST_STRING(sTestString, "TESTSTRING");

    EZ_TEST_BOOL(duk.ExecuteString("function MakeUpper(bla) { return bla.toUpperCase() }").Succeeded());


    duk_eval_string(duk.GetContext(), "MakeUpper(\"myTest\")");
    sTestString = duk_get_string(duk.GetContext(), -1);
    duk_pop(duk.GetContext());
    EZ_TEST_STRING(sTestString, "MYTEST");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExecuteString (error)")
  {
    ezDuktapeWrapper duk("DukTest");

    ezTestLogInterface log;
    ezTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("SyntaxError: parse error (line 1)", ezLogMsgType::ErrorMsg);

    EZ_TEST_BOOL(duk.ExecuteString(" == invalid code == ").Failed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "C Function")
  {
    ezDuktapeWrapper duk("DukTest");

    ezTestLogInterface log;
    ezTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("Hello Test", ezLogMsgType::InfoMsg);

    duk.RegisterFunction("Print", CFuncPrint, 1);

    duk.ExecuteString("Print('Hello Test')");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Call Function")
  {
    ezDuktapeWrapper duk("DukTest");

    ezTestLogInterface log;
    ezTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("You did it", ezLogMsgType::InfoMsg);

    duk.RegisterFunction("Print", CFuncPrint, 1);

    if (EZ_TEST_BOOL(duk.PrepareFunctionCall("Print")).Succeeded())
    {
      duk.PushParameter("You did it, Fry!");
      EZ_TEST_BOOL(duk.CallPreparedFunction().Succeeded());
    }
  }
}

#endif
