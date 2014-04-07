#include <PCH.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>

static ezInt32 g_iPluginState = -1;

void OnLoadPlugin(bool bReloading);
void OnUnloadPlugin(bool bReloading);

ezPlugin g_Plugin(true, OnLoadPlugin, OnUnloadPlugin, "ezFoundationTest_Plugin1");

ezCVarInt     CVar_TestInt    ("test2_Int",   22,   ezCVarFlags::None, "Desc: test2_Int");
ezCVarFloat   CVar_TestFloat  ("test2_Float", 2.2f, ezCVarFlags::Default, "Desc: test2_Float");
ezCVarBool    CVar_TestBool   ("test2_Bool",  true, ezCVarFlags::Save, "Desc: test2_Bool");
ezCVarString  CVar_TestString ("test2_String", "test2", ezCVarFlags::RequiresRestart, "Desc: test2_String");

ezCVarBool    CVar_TestInited ("test2_Inited", false, ezCVarFlags::None, "");

void OnLoadPlugin(bool bReloading)
{
  EZ_TEST_BOOL_MSG(g_iPluginState == -1, "Plugin is in an invalid state.");
  g_iPluginState = 1;

  EZ_TEST_BOOL(ezPlugin::FindPluginByName("ezFoundationTest_Plugin1") != nullptr); // dependency is already loaded
  EZ_TEST_BOOL(ezPlugin::FindPluginByName("ezFoundationTest_Plugin2") != nullptr); // should find itself

  ezCVarInt* pCVar = (ezCVarInt*) ezCVar::FindCVarByName("TestPlugin2InitCount");

  if (pCVar)
    *pCVar = *pCVar + 1;

  if (bReloading)
  {
    ezCVarInt* pCVarReload = (ezCVarInt*) ezCVar::FindCVarByName("TestPlugin2Reloaded");

    if (pCVarReload)
      *pCVarReload = *pCVarReload + 1;
  }

  ezCVarBool* pCVarDep = (ezCVarBool*) ezCVar::FindCVarByName("TestPlugin2FoundDependencies");

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

void OnUnloadPlugin(bool bReloading)
{
  EZ_TEST_BOOL_MSG(g_iPluginState == 1, "Plugin is in an invalid state.");
  g_iPluginState = 2;

  ezCVarInt* pCVar = (ezCVarInt*) ezCVar::FindCVarByName("TestPlugin2UninitCount");

  if (pCVar)
    *pCVar = *pCVar + 1;

  if (bReloading)
  {
    ezCVarInt* pCVarReload = (ezCVarInt*) ezCVar::FindCVarByName("TestPlugin2Reloaded");

    if (pCVarReload)
      *pCVarReload = *pCVarReload + 1;
  }

  ezCVarBool* pCVarDep = (ezCVarBool*) ezCVar::FindCVarByName("TestPlugin2FoundDependencies");

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

EZ_BEGIN_SUBSYSTEM_DECLARATION(PluginGroup_Plugin2, TestSubSystem2)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "PluginGroup_Plugin1"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
  }

  ON_CORE_SHUTDOWN
  {
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION



