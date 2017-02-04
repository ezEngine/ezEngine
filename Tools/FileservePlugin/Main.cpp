#include <PCH.h>
#include <FileservePlugin/FileserveDataDir.h>

void OnLoadPlugin(bool bReloading)    { }
void OnUnloadPlugin(bool bReloading)  { }

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_FILESERVEPLUGIN_DLL, ezFileservePlugin);

EZ_BEGIN_SUBSYSTEM_DECLARATION(FileservePlugin, FileservePluginMain)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FileserveType::Factory, 100.0f);
  }

  ON_CORE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION


