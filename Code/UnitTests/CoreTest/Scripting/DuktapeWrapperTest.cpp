#include <CoreTestPCH.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Core/Scripting/DuktapeWrapper.h>

#  include <Duktape/duk_module_duktape.h>
#  include <Duktape/duktape.h>
#  include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <TestFramework/Utilities/TestLogInterface.h>
#include <Foundation/IO/FileSystem/FileReader.h>

duk_ret_t ModuleSearchFunction(duk_context* ctx);

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
    if (EZ_TEST_RESULT(ezFileSystem::AddDataDirectory(sTestDataDir, "DuktapeTest")).Failed())
      return;
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Basics")
  {
    ezDuktapeWrapper duk("DukTest");

    duk_eval_string(duk.GetContext(), "'testString'.toUpperCase()");
    ezStringBuilder sTestString = duk_get_string(duk.GetContext(), -1);
    duk_pop(duk.GetContext());
    EZ_TEST_STRING(sTestString, "TESTSTRING");

    EZ_TEST_RESULT(duk.ExecuteString("function MakeUpper(bla) { return bla.toUpperCase() }"));


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
    duk.EnableModuleSupport(nullptr);

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

    if (EZ_TEST_RESULT(duk.BeginFunctionCall("Print")).Succeeded())
    {
      duk.PushParameter("You did it, Fry!");
      EZ_TEST_RESULT(duk.ExecuteFunctionCall());

      duk.EndFunctionCall();
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

    if (EZ_TEST_RESULT(duk.BeginFunctionCall("Magic1")).Succeeded())
    {
      EZ_TEST_RESULT(duk.ExecuteFunctionCall());
      duk.EndFunctionCall();
    }

    if (EZ_TEST_RESULT(duk.BeginFunctionCall("Magic2")).Succeeded())
    {
      EZ_TEST_RESULT(duk.ExecuteFunctionCall());
      duk.EndFunctionCall();
    }

    if (EZ_TEST_RESULT(duk.BeginFunctionCall("Magic3")).Succeeded())
    {
      EZ_TEST_RESULT(duk.ExecuteFunctionCall());
      duk.EndFunctionCall();
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Inspect Object")
  {
    ezDuktapeWrapper duk("DukTest");
    EZ_TEST_RESULT(duk.ExecuteFile("Object.js"));

    EZ_TEST_RESULT(duk.OpenObject("obj"));

    EZ_TEST_BOOL(duk.HasProperty("i"));
    EZ_TEST_INT(duk.GetIntProperty("i", 0), 23);

    EZ_TEST_BOOL(duk.HasProperty("f"));
    EZ_TEST_FLOAT(duk.GetFloatProperty("f", 0), 4.2f, 0.01f);

    EZ_TEST_BOOL(duk.HasProperty("b"));
    EZ_TEST_BOOL(duk.GetBoolProperty("b", false));

    EZ_TEST_BOOL(duk.HasProperty("s"));
    EZ_TEST_STRING(duk.GetStringProperty("s", ""), "text");

    EZ_TEST_BOOL(duk.HasProperty("n"));

    EZ_TEST_BOOL(duk.HasProperty("o"));

    {
      EZ_TEST_RESULT(duk.OpenObject("o"));
      EZ_TEST_BOOL(duk.HasProperty("sub"));
      EZ_TEST_STRING(duk.GetStringProperty("sub", ""), "wub");

      duk.CloseObject();
    }
    duk.CloseObject();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "require")
  {
    ezDuktapeWrapper duk("DukTest");
    duk.EnableModuleSupport(ModuleSearchFunction);

    duk.RegisterFunction("Print", CFuncPrint, 1);

    ezTestLogInterface log;
    ezTestLogSystemScope logSystemScope(&log);
    log.ExpectMessage("Print: 'called f1'", ezLogMsgType::InfoMsg);
    log.ExpectMessage("Print: 'Called require.js'", ezLogMsgType::InfoMsg);
    log.ExpectMessage("Print: 'require.js: called f1'", ezLogMsgType::InfoMsg);

    EZ_TEST_RESULT(duk.ExecuteFile("require.js"));
  }

  ezFileSystem::RemoveDataDirectoryGroup("DuktapeTest");
}

static duk_ret_t ModuleSearchFunction(duk_context* ctx)
{
  ezDuktapeFunction script(ctx);

  /* Nargs was given as 4 and we get the following stack arguments:
  *   index 0: id
  *   index 1: require
  *   index 2: exports
  *   index 3: module
  */

  ezStringBuilder id = script.GetStringParameter(0);
  id.ChangeFileExtension("js");

  ezStringBuilder source;
  ezFileReader file;
  file.Open(id);
  source.ReadAll(file);

  return script.ReturnString(source);

  /* Return 'undefined' to indicate no source code. */
  //return 0;
}

#endif
