#include <PCH.h>

void OnLoadPlugin();
void OnUnloadPlugin();

ezPlugin g_Plugin(true, OnLoadPlugin, OnUnloadPlugin, "FoundationTest_Plugin1");

ezCVarInt     CVar_TestInt    ("test2_Int",   22,   ezCVarFlags::None, "Desc: test2_Int");
ezCVarFloat   CVar_TestFloat  ("test2_Float", 2.2f, ezCVarFlags::Default, "Desc: test2_Float");
ezCVarBool    CVar_TestBool   ("test2_Bool",  true, ezCVarFlags::Save, "Desc: test2_Bool");
ezCVarString  CVar_TestString ("test2_String", "test2", ezCVarFlags::RequiresRestart, "Desc: test2_String");

void OnLoadPlugin()
{
  ezCVarInt* pCVar = (ezCVarInt*) ezCVar::FindCVarByName("TestPlugin2InitCount");
  *pCVar = *pCVar + 1;
}

void OnUnloadPlugin()
{
  ezCVarInt* pCVar = (ezCVarInt*) ezCVar::FindCVarByName("TestPlugin2UninitCount");
  *pCVar = *pCVar + 1;
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



