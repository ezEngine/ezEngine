#pragma once

#include <ModelImporter2/Importer/Importer.h>

namespace ezModelImporter2
{
  /// Importer implementation to import Source engine BSP files.
  class ImporterMagicaVoxel : public Importer
  {
  public:
    ImporterMagicaVoxel();
    ~ImporterMagicaVoxel();

  protected:
    virtual ezResult DoImport() override;
  };
} // namespace ezModelImporter2
