#include <CoreTestPCH.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Core/Scripting/DuktapeWrapper.h>

#  include <Duktape/duktape.h>
#  include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <Foundation/IO/FileSystem/FileWriter.h>
#  include <TestFramework/Utilities/TestLogInterface.h>

static ezResult TranspileString(const char* szSource, ezDuktapeWrapper& script, ezStringBuilder& result)
{
  EZ_SUCCEED_OR_RETURN(script.OpenObject("ts"));
  EZ_SUCCEED_OR_RETURN(script.BeginFunctionCall("transpile"));
  script.PushParameter(szSource);
  EZ_SUCCEED_OR_RETURN(script.ExecuteFunctionCall());

  result = script.GetStringReturnValue();

  script.EndFunctionCall();
  script.CloseObject();

  return EZ_SUCCESS;
}

static ezResult TranspileFile(const char* szFile, ezDuktapeWrapper& script, ezStringBuilder& result)
{
  ezFileReader file;
  file.Open(szFile);

  ezStringBuilder source;
  source.ReadAll(file);

  return TranspileString(source, script, result);
}

static ezResult TranspileFileToJS(const char* szFile, ezDuktapeWrapper& script, ezStringBuilder& result)
{
  EZ_SUCCEED_OR_RETURN(TranspileFile(szFile, script, result));

  ezStringBuilder sFile(":TypeScriptTest/", szFile);
  sFile.ChangeFileExtension("js");

  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(sFile));

  file.WriteBytes(result.GetData(), result.GetElementCount());
  return EZ_SUCCESS;
}

static int Duk_Print(duk_context* pContext)
{
  ezDuktapeFunction duk(pContext);

  ezLog::Info(duk.GetStringParameter(0));

  return duk.ReturnVoid();
}

static duk_ret_t ModuleSearchFunction2(duk_context* ctx)
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

EZ_CREATE_SIMPLE_TEST(Scripting, TypeScript)
{
  // setup file system
  {
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    ezStringBuilder sTestDataDir(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());
    sTestDataDir.AppendPath("Scripting/TypeScript");
    if (EZ_TEST_RESULT(ezFileSystem::AddDataDirectory(sTestDataDir, "TypeScriptTest", "TypeScriptTest", ezFileSystem::AllowWrites)).Failed())
      return;

    if (EZ_TEST_RESULT(ezFileSystem::AddDataDirectory(">sdk/Data/Tools/ezEditor", "DuktapeTest")).Failed())
      return;
  }

  ezDuktapeWrapper duk("DukTS");
  duk.EnableModuleSupport(ModuleSearchFunction2);

  duk.RegisterFunction("Print", Duk_Print, 1);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compile TypeScriptServices")
  {
    EZ_TEST_RESULT(duk.ExecuteFile("Typescript/typescriptServices.js"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transpile Simple")
  {
    // simple way
    EZ_TEST_RESULT(duk.ExecuteString("ts.transpile('class X{}');"));

    // complicated way, needed to retrieve the result
    ezStringBuilder sTranspiled;
    TranspileString("class X{}", duk, sTranspiled);

    // validate that the transpiled code can be executed by Duktape
    ezDuktapeWrapper duk("duk");
    EZ_TEST_RESULT(duk.ExecuteString(sTranspiled));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transpile File")
  {
    ezStringBuilder result;
    EZ_TEST_RESULT(TranspileFileToJS("Foo.ts", duk, result));

    duk.ExecuteFile("Foo.js");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Import files")
  {
    ezStringBuilder result;
    EZ_TEST_RESULT(TranspileFileToJS("Bar.ts", duk, result));

    duk.ExecuteFile("Bar.js");
  }

  ezFileSystem::DeleteFile(":TypeScriptTest/Foo.js");
  ezFileSystem::DeleteFile(":TypeScriptTest/Bar.js");

  ezFileSystem::RemoveDataDirectoryGroup("DuktapeTest");
}

#endif
