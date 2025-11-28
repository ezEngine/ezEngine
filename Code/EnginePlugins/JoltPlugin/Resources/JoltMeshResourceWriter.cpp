#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/Resources/JoltMeshResourceWriter.h>
#include <Foundation/Configuration/Singleton.h>

// static
ezResult ezJoltMeshResourceWriterInterface::WriteMeshResource(ezJoltMeshDesc&& meshDesc, ezStreamWriter& inout_stream, ezUInt64 uiAssetHash /*= 0*/)
{
  if (auto pWriter = ezSingletonRegistry::GetSingletonInstance<ezJoltMeshResourceWriterInterface>())
  {
    return pWriter->WriteMeshResourceInternal(std::move(meshDesc), inout_stream, uiAssetHash);
  }

  return EZ_FAILURE;
}
