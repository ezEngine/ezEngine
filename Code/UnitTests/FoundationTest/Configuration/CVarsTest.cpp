#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Utilities/CommandLineUtils.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Configuration);

#define ezCVarValueDefault ezCVarValue::Default
#define ezCVarValueStored ezCVarValue::Stored
#define ezCVarValueRestart ezCVarValue::DelayedSync

// Interestingly using 'ezCVarValue::Default' directly inside a macro does not work. (?!)
#define CHECK_CVAR(var, Current, Default, Stored, Restart)      \
  EZ_TEST_BOOL(var != nullptr);                                 \
  if (var != nullptr)                                           \
  {                                                             \
    EZ_TEST_BOOL(var->GetValue() == Current);                   \
    EZ_TEST_BOOL(var->GetValue(ezCVarValueDefault) == Default); \
    EZ_TEST_BOOL(var->GetValue(ezCVarValueStored) == Stored);   \
    EZ_TEST_BOOL(var->GetValue(ezCVarValueRestart) == Restart); \
  }

static ezInt32 iChangedValue = 0;
static ezInt32 iChangedRestart = 0;

#if EZ_ENABLED(EZ_SUPPORTS_DYNAMIC_PLUGINS) && EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)

static void ChangedCVar(const ezCVarEvent& e)
{
  switch (e.m_EventType)
  {
    case ezCVarEvent::ValueChanged:
      ++iChangedValue;
      break;
    case ezCVarEvent::DelayedSyncValueChanged:
      ++iChangedRestart;
      break;
    default:
      break;
  }
}

#endif

