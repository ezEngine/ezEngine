#include <RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>
#include <RendererCore/Lights/ProbeTreeSectorResource.h>

ezProbeTreeSectorResourceDescriptor::ezProbeTreeSectorResourceDescriptor()
{
}

ezProbeTreeSectorResourceDescriptor::~ezProbeTreeSectorResourceDescriptor()
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProbeTreeSectorResource, 1, ezRTTIDefaultAllocator<ezProbeTreeSectorResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezProbeTreeSectorResource);
// clang-format on

ezProbeTreeSectorResource::ezProbeTreeSectorResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

ezProbeTreeSectorResource::~ezProbeTreeSectorResource() = default;

ezResourceLoadDesc ezProbeTreeSectorResource::UnloadData(Unload WhatToUnload)
{
  return ezResourceLoadDesc();
}

ezResourceLoadDesc ezProbeTreeSectorResource::UpdateContent(ezStreamReader* Stream)
{
  return ezResourceLoadDesc();
}

void ezProbeTreeSectorResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
}
