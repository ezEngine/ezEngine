#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/CVar.h>

ezCVarInt CVar_TestPlugin1InitializedCount("TestPlugin1InitCount", 0, ezCVarFlags::None, "How often Plugin1 has been initialized.");
ezCVarInt CVar_TestPlugin1UninitializedCount("TestPlugin1UninitCount", 0, ezCVarFlags::None, "How often Plugin1 has been uninitialized.");
ezCVarInt CVar_TestPlugin1Reloaded("TestPlugin1Reloaded", 0, ezCVarFlags::None, "How often Plugin1 has been reloaded (counts init AND de-init).");

ezCVarInt CVar_TestPlugin2InitializedCount("TestPlugin2InitCount", 0, ezCVarFlags::None, "How often Plugin2 has been initialized.");
ezCVarInt CVar_TestPlugin2UninitializedCount("TestPlugin2UninitCount", 0, ezCVarFlags::None, "How often Plugin2 has been uninitialized.");
ezCVarInt CVar_TestPlugin2Reloaded("TestPlugin2Reloaded", 0, ezCVarFlags::None, "How often Plugin2 has been reloaded (counts init AND de-init).");
ezCVarBool CVar_TestPlugin2FoundDependencies("TestPlugin2FoundDependencies", false, ezCVarFlags::None, "Whether Plugin2 found all its dependencies (other plugins).");

EZ_CREATE_SIMPLE_TEST(Configuration, Plugin)
{
  CVar_TestPlugin1InitializedCount = 0;
  CVar_TestPlugin1UninitializedCount = 0;
  CVar_TestPlugin1Reloaded = 0;
  CVar_TestPlugin2InitializedCount = 0;
  CVar_TestPlugin2UninitializedCount = 0;
  CVar_TestPlugin2Reloaded = 0;
  CVar_TestPlugin2FoundDependencies = false;

#if EZ_ENABLED(EZ_SUPPORTS_DYNAMIC_PLUGINS)

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "LoadPlugin")
  {
    EZ_TEST_BOOL(ezPlugin::LoadPlugin(ezFoundationTest_Plugin2) == EZ_SUCCESS);
    EZ_TEST_BOOL(ezPlugin::LoadPlugin(ezFoundationTest_Plugin2, ezPluginLoadFlags::PluginIsOptional) == EZ_SUCCESS); // loading already loaded plugin is always a success

    EZ_TEST_INT(CVar_TestPlugin1InitializedCount, 1);
    EZ_TEST_INT(CVar_TestPlugin2InitializedCount, 1);

    EZ_TEST_INT(CVar_TestPlugin1UninitializedCount, 0);
    EZ_TEST_INT(CVar_TestPlugin2UninitializedCount, 0);

    EZ_TEST_INT(CVar_TestPlugin1Reloaded, 0);
    EZ_TEST_INT(CVar_TestPlugin2Reloaded, 0);

    EZ_TEST_BOOL(CVar_TestPlugin2FoundDependencies);

    // this will fail the FoundationTests, as it logs an error
    // EZ_TEST_BOOL(ezPlugin::LoadPlugin("Test") == EZ_FAILURE); // plugin does not exist
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "UnloadPlugin")
  {
    CVar_TestPlugin2FoundDependencies = false;
    ezPlugin::UnloadAllPlugins();

    EZ_TEST_INT(CVar_TestPlugin1InitializedCount, 1);
    EZ_TEST_INT(CVar_TestPlugin2InitializedCount, 1);

    EZ_TEST_INT(CVar_TestPlugin1UninitializedCount, 1);
    EZ_TEST_INT(CVar_TestPlugin2UninitializedCount, 1);

    EZ_TEST_INT(CVar_TestPlugin1Reloaded, 0);
    EZ_TEST_INT(CVar_TestPlugin2Reloaded, 0);

    EZ_TEST_BOOL(CVar_TestPlugin2FoundDependencies);
    EZ_TEST_BOOL(ezPlugin::LoadPlugin("Test", ezPluginLoadFlags::PluginIsOptional) == EZ_FAILURE); // plugin does not exist
  }

#endif
}
