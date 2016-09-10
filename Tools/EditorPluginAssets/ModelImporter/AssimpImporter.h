#pragma once

#include <EditorPluginAssets/ModelImporter/ImporterImplementation.h>

namespace ezModelImporter
{
  /// Importer implementatoin based on assimp.
  class AssimpImporter : public ImporterImplementation
  {
  public:
    AssimpImporter();
    ~AssimpImporter() {}

    virtual ezArrayPtr<const ezString> GetSupportedFileFormats() const override;
    virtual ezUniquePtr<Scene> ImportScene(const char* szFileName) override;

  private:
    ezDynamicArray<ezString> m_supportedFileFormats;
  };
}