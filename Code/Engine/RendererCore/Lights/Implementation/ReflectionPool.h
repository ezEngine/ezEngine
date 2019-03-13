#pragma once

#include <RendererCore/Declarations.h>

class ezGALTextureHandle;
class ezGALBufferHandle;
class ezView;
struct ezReflectionProbeData;

class EZ_RENDERERCORE_DLL ezReflectionPool
{
public:

  static void AddReflectionProbe(const ezReflectionProbeData& data, float fPriority = 0.0f);

  //static ezGALTextureHandle GetReflectionPoolTexture();
  //static ezGALBufferHandle GetShadowDataBuffer();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, ReflectionPool);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static void OnEndExtraction(ezUInt64);
  //static void OnBeginRender(ezUInt64);
};

