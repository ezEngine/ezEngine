#include <PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <EditorPluginAssets/ModelImporter/ModelImporter.h>
#include <EditorPluginAssets/ModelImporter/Importers/AssimpImporter.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(Editor, PluginAssets)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    EZ_DEFAULT_NEW(ezModelImporter::Importer);
    ezModelImporter::Importer::GetSingleton()->AddImporterImplementation(EZ_DEFAULT_NEW(ezModelImporter::AssimpImporter));
  }

  ON_CORE_SHUTDOWN
  {
    auto ptr = ezModelImporter::Importer::GetSingleton();
    EZ_DEFAULT_DELETE(ptr);
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION
