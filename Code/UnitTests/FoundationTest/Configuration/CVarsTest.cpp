#include <FoundationTestPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Utilities/CommandLineUtils.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Configuration);

#define ezCVarValueDefault ezCVarValue::Default
#define ezCVarValueStored ezCVarValue::Stored
#define ezCVarValueRestart ezCVarValue::Restart

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

#if EZ_ENABLED(EZ_SUPPORTS_DYNAMIC_PLUGINS)

static void ChangedCVar(const ezCVarEvent& e)
{
  switch (e.m_EventType)
  {
    case ezCVarEvent::ValueChanged:
      ++iChangedValue;
      break;
    case ezCVarEvent::RestartValueChanged:
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

  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder1.GetData(), "test", "output", ezFileSystem::AllowWrites) == EZ_SUCCESS);

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

#if EZ_ENABLED(EZ_SUPPORTS_DYNAMIC_PLUGINS)

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

    EZ_TEST_BOOL(ezPlugin::UnloadPlugin(ezFoundationTest_Plugin1) == EZ_SUCCESS);
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

#if EZ_ENABLED(EZ_SUPPORTS_DYNAMIC_PLUGINS)

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

    EZ_TEST_BOOL(ezPlugin::UnloadPlugin(ezFoundationTest_Plugin2) == EZ_SUCCESS);
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

#if EZ_ENABLED(EZ_SUPPORTS_DYNAMIC_PLUGINS)

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
        EZ_TEST_BOOL(ezStringUtils::IsEqual(pInt->GetName(), "test1_Int"));
        EZ_TEST_BOOL(ezStringUtils::IsEqual(pInt->GetDescription(), "Desc: test1_Int"));

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
        EZ_TEST_BOOL(ezStringUtils::IsEqual(pFloat->GetName(), "test1_Float"));
        EZ_TEST_BOOL(ezStringUtils::IsEqual(pFloat->GetDescription(), "Desc: test1_Float"));

        pFloat->m_CVarEvents.AddEventHandler(ChangedCVar);

        *pFloat = 1.2f;
        CHECK_CVAR(pFloat, 1.1f, 1.1f, 1.1f, 1.2f);

        EZ_TEST_INT(iChangedValue, 1);
        EZ_TEST_INT(iChangedRestart, 1);

        // no change
        *pFloat = 1.2f;
        EZ_TEST_INT(iChangedValue, 1);
        EZ_TEST_INT(iChangedRestart, 1);

        pFloat->SetToRestartValue();
        CHECK_CVAR(pFloat, 1.2f, 1.1f, 1.1f, 1.2f);

        EZ_TEST_INT(iChangedValue, 2);
        EZ_TEST_INT(iChangedRestart, 1);
      }

      ezCVarBool* pBool = (ezCVarBool*)ezCVar::FindCVarByName("test1_Bool");
      CHECK_CVAR(pBool, false, false, false, false);

      if (pBool)
      {
        EZ_TEST_BOOL(pBool->GetType() == ezCVarType::Bool);
        EZ_TEST_BOOL(ezStringUtils::IsEqual(pBool->GetName(), "test1_Bool"));
        EZ_TEST_BOOL(ezStringUtils::IsEqual(pBool->GetDescription(), "Desc: test1_Bool"));

        *pBool = true;
        CHECK_CVAR(pBool, true, false, false, true);
      }

      ezCVarString* pString = (ezCVarString*)ezCVar::FindCVarByName("test1_String");
      CHECK_CVAR(pString, "test1", "test1", "test1", "test1");

      if (pString)
      {
        EZ_TEST_BOOL(pString->GetType() == ezCVarType::String);
        EZ_TEST_BOOL(ezStringUtils::IsEqual(pString->GetName(), "test1_String"));
        EZ_TEST_BOOL(ezStringUtils::IsEqual(pString->GetDescription(), "Desc: test1_String"));

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

        pString->SetToRestartValue();
        CHECK_CVAR(pString, "test2_value2", "test2", "test2", "test2_value2");
      }
    }

    EZ_TEST_BOOL(ezPlugin::UnloadPlugin(ezFoundationTest_Plugin2) == EZ_SUCCESS);
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

    EZ_TEST_BOOL(ezPlugin::UnloadPlugin(ezFoundationTest_Plugin2) == EZ_SUCCESS);
  }

#endif

  ezFileSystem::ClearAllDataDirectories();
}
