#include <PCH.h>
#include <FileservePlugin/Client/FileserveDataDir.h>

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

    if (ezStartup::HasApplicationTag("tool") ||
        ezStartup::HasApplicationTag("testframework"))
      return;

    if (ezFileserveClient::GetSingleton() == nullptr)
    {
      EZ_DEFAULT_NEW(ezFileserveClient);
    }
  }

  ON_CORE_SHUTDOWN
  {
    if (ezStartup::HasApplicationTag("tool") ||
        ezStartup::HasApplicationTag("testframework"))
      return;

    if (ezFileserveClient::GetSingleton() != nullptr)
    {
      ezFileserveClient* pSingleton = ezFileserveClient::GetSingleton();
      EZ_DEFAULT_DELETE(pSingleton);
    }
  }

EZ_END_SUBSYSTEM_DECLARATION


