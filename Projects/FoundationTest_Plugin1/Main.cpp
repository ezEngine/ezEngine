#include <PCH.h>

void OnLoadPlugin();
void OnUnloadPlugin();

ezPlugin g_Plugin(true, OnLoadPlugin, OnUnloadPlugin);

ezCVarInt     CVar_TestInt    ("test1_Int",   11,   ezCVarFlags::Save, "Desc: test1_Int");
ezCVarFloat   CVar_TestFloat  ("test1_Float", 1.1f, ezCVarFlags::RequiresRestart, "Desc: test1_Float");
ezCVarBool    CVar_TestBool   ("test1_Bool",  false, ezCVarFlags::None, "Desc: test1_Bool");
ezCVarString  CVar_TestString ("test1_String", "test1", ezCVarFlags::Default, "Desc: test1_String");

void OnLoadPlugin()
{
  ezCVarInt* pCVar = (ezCVarInt*) ezCVar::FindCVarByName("TestPlugin1InitCount");
  *pCVar = *pCVar + 1;
}

void OnUnloadPlugin()
{
  ezCVarInt* pCVar = (ezCVarInt*) ezCVar::FindCVarByName("TestPlugin1UninitCount");
  *pCVar = *pCVar + 1;
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


