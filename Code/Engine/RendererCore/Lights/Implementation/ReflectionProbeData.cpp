#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezReflectionProbeMode, 1)
  EZ_BITFLAGS_CONSTANTS(ezReflectionProbeMode::Static, ezReflectionProbeMode::Dynamic)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezProbeFlags, 1)
  EZ_BITFLAGS_CONSTANTS(ezProbeFlags::SkyLight, ezProbeFlags::HasCustomCubeMap, ezProbeFlags::Sphere, ezProbeFlags::Box, ezProbeFlags::Dynamic)
EZ_END_STATIC_REFLECTED_BITFLAGS;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezReflectionProbeRenderData, 1, ezRTTIDefaultAllocator<ezReflectionProbeRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionProbeData);
