#pragma once

#include <Foundation/Types/UniquePtr.h>

class ezStreamReader;

namespace ezModelImporter
{
  class Scene;

  /// A importer implementation does all the heavy lifting of loading a file into ezModelImporter::Scene
  ///
  /// Importer implementatoins should be registerd at ezModelImporter::Importer.
  /// \see ezModelImporter::Importer
  class ImporterImplementation
  {
  public:
    /// Returns a list of all supported file formats for this importer.
    ///
    /// File endings without dot, e.g. "obj", "fbx".
    virtual ezArrayPtr<const ezString> GetSupportedFileFormats() const = 0;

    /// Tries to import a scene from a source file.
    ///
    /// Some formats have dependent files. This makes it difficult to pass a stream reader instead of a filepath.
    /// \returns
    ///   Null if something went wrong (see log).
    virtual ezUniquePtr<Scene> ImportScene(const char* szFileName) = 0;
  };
}