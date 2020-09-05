#include <ModelImporterPCH.h>

#include <ModelImporter2/ImporterAssimp/ImporterAssimp.h>
#include <ModelImporter2/ModelImporter.h>

namespace ezModelImporter2
{

  ezUniquePtr<Importer> RequestImporterForFileType(const char* szFile)
  {
    ezStringBuilder sFile = szFile;

    if (sFile.HasExtension(".fbx") || sFile.HasExtension(".obj") || sFile.HasExtension(".gltf") || sFile.HasExtension(".glb") || sFile.HasExtension(".blend"))
    {
      return EZ_DEFAULT_NEW(ImporterAssimp);
    }

    return nullptr;
  }


} // namespace ezModelImporter2
