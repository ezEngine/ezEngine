#pragma once

#include <ModelImporter2/Importer/Importer.h>

namespace ezModelImporter2
{
  /// Importer implementation to import Source engine BSP files.
  class ImporterSourceBSP : public Importer
  {
  public:
    ImporterSourceBSP();
    ~ImporterSourceBSP();

  protected:
    virtual ezResult DoImport() override;
  };
} // namespace ezModelImporter2