EZ_CREATE_SIMPLE_TEST(Configuration, CVars)
{
  iChangedValue = 0;
  iChangedRestart = 0;

  // setup the filesystem
  // we need it to test the storing of cvars (during plugin reloading)

  ezStringBuilder sOutputFolder1 = ezTestFramework::GetInstance()->GetAbsOutputPath();

  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder1.GetData(), "test", "output", ezDataDirUsage::AllowWrites) == EZ_SUCCESS);

  // Delete all cvar setting files
  {
    ezStringBuilder sConfigFile;

    sConfigFile = ":output/CVars/CVars_" ezFoundationTest_Plugin1 ".cfg";

    ezFileSystem::DeleteFile(sConfigFile.GetData());

    sConfigFile = ":output/CVars/CVars_" ezFoundationTest_Plugin2 ".cfg";

    ezFileSystem::DeleteFile(sConfigFile.GetData());
  }

  ezCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-test1_Int2");
  ezCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("102");

  ezCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-test1_Float2");
  ezCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("102.2");

  ezCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-test1_Bool2");
  ezCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("false");

  ezCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-test1_String2");
  ezCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("test1c");

  ezCVar::SetStorageFolder(":output/CVars");
  ezCVar::LoadCVars(); // should do nothing (no settings files available)

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SaveCVarsToFile and LoadCVarsFromFile again")
  {
    const char* cvarConfigFileDir = ezTestFramework::GetInstance()->GetAbsOutputPath();
    EZ_TEST_BOOL_MSG(ezFileSystem::AddDataDirectory(cvarConfigFileDir, "CVarsTest", "CVarConfigTempDir", ezDataDirUsage::AllowWrites) == EZ_SUCCESS, "Failed to mount data dir '%s'", cvarConfigFileDir);
    ezStringView cvarConfigFile = ":CVarConfigTempDir/CVars.cfg";

    ezCVarInt testCVarInt("testCVarInt", 0, ezCVarFlags::Default, "Test");
    ezCVarFloat testCVarFloat("testCVarFloat", 0.0f, ezCVarFlags::Default, "Test");
    ezCVarBool testCVarBool("testCVarBool", false, ezCVarFlags::Save, "Test");
    ezCVarString testCVarString("testCVarString", "", ezCVarFlags::Save, "Test");

    // ignore save flag = false
    {
      testCVarInt = 481516;
      testCVarFloat = 23.42f;
      testCVarBool = true;
      testCVarString = "Hello World!";

      bool bIgnoreSaveFlag = false;
      ezCVar::SaveCVarsToFile(cvarConfigFile, bIgnoreSaveFlag);
      EZ_TEST_BOOL(ezFileSystem::ExistsFile(cvarConfigFile) == EZ_SUCCESS);

      testCVarInt = 0;
      testCVarFloat = 0.0f;
      testCVarBool = false;
      testCVarString = "";

      ezDynamicArray<ezCVar*> outCVars;
      constexpr bool bOnlyNewOnes = false;
      constexpr bool bSetAsCurrentValue = true;
      ezCVar::LoadCVarsFromFile(cvarConfigFile, bOnlyNewOnes, bSetAsCurrentValue, bIgnoreSaveFlag, &outCVars);

      EZ_TEST_INT(testCVarInt, 0);
      EZ_TEST_FLOAT(testCVarFloat, 0.0f, ezMath::DefaultEpsilon<float>());
      EZ_TEST_BOOL(testCVarBool == true);
      EZ_TEST_STRING(testCVarString.GetValue(), "Hello World!");

      EZ_TEST_BOOL(outCVars.Contains(&testCVarInt) == false);
      EZ_TEST_BOOL(outCVars.Contains(&testCVarFloat) == false);
      EZ_TEST_BOOL(outCVars.Contains(&testCVarBool));
      EZ_TEST_BOOL(outCVars.Contains(&testCVarString));

      testCVarInt = 0;
      testCVarFloat = 0.0f;
      testCVarBool = false;
      testCVarString = "";

      // Even if we ignore the save flag the result should be same as above since we only stored CVars with the save flag in the file.
      bIgnoreSaveFlag = true;
      ezCVar::LoadCVarsFromFile(cvarConfigFile, bOnlyNewOnes, bSetAsCurrentValue, bIgnoreSaveFlag, &outCVars);

      EZ_TEST_INT(testCVarInt, 0);
      EZ_TEST_FLOAT(testCVarFloat, 0.0f, ezMath::DefaultEpsilon<float>());
      EZ_TEST_BOOL(testCVarBool == true);
      EZ_TEST_STRING(testCVarString.GetValue(), "Hello World!");

      EZ_TEST_BOOL(outCVars.Contains(&testCVarInt) == false);
      EZ_TEST_BOOL(outCVars.Contains(&testCVarFloat) == false);
      EZ_TEST_BOOL(outCVars.Contains(&testCVarBool));
      EZ_TEST_BOOL(outCVars.Contains(&testCVarString));

      ezFileSystem::DeleteFile(cvarConfigFile);
    }

    // ignore save flag = true
    {
      testCVarInt = 481516;
      testCVarFloat = 23.42f;
      testCVarBool = true;
      testCVarString = "Hello World!";

      bool bIgnoreSaveFlag = true;
      ezCVar::SaveCVarsToFile(cvarConfigFile, bIgnoreSaveFlag);
      EZ_TEST_BOOL(ezFileSystem::ExistsFile(cvarConfigFile) == EZ_SUCCESS);

      testCVarInt = 0;
      testCVarFloat = 0.0f;
      testCVarBool = false;
      testCVarString = "";

      ezDynamicArray<ezCVar*> outCVars;
      constexpr bool bOnlyNewOnes = false;
      constexpr bool bSetAsCurrentValue = true;
      // Check whether the save flag is correctly checked during load now that we have saved all CVars to the file.
      bIgnoreSaveFlag = false;
      ezCVar::LoadCVarsFromFile(cvarConfigFile, bOnlyNewOnes, bSetAsCurrentValue, bIgnoreSaveFlag, &outCVars);

      EZ_TEST_INT(testCVarInt, 0);
      EZ_TEST_FLOAT(testCVarFloat, 0.0f, ezMath::DefaultEpsilon<float>());
      EZ_TEST_BOOL(testCVarBool == true);
      EZ_TEST_STRING(testCVarString.GetValue(), "Hello World!");

      EZ_TEST_BOOL(outCVars.Contains(&testCVarInt) == false);
      EZ_TEST_BOOL(outCVars.Contains(&testCVarFloat) == false);
      EZ_TEST_BOOL(outCVars.Contains(&testCVarBool));
      EZ_TEST_BOOL(outCVars.Contains(&testCVarString));

      testCVarInt = 0;
      testCVarFloat = 0.0f;
      testCVarBool = false;
      testCVarString = "";

      // Now load all cvars stored in the file.
      bIgnoreSaveFlag = true;
      ezCVar::LoadCVarsFromFile(cvarConfigFile, bOnlyNewOnes, bSetAsCurrentValue, bIgnoreSaveFlag, &outCVars);

      EZ_TEST_INT(testCVarInt, 481516);
      EZ_TEST_FLOAT(testCVarFloat, 23.42f, ezMath::DefaultEpsilon<float>());
      EZ_TEST_BOOL(testCVarBool == true);
      EZ_TEST_STRING(testCVarString.GetValue(), "Hello World!");

      EZ_TEST_BOOL(outCVars.Contains(&testCVarInt));
      EZ_TEST_BOOL(outCVars.Contains(&testCVarFloat));
      EZ_TEST_BOOL(outCVars.Contains(&testCVarBool));
      EZ_TEST_BOOL(outCVars.Contains(&testCVarString));

      ezFileSystem::DeleteFile(cvarConfigFile);
    }


    EZ_TEST_BOOL(ezFileSystem::RemoveDataDirectory("CVarConfigTempDir"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "No Plugin Loaded")
  {
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Int") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Float") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Bool") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_String") == nullptr);

    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_Int") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_Float") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_Bool") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_String") == nullptr);
  }

