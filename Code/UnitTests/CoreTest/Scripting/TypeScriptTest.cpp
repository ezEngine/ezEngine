#include <CoreTestPCH.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Core/Scripting/DuktapeWrapper.h>

#  include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <TestFramework/Utilities/TestLogInterface.h>

EZ_CREATE_SIMPLE_TEST(Scripting, TypeScript)
{
  // setup file system
  {
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    ezStringBuilder sTestDataDir(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());
    sTestDataDir.AppendPath("Scripting/Duktape");
    if (EZ_TEST_RESULT(ezFileSystem::AddDataDirectory(sTestDataDir, "DuktapeTest")).Failed())
      return;

    if (EZ_TEST_RESULT(ezFileSystem::AddDataDirectory(">sdk/Data/Tools/ezEditor", "DuktapeTest")).Failed())
      return;
  }

  ezDuktapeWrapper dukTS("DukTS");

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compile TypeScriptServices")
  {
    EZ_TEST_RESULT(dukTS.ExecuteFile("Typescript/typescriptServices.js"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transpile Simple")
  {
    // simple way
    EZ_TEST_RESULT(dukTS.ExecuteString("ts.transpile('class X{}');"));

    // complicated way, needed to retrieve the result
    EZ_TEST_RESULT(dukTS.OpenObject("ts"));
    EZ_TEST_RESULT(dukTS.BeginFunctionCall("transpile"));
    dukTS.PushParameter("class X{}");
    EZ_TEST_RESULT(dukTS.ExecuteFunctionCall());

    EZ_TEST_BOOL(dukTS.IsReturnValueString());

    ezStringBuilder sTranspiled = dukTS.GetStringReturnValue();

    dukTS.EndFunctionCall();

    // validate that the transpiled code can be executed by Duktape
    ezDuktapeWrapper duk("duk");
    EZ_TEST_RESULT(duk.ExecuteString(sTranspiled));
  }

  ezFileSystem::RemoveDataDirectoryGroup("DuktapeTest");
}

#endif
