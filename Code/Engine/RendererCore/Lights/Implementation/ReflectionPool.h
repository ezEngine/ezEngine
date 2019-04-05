#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <RendererCore/Declarations.h>

class ezGALTextureHandle;
class ezGALBufferHandle;
class ezView;
class ezWorld;
class ezComponent;
struct ezMsgExtractRenderData;
struct ezReflectionProbeData;

class EZ_RENDERERCORE_DLL ezReflectionPool
{
public:
  static void RegisterReflectionProbe(ezReflectionProbeData& data, ezWorld* pWorld, float fPriority = 1.0f);
  static void DeregisterReflectionProbe(ezReflectionProbeData& data, ezWorld* pWorld);

  static void ExtractReflectionProbe(
    ezMsgExtractRenderData& msg, const ezReflectionProbeData& data, const ezComponent* pComponent, float fPriority = 1.0f);

  static void SetConstantSkyIrradiance(const ezWorld* pWorld, const ezAmbientCube<ezColor>& skyIrradiance);

  static ezUInt32 GetReflectionCubeMapSize();
  static ezGALTextureHandle GetReflectionSpecularTexture();
  static ezGALTextureHandle GetSkyIrradianceTexture();

  // static ezGALBufferHandle GetShadowDataBuffer();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, ReflectionPool);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static void OnBeginExtraction(ezUInt64 uiFrameCounter);
  static void OnBeginRender(ezUInt64 uiFrameCounter);

  struct Data;
  static Data* s_pData;
};
