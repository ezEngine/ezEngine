#include <PCH.h>
#include <TestFramework/Framework/TestFramework.h>

static ezInt32 g_iPluginState = -1;

void OnLoadPlugin(bool bReloading);
void OnUnloadPlugin(bool bReloading);

ezPlugin g_Plugin(true, OnLoadPlugin, OnUnloadPlugin);

ezCVarInt     CVar_TestInt    ("test1_Int",   11,   ezCVarFlags::Save, "Desc: test1_Int");
ezCVarFloat   CVar_TestFloat  ("test1_Float", 1.1f, ezCVarFlags::RequiresRestart, "Desc: test1_Float");
ezCVarBool    CVar_TestBool   ("test1_Bool",  false, ezCVarFlags::None, "Desc: test1_Bool");
ezCVarString  CVar_TestString ("test1_String", "test1", ezCVarFlags::Default, "Desc: test1_String");

void OnLoadPlugin(bool bReloading)
{
  EZ_TEST_MSG(g_iPluginState == -1, "Plugin is in an invalid state.");
  g_iPluginState = 1;

  EZ_TEST(ezPlugin::FindPluginByName("FoundationTest_Plugin1") != NULL); // should find itself

  ezCVarInt* pCVar = (ezCVarInt*) ezCVar::FindCVarByName("TestPlugin1InitCount");
  *pCVar = *pCVar + 1;

  if (bReloading)
  {
    ezCVarInt* pCVarReload = (ezCVarInt*) ezCVar::FindCVarByName("TestPlugin1Reloaded");
    *pCVarReload = *pCVarReload + 1;
  }

  ezCVarBool* pCVarPlugin2Inited = (ezCVarBool*) ezCVar::FindCVarByName("test2_Inited");
  if (pCVarPlugin2Inited)
  {
    EZ_TEST(*pCVarPlugin2Inited == false); // Although Plugin2 is present, it should not yet have been initialized
  }
}

void OnUnloadPlugin(bool bReloading)
{
  EZ_TEST_MSG(g_iPluginState == 1, "Plugin is in an invalid state.");
  g_iPluginState = 2;

  ezCVarInt* pCVar = (ezCVarInt*) ezCVar::FindCVarByName("TestPlugin1UninitCount");
  *pCVar = *pCVar + 1;

  if (bReloading)
  {
    ezCVarInt* pCVarReload = (ezCVarInt*) ezCVar::FindCVarByName("TestPlugin1Reloaded");
    *pCVarReload = *pCVarReload + 1;
  }
}



EZ_BEGIN_SUBSYSTEM_DECLARATION(PluginGroup_Plugin1, TestSubSystem1)

  //BEGIN_SUBSYSTEM_DEPENDENCIES
  //  "PluginGroup_Plugin1"
  //END_SUBSYSTEM_DEPENDENCIES

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


