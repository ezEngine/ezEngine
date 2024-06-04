#include <CoreTest/CoreTestPCH.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Core/Scripting/DuktapeContext.h>

#  include <Duktape/duktape.h>
#  include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <Foundation/IO/FileSystem/FileWriter.h>
#  include <TestFramework/Utilities/TestLogInterface.h>

static ezResult TranspileString(const char* szSource, ezDuktapeContext& ref_script, ezStringBuilder& ref_sResult)
{
  ref_script.PushGlobalObject();                                           // [ global ]
  ref_script.PushLocalObject("ts").IgnoreResult();                         // [ global ts ]
  EZ_SUCCEED_OR_RETURN(ref_script.PrepareObjectFunctionCall("transpile")); // [ global ts transpile ]
  ref_script.PushString(szSource);                                         // [ global ts transpile source ]
  EZ_SUCCEED_OR_RETURN(ref_script.CallPreparedFunction());                 // [ global ts result ]
  ref_sResult = ref_script.GetStringValue(-1);                             // [ global ts result ]
  ref_script.PopStack(3);                                                  // [ ]

  return EZ_SUCCESS;
}

static ezResult TranspileFile(const char* szFile, ezDuktapeContext& ref_script, ezStringBuilder& ref_sResult)
{
  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  ezStringBuilder source;
  source.ReadAll(file);

  return TranspileString(source, ref_script, ref_sResult);
}

static ezResult TranspileFileToJS(const char* szFile, ezDuktapeContext& ref_script, ezStringBuilder& ref_sResult)
{
  EZ_SUCCEED_OR_RETURN(TranspileFile(szFile, ref_script, ref_sResult));

  ezStringBuilder sFile(":TypeScriptTest/", szFile);
  sFile.ChangeFileExtension("js");

  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(sFile));

  EZ_SUCCEED_OR_RETURN(file.WriteBytes(ref_sResult.GetData(), ref_sResult.GetElementCount()));
  return EZ_SUCCESS;
}

static int Duk_Print(duk_context* pContext)
{
  ezDuktapeFunction duk(pContext);

  ezLog::Info(duk.GetStringValue(0));

  return duk.ReturnVoid();
}

static duk_ret_t ModuleSearchFunction2(duk_context* pCtx)
{
  ezDuktapeFunction script(pCtx);

  /* Nargs was given as 4 and we get the following stack arguments:
   *   index 0: id
   *   index 1: require
   *   index 2: exports
   *   index 3: module
   */

  ezStringBuilder id = script.GetStringValue(0);
  id.ChangeFileExtension("js");

  ezStringBuilder source;
  ezFileReader file;
  file.Open(id).IgnoreResult();
  source.ReadAll(file);

  return script.ReturnString(source);


  /* Return 'undefined' to indicate no source code. */
  // return 0;
}

EZ_CREATE_SIMPLE_TEST(Scripting, TypeScript)
{
  // setup file system
  {
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    ezStringBuilder sTestDataDir(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());
    sTestDataDir.AppendPath("Scripting/TypeScript");
    if (!EZ_TEST_RESULT(ezFileSystem::AddDataDirectory(sTestDataDir, "TypeScriptTest")))
      return;

    const ezStringBuilder sWriteDir = ezTestFramework::GetInstance()->GetAbsOutputPath();
    if (!EZ_TEST_RESULT(ezFileSystem::AddDataDirectory(sWriteDir, "TypeScriptTest", "TypeScriptTest", ezDataDirUsage::AllowWrites)))
      return;

    if (!EZ_TEST_RESULT(ezFileSystem::AddDataDirectory(">sdk/Data/Tools/ezEditor", "DuktapeTest")))
      return;
  }

  ezDuktapeContext duk("DukTS");
  duk.EnableModuleSupport(ModuleSearchFunction2);

  duk.RegisterGlobalFunction("Print", Duk_Print, 1);

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
    TranspileString("class X{}", duk, sTranspiled).IgnoreResult();

    // validate that the transpiled code can be executed by Duktape
    ezDuktapeContext duk2("duk");
    EZ_TEST_RESULT(duk2.ExecuteString(sTranspiled));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transpile File")
  {
    ezStringBuilder result;
    EZ_TEST_RESULT(TranspileFileToJS("Foo.ts", duk, result));

    duk.ExecuteFile("Foo.js").IgnoreResult();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Import files")
  {
    ezStringBuilder result;
    EZ_TEST_RESULT(TranspileFileToJS("Bar.ts", duk, result));

    duk.ExecuteFile("Bar.js").IgnoreResult();
  }

  ezFileSystem::DeleteFile(":TypeScriptTest/Foo.js");
  ezFileSystem::DeleteFile(":TypeScriptTest/Bar.js");

  ezFileSystem::RemoveDataDirectoryGroup("DuktapeTest");
}

#endif
