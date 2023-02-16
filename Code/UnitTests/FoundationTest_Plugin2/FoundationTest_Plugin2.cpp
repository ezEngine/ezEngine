#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/ConstructionCounter.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>

static ezInt32 g_iPluginState = -1;

void OnLoadPlugin();
void OnUnloadPlugin();

EZ_PLUGIN_DEPENDENCY(ezFoundationTest_Plugin1);

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

ezCVarInt CVar_TestInt("test2_Int", 22, ezCVarFlags::None, "Desc: test2_Int");
ezCVarFloat CVar_TestFloat("test2_Float", 2.2f, ezCVarFlags::Default, "Desc: test2_Float");
ezCVarBool CVar_TestBool("test2_Bool", true, ezCVarFlags::Save, "Desc: test2_Bool");
ezCVarString CVar_TestString("test2_String", "test2", ezCVarFlags::RequiresRestart, "Desc: test2_String");

ezCVarBool CVar_TestInited("test2_Inited", false, ezCVarFlags::None, "Desc: test2_Inited");

void OnLoadPlugin()
{
  EZ_TEST_BOOL_MSG(g_iPluginState == -1, "Plugin is in an invalid state.");
  g_iPluginState = 1;

  ezCVarInt* pCVar = (ezCVarInt*)ezCVar::FindCVarByName("TestPlugin2InitCount");

  if (pCVar)
    *pCVar = *pCVar + 1;

  ezCVarBool* pCVarDep = (ezCVarBool*)ezCVar::FindCVarByName("TestPlugin2FoundDependencies");

  if (pCVarDep)
  {
    *pCVarDep = true;

    // check that all CVars from plugin1 are available (ie. plugin1 is already loaded)
    *pCVarDep = *pCVarDep && (ezCVar::FindCVarByName("test1_Int") != nullptr);
    *pCVarDep = *pCVarDep && (ezCVar::FindCVarByName("test1_Float") != nullptr);
    *pCVarDep = *pCVarDep && (ezCVar::FindCVarByName("test1_Bool") != nullptr);
    *pCVarDep = *pCVarDep && (ezCVar::FindCVarByName("test1_String") != nullptr);
  }

  CVar_TestInited = true;
}

void OnUnloadPlugin()
{
  EZ_TEST_BOOL_MSG(g_iPluginState == 1, "Plugin is in an invalid state.");
  g_iPluginState = 2;

  ezCVarInt* pCVar = (ezCVarInt*)ezCVar::FindCVarByName("TestPlugin2UninitCount");

  if (pCVar)
    *pCVar = *pCVar + 1;

  ezCVarBool* pCVarDep = (ezCVarBool*)ezCVar::FindCVarByName("TestPlugin2FoundDependencies");

  if (pCVarDep)
  {
    *pCVarDep = true;

    // check that all CVars from plugin1 are STILL available (ie. plugin1 is not yet unloaded)
    *pCVarDep = *pCVarDep && (ezCVar::FindCVarByName("test1_Int") != nullptr);
    *pCVarDep = *pCVarDep && (ezCVar::FindCVarByName("test1_Float") != nullptr);
    *pCVarDep = *pCVarDep && (ezCVar::FindCVarByName("test1_Bool") != nullptr);
    *pCVarDep = *pCVarDep && (ezCVar::FindCVarByName("test1_String") != nullptr);
  }

  CVar_TestInited = false;
}

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(PluginGroup_Plugin2, TestSubSystem2)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "PluginGroup_Plugin1"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on
