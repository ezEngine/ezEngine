#include <CoreTestPCH.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Core/Scripting/DuktapeWrapper.h>

#  include <Duktape/duktape.h>
#  include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <TestFramework/Utilities/TestLogInterface.h>

static int CFuncPrint(duk_context* pContext)
{
  ezDuktapeFunction wrapper(pContext);
  const char* szText = wrapper.GetStringParameter(0, nullptr);

  ezLog::Info("Print: '{}'", szText);
  return wrapper.ReturnVoid();
}

static int CFuncPrintVA(duk_context* pContext)
{
  ezDuktapeFunction wrapper(pContext);

  const ezUInt32 uiNumArgs = wrapper.GetNumVarArgFunctionParameters();

  ezStringBuilder s;
  s.AppendFormat("#Args: {}", uiNumArgs);

  for (ezUInt32 arg = 0; arg < uiNumArgs; ++arg)
  {
    if (wrapper.IsParameterNumber(arg))
    {
      double val = wrapper.GetNumberParameter(arg);
      s.AppendFormat(", #{}: Number = {}", arg, val);
    }
    else if (wrapper.IsParameterBool(arg))
    {
      bool val = wrapper.GetBoolParameter(arg);
      s.AppendFormat(", #{}: Bool = {}", arg, val);
    }
    else if (wrapper.IsParameterString(arg))
    {
      const char* val = wrapper.GetStringParameter(arg);
      s.AppendFormat(", #{}: String = {}", arg, val);
    }
    else if (wrapper.IsParameterNull(arg))
    {
      s.AppendFormat(", #{}: null", arg);
    }
    else if (wrapper.IsParameterUndefined(arg))
    {
      s.AppendFormat(", #{}: undefined", arg);
    }
    else if (wrapper.IsParameterObject(arg))
    {
      s.AppendFormat(", #{}: object", arg);
    }
    else if (duk_check_type_mask(pContext, arg, DUK_TYPE_MASK_BUFFER))
    {
      s.AppendFormat(", #{}: buffer", arg);
    }
    else if (duk_check_type_mask(pContext, arg, DUK_TYPE_MASK_POINTER))
    {
      s.AppendFormat(", #{}: pointer", arg);
    }
    else if (duk_check_type_mask(pContext, arg, DUK_TYPE_MASK_LIGHTFUNC))
    {
      s.AppendFormat(", #{}: lightfunc", arg);
    }
    else
    {
      s.AppendFormat(", #{}: UNKNOWN TYPE", arg);
    }
  }

  ezLog::Info(s);
  return wrapper.ReturnString(s);
}

static int CFuncMagic(duk_context* pContext)
{
  ezDuktapeFunction wrapper(pContext);
  ezInt16 iMagic = wrapper.GetFunctionMagicValue();

  ezLog::Info("Magic: '{}'", iMagic);
  return wrapper.ReturnInt(iMagic);
}


EZ_CREATE_SIMPLE_TEST(Scripting, DuktapeWrapper)
{
  // setup file system
  {
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    ezStringBuilder sTestDataDir(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());
    sTestDataDir.AppendPath("Scripting/Duktape");
    if (EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sTestDataDir, "DuktapeTest").Succeeded()).Failed())
      return;
  }

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

    log.ExpectMessage("ReferenceError: identifier 'Print' undefined", ezLogMsgType::ErrorMsg);
    EZ_TEST_BOOL(duk.ExecuteString("Print(\"do stuff\")").Failed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExecuteFile")
  {
    ezDuktapeWrapper duk("DukTest");

    ezTestLogInterface log;
    ezTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("Print: 'called f1'", ezLogMsgType::InfoMsg);

    duk.RegisterFunction("Print", CFuncPrint, 1);

    duk.ExecuteFile("ExecuteFile.js");
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "VarArgs C Function")
  {
    ezDuktapeWrapper duk("DukTest");

    ezTestLogInterface log;
    ezTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("#Args: 5, #0: String = text, #1: Number = 7, #2: Bool = true, #3: null, #4: object", ezLogMsgType::InfoMsg);

    duk.RegisterFunctionWithVarArgs("PrintVA", CFuncPrintVA);

    duk.ExecuteString("PrintVA('text', 7, true, null, {})");
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Function Magic Value")
  {
    ezDuktapeWrapper duk("DukTest");

    ezTestLogInterface log;
    ezTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("Magic: '1'", ezLogMsgType::InfoMsg);
    log.ExpectMessage("Magic: '2'", ezLogMsgType::InfoMsg);
    log.ExpectMessage("Magic: '3'", ezLogMsgType::InfoMsg);

    duk.RegisterFunction("Magic1", CFuncMagic, 0, 1);
    duk.RegisterFunction("Magic2", CFuncMagic, 0, 2);
    duk.RegisterFunction("Magic3", CFuncMagic, 0, 3);

    if (EZ_TEST_BOOL(duk.PrepareFunctionCall("Magic1")).Succeeded())
    {
      EZ_TEST_BOOL(duk.CallPreparedFunction().Succeeded());
    }

    if (EZ_TEST_BOOL(duk.PrepareFunctionCall("Magic2")).Succeeded())
    {
      EZ_TEST_BOOL(duk.CallPreparedFunction().Succeeded());
    }

    if (EZ_TEST_BOOL(duk.PrepareFunctionCall("Magic3")).Succeeded())
    {
      EZ_TEST_BOOL(duk.CallPreparedFunction().Succeeded());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Inspect Object")
  {
    ezDuktapeWrapper duk("DukTest");

    const char* objCode = "var obj = \n\
{\n\
  a : \"one\",\n\
  b : 2.0,\n\
  c : true\n\
};";

    EZ_TEST_BOOL(duk.ExecuteString(objCode).Succeeded());

    if (EZ_TEST_BOOL(duk.OpenObject("obj").Succeeded()).Succeeded())
    {
      EZ_TEST_BOOL(duk.HasProperty("a"));
      EZ_TEST_BOOL(duk.HasProperty("b"));
      EZ_TEST_BOOL(duk.HasProperty("c"));
      EZ_TEST_BOOL(!duk.HasProperty("d"));

      duk.CloseObject();
    }
  }

  ezFileSystem::RemoveDataDirectoryGroup("DuktapeTest");
}

EZ_CREATE_SIMPLE_TEST(Scripting, TypeScript)
{
  // setup file system
  {
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    ezStringBuilder sTestDataDir(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());
    sTestDataDir.AppendPath("Scripting/Duktape");
    if (EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sTestDataDir, "DuktapeTest").Succeeded()).Failed())
      return;

    if (EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(">sdk/Data/Tools/ezEditor", "DuktapeTest").Succeeded()).Failed())
      return;
  }

  ezDuktapeWrapper dukTS("DukTS");

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compile TypeScriptServices")
  {
    EZ_TEST_BOOL(dukTS.ExecuteFile("Typescript/typescriptServices.js").Succeeded());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transpile Simple")
  {
    // simple way
    EZ_TEST_BOOL(dukTS.ExecuteString("ts.transpile('class X{}');").Succeeded());

    // complicated way
    //EZ_TEST_BOOL(dukTS.OpenObject("ts").Succeeded());
    //EZ_TEST_BOOL(dukTS.PrepareFunctionCall("transpile"));
    //dukTS.PushParameter("class X{}');");
    //EZ_TEST_BOOL(dukTS.CallPreparedFunction().Succeeded());
  }

  ezFileSystem::RemoveDataDirectoryGroup("DuktapeTest");
}

#endif