#if EZ_ENABLED(EZ_SUPPORTS_DYNAMIC_PLUGINS) && EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Plugin1 Loaded")
  {
    EZ_TEST_BOOL(ezPlugin::LoadPlugin(ezFoundationTest_Plugin1) == EZ_SUCCESS);

    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Int") != nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Float") != nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Bool") != nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_String") != nullptr);

    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Int2") != nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Float2") != nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Bool2") != nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_String2") != nullptr);

    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_Int") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_Float") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_Bool") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_String") == nullptr);

    ezPlugin::UnloadAllPlugins();
  }

#endif

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "No Plugin Loaded (2)")
  {
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Int") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Float") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Bool") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_String") == nullptr);

    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_Int") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_Float") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_Bool") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_String") == nullptr);
  }

#if EZ_ENABLED(EZ_SUPPORTS_DYNAMIC_PLUGINS) && EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Plugin2 Loaded")
  {
    // Plugin2 should automatically load Plugin1 with it

    EZ_TEST_BOOL(ezPlugin::LoadPlugin(ezFoundationTest_Plugin2) == EZ_SUCCESS);

    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Int") != nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Float") != nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Bool") != nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_String") != nullptr);

    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_Int") != nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_Float") != nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_Bool") != nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_String") != nullptr);

    ezPlugin::UnloadAllPlugins();
  }

#endif

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "No Plugin Loaded (2)")
  {
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Int") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Float") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_Bool") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test1_String") == nullptr);

    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_Int") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_Float") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_Bool") == nullptr);
    EZ_TEST_BOOL(ezCVar::FindCVarByName("test2_String") == nullptr);
  }

