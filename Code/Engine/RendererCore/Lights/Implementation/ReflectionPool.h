#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/Declarations.h>

class ezGALTextureHandle;
class ezGALBufferHandle;
class ezView;
class ezWorld;
class ezComponent;
struct ezRenderWorldExtractionEvent;
struct ezRenderWorldRenderEvent;
struct ezMsgExtractRenderData;
struct ezReflectionProbeDesc;
class ezReflectionProbeRenderData;
using ezReflectionProbeId = ezGenericId<24, 8>;
class ezReflectionProbeComponentBase;
class ezSkyLightComponent;

class EZ_RENDERERCORE_DLL ezReflectionPool
{
public:
  // Probes
  static ezReflectionProbeId RegisterReflectionProbe(const ezWorld* pWorld, const ezReflectionProbeDesc& desc, const ezReflectionProbeComponentBase* pComponent);
  static void DeregisterReflectionProbe(const ezWorld* pWorld, ezReflectionProbeId id);
  static void UpdateReflectionProbe(const ezWorld* pWorld, ezReflectionProbeId id, const ezReflectionProbeDesc& desc, const ezReflectionProbeComponentBase* pComponent);
  static void ExtractReflectionProbe(const ezComponent* pComponent, ezMsgExtractRenderData& ref_msg, ezReflectionProbeRenderData* pRenderData, const ezWorld* pWorld, ezReflectionProbeId id, float fPriority);

  // SkyLight
  static ezReflectionProbeId RegisterSkyLight(const ezWorld* pWorld, ezReflectionProbeDesc& ref_desc, const ezSkyLightComponent* pComponent);
  static void DeregisterSkyLight(const ezWorld* pWorld, ezReflectionProbeId id);
  static void UpdateSkyLight(const ezWorld* pWorld, ezReflectionProbeId id, const ezReflectionProbeDesc& desc, const ezSkyLightComponent* pComponent);


  static void SetConstantSkyIrradiance(const ezWorld* pWorld, const ezAmbientCube<ezColor>& skyIrradiance);
  static void ResetConstantSkyIrradiance(const ezWorld* pWorld);

  static ezUInt32 GetReflectionCubeMapSize();
  static ezGALTextureHandle GetReflectionSpecularTexture(ezUInt32 uiWorldIndex, ezEnum<ezCameraUsageHint> cameraUsageHint);
  static ezGALTextureHandle GetSkyIrradianceTexture();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, ReflectionPool);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static void OnExtractionEvent(const ezRenderWorldExtractionEvent& e);
  static void OnRenderEvent(const ezRenderWorldRenderEvent& e);

  struct Data;
  static Data* s_pData;
};
