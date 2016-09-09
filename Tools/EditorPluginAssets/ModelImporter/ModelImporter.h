#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <EditorPluginAssets/ModelImporter/Scene.h>

namespace ezModelImporter
{
  class ImporterImplementation;

  /// Singleton to import models.
  class Importer
  {
    EZ_DECLARE_SINGLETON(ezModelImporter::Importer);
  public:
    Importer();

    /// Imports a scene from given file using the first available ImporterImplementation that supports the given file.
    /// 
    /// \returns
    ///   Null if something went wrong (see log).
    ezUniquePtr<Scene> ImportScene(const char* szFileName);

    /// Adds a importer implementation.
    /// 
    /// Importers added later have a higher priority than those added earlier.
    /// This means that more general importer should be added earlier and more specialized later.
    /// Importer takes ownership of the importer object.
    void AddImporterImplementation(ezUniquePtr<ImporterImplementation> importerImplementation);

  private:
    ezHybridArray<ezUniquePtr<ImporterImplementation>, 4> m_ImporterImplementations;
  };
};
