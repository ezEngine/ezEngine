#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <ModelImporter2/Importer/Importer.h>

namespace ezModelImporter2
{
  EZ_MODELIMPORTER2_DLL ezUniquePtr<Importer> RequestImporterForFileType(const char* szFile);

} // namespace ezModelImporter2
