#pragma once

#include <ModelImporter/ImporterImplementation.h>

namespace ezModelImporter
{
  /// Importer implementation based on assimp.
  class EZ_MODELIMPORTER_DLL AssimpImporter : public ImporterImplementation
  {
  public:
    AssimpImporter();
    ~AssimpImporter() {}

    virtual ezArrayPtr<const ezString> GetSupportedFileFormats() const override;
    virtual ezSharedPtr<Scene> ImportScene(const char* szFileName) override;

  private:
    ezDynamicArray<ezString> m_supportedFileFormats;
  };
}
