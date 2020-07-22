#pragma once

#include <ModelImporter/ImporterImplementation.h>

namespace ezModelImporter
{
  /// Importer implementation to import Source engine BSP files.
  class EZ_MODELIMPORTER_DLL SourceBSPImporter : public ImporterImplementation
  {
  public:
    SourceBSPImporter();
    ~SourceBSPImporter() {}

    virtual ezArrayPtr<const ezString> GetSupportedFileFormats() const override;
    virtual ezSharedPtr<Scene> ImportScene(const char* szFileName, ezBitflags<ImportFlags> importFlags) override;

  private:
    ezDynamicArray<ezString> m_supportedFileFormats;
  };
} // namespace ezModelImporter
