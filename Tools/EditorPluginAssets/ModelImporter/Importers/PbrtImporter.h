#pragma once

#include <EditorPluginAssets/ModelImporter/ImporterImplementation.h>

namespace ezModelImporter
{
  /// Importer implementation for Pbrt files.
  /// Not all aspects of the Pbrt file are supported!
  class PbrtImporter : public ImporterImplementation
  {
  public:
    PbrtImporter();
    ~PbrtImporter() {}

    virtual ezArrayPtr<const ezString> GetSupportedFileFormats() const override;
    virtual ezSharedPtr<Scene> ImportScene(const char* szFileName) override;
  };
}