#if EZ_ENABLED(EZ_SUPPORTS_DYNAMIC_PLUGINS) && EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Default Value Test")
  {
    EZ_TEST_BOOL(ezPlugin::LoadPlugin(ezFoundationTest_Plugin2) == EZ_SUCCESS);

    // CVars from Plugin 1
    {
      ezCVarInt* pInt = (ezCVarInt*)ezCVar::FindCVarByName("test1_Int");
      CHECK_CVAR(pInt, 11, 11, 11, 11);

      if (pInt)
      {
        EZ_TEST_BOOL(pInt->GetType() == ezCVarType::Int);
        EZ_TEST_BOOL(pInt->GetName() == "test1_Int");
        EZ_TEST_BOOL(pInt->GetDescription() == "Desc: test1_Int");

        pInt->m_CVarEvents.AddEventHandler(ChangedCVar);

        *pInt = 12;
        CHECK_CVAR(pInt, 12, 11, 11, 12);
        EZ_TEST_INT(iChangedValue, 1);
        EZ_TEST_INT(iChangedRestart, 0);

        // no change
        *pInt = 12;
        EZ_TEST_INT(iChangedValue, 1);
        EZ_TEST_INT(iChangedRestart, 0);
      }

      ezCVarFloat* pFloat = (ezCVarFloat*)ezCVar::FindCVarByName("test1_Float");
      CHECK_CVAR(pFloat, 1.1f, 1.1f, 1.1f, 1.1f);

      if (pFloat)
      {
        EZ_TEST_BOOL(pFloat->GetType() == ezCVarType::Float);
        EZ_TEST_BOOL(pFloat->GetName() == "test1_Float");
        EZ_TEST_BOOL(pFloat->GetDescription() == "Desc: test1_Float");

        pFloat->m_CVarEvents.AddEventHandler(ChangedCVar);

        *pFloat = 1.2f;
        CHECK_CVAR(pFloat, 1.1f, 1.1f, 1.1f, 1.2f);

        EZ_TEST_INT(iChangedValue, 1);
        EZ_TEST_INT(iChangedRestart, 1);

        // no change
        *pFloat = 1.2f;
        EZ_TEST_INT(iChangedValue, 1);
        EZ_TEST_INT(iChangedRestart, 1);

        pFloat->SetToDelayedSyncValue();
        CHECK_CVAR(pFloat, 1.2f, 1.1f, 1.1f, 1.2f);

        EZ_TEST_INT(iChangedValue, 2);
        EZ_TEST_INT(iChangedRestart, 1);
      }

      ezCVarBool* pBool = (ezCVarBool*)ezCVar::FindCVarByName("test1_Bool");
      CHECK_CVAR(pBool, false, false, false, false);

      if (pBool)
      {
        EZ_TEST_BOOL(pBool->GetType() == ezCVarType::Bool);
        EZ_TEST_BOOL(pBool->GetName() == "test1_Bool");
        EZ_TEST_BOOL(pBool->GetDescription() == "Desc: test1_Bool");

        *pBool = true;
        CHECK_CVAR(pBool, true, false, false, true);
      }

      ezCVarString* pString = (ezCVarString*)ezCVar::FindCVarByName("test1_String");
      CHECK_CVAR(pString, "test1", "test1", "test1", "test1");

      if (pString)
      {
        EZ_TEST_BOOL(pString->GetType() == ezCVarType::String);
        EZ_TEST_BOOL(pString->GetName() == "test1_String");
        EZ_TEST_BOOL(pString->GetDescription() == "Desc: test1_String");

        *pString = "test1_value2";
        CHECK_CVAR(pString, "test1_value2", "test1", "test1", "test1_value2");
      }
    }

    // CVars from Plugin 2
    {
      ezCVarInt* pInt = (ezCVarInt*)ezCVar::FindCVarByName("test2_Int");
      CHECK_CVAR(pInt, 22, 22, 22, 22);

      if (pInt)
      {
        pInt->m_CVarEvents.AddEventHandler(ChangedCVar);

        *pInt = 23;
        CHECK_CVAR(pInt, 23, 22, 22, 23);
        EZ_TEST_INT(iChangedValue, 3);
        EZ_TEST_INT(iChangedRestart, 1);
      }

      ezCVarFloat* pFloat = (ezCVarFloat*)ezCVar::FindCVarByName("test2_Float");
      CHECK_CVAR(pFloat, 2.2f, 2.2f, 2.2f, 2.2f);

      if (pFloat)
      {
        *pFloat = 2.3f;
        CHECK_CVAR(pFloat, 2.3f, 2.2f, 2.2f, 2.3f);
      }

      ezCVarBool* pBool = (ezCVarBool*)ezCVar::FindCVarByName("test2_Bool");
      CHECK_CVAR(pBool, true, true, true, true);

      if (pBool)
      {
        *pBool = false;
        CHECK_CVAR(pBool, false, true, true, false);
      }

      ezCVarString* pString = (ezCVarString*)ezCVar::FindCVarByName("test2_String");
      CHECK_CVAR(pString, "test2", "test2", "test2", "test2");

      if (pString)
      {
        *pString = "test2_value2";
        CHECK_CVAR(pString, "test2", "test2", "test2", "test2_value2");

        pString->SetToDelayedSyncValue();
        CHECK_CVAR(pString, "test2_value2", "test2", "test2", "test2_value2");
      }
    }

    ezPlugin::UnloadAllPlugins();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Loaded Value Test")
  {
    EZ_TEST_BOOL(ezPlugin::LoadPlugin(ezFoundationTest_Plugin2) == EZ_SUCCESS);

    // CVars from Plugin 1
    {
      ezCVarInt* pInt = (ezCVarInt*)ezCVar::FindCVarByName("test1_Int");
      CHECK_CVAR(pInt, 12, 11, 12, 12);

      ezCVarFloat* pFloat = (ezCVarFloat*)ezCVar::FindCVarByName("test1_Float");
      CHECK_CVAR(pFloat, 1.2f, 1.1f, 1.2f, 1.2f);

      ezCVarBool* pBool = (ezCVarBool*)ezCVar::FindCVarByName("test1_Bool");
      CHECK_CVAR(pBool, false, false, false, false);

      ezCVarString* pString = (ezCVarString*)ezCVar::FindCVarByName("test1_String");
      CHECK_CVAR(pString, "test1", "test1", "test1", "test1");
    }

    // CVars from Plugin 1, overridden by command line
    {
      ezCVarInt* pInt = (ezCVarInt*)ezCVar::FindCVarByName("test1_Int2");
      CHECK_CVAR(pInt, 102, 21, 102, 102);

      ezCVarFloat* pFloat = (ezCVarFloat*)ezCVar::FindCVarByName("test1_Float2");
      CHECK_CVAR(pFloat, 102.2f, 2.1f, 102.2f, 102.2f);

      ezCVarBool* pBool = (ezCVarBool*)ezCVar::FindCVarByName("test1_Bool2");
      CHECK_CVAR(pBool, false, true, false, false);

      ezCVarString* pString = (ezCVarString*)ezCVar::FindCVarByName("test1_String2");
      CHECK_CVAR(pString, "test1c", "test1b", "test1c", "test1c");
    }

    // CVars from Plugin 2
    {
      ezCVarInt* pInt = (ezCVarInt*)ezCVar::FindCVarByName("test2_Int");
      CHECK_CVAR(pInt, 22, 22, 22, 22);

      ezCVarFloat* pFloat = (ezCVarFloat*)ezCVar::FindCVarByName("test2_Float");
      CHECK_CVAR(pFloat, 2.2f, 2.2f, 2.2f, 2.2f);

      ezCVarBool* pBool = (ezCVarBool*)ezCVar::FindCVarByName("test2_Bool");
      CHECK_CVAR(pBool, false, true, false, false);

      ezCVarString* pString = (ezCVarString*)ezCVar::FindCVarByName("test2_String");
      CHECK_CVAR(pString, "test2_value2", "test2", "test2_value2", "test2_value2");
    }

    ezPlugin::UnloadAllPlugins();
  }

#endif

  ezFileSystem::ClearAllDataDirectories();
}
