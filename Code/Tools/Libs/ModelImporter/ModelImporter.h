#pragma once

#include <ModelImporter/Scene.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Types/Status.h>

class ezEditableSkeleton;

namespace ezModelImporter
{
  class ImporterImplementation;

  /// Singleton to import models.
  class EZ_MODELIMPORTER_DLL Importer
  {
    EZ_DECLARE_SINGLETON(Importer);

  public:
    Importer();
    ~Importer();

    /// Imports a scene from given file using the first available ImporterImplementation that supports the given file.
    ///
    /// \param addToCache
    ///   If true, the scene will be kept in the Importer's cache. Note that the cache needs be cleared manually by calling ClearCachedScenes.
    /// \returns
    ///   Null if something went wrong (see log).
    /// \see ClearCachedScenes
    ezSharedPtr<Scene> ImportScene(const char* szFileName, ezBitflags<ImportFlags> importFlags, bool addToCache = false);

    /// Clears list of cached scenes.
    /// \see ImportScene
    void ClearCachedScenes();

    /// Adds a importer implementation.
    ///
    /// Importers added later have a higher priority than those added earlier.
    /// This means that more general importer should be added earlier and more specialized later.
    /// Importer takes ownership of the importer object.
    void AddImporterImplementation(ezUniquePtr<ImporterImplementation> importerImplementation);

    /// Returns a set of all supported types.
    ezHashSet<ezString> GetSupportedTypes();

    /// \brief Utility function that imports the given scene. If szSubMesh is set, only the sub-mesh with that name is extracted.
    ///
    /// If szSubMesh is empty, the entire scene is merged into one big mesh and returned.
    ezStatus ImportMesh(const char* szSceneFile, const char* szSubMesh, bool bSkinnedMesh, ezSharedPtr<ezModelImporter::Scene>& outScene,
                        ezModelImporter::Mesh*& outMesh);

    static ezStatus ImportSkeleton(ezEditableSkeleton& out_Skeleton, const ezSharedPtr<Scene>& scene);

  private:
    ezHybridArray<ezUniquePtr<ImporterImplementation>, 4> m_ImporterImplementations;
    ezHashTable<ezString, ezSharedPtr<Scene>> m_cachedScenes;
  };
};
