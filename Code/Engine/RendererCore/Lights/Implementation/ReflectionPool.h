#pragma once

#include <RendererCore/Declarations.h>

class ezGALTextureHandle;
class ezGALBufferHandle;
class ezView;
struct ezMsgExtractRenderData;
struct ezReflectionProbeData;

class EZ_RENDERERCORE_DLL ezReflectionPool
{
public:
  static void RegisterReflectionProbe(ezReflectionProbeData& data, ezWorld* pWorld, float fPriority = 1.0f);
  static void DeregisterReflectionProbe(ezReflectionProbeData& data);

  static void ExtractReflectionProbe(ezMsgExtractRenderData& msg, const ezReflectionProbeData& data, const ezComponent* pComponent, float fPriority = 1.0f);

  static ezUInt32 GetReflectionCubeMapSize();
  static ezGALTextureHandle GetReflectionSpecularTexture();
  // static ezGALBufferHandle GetShadowDataBuffer();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, ReflectionPool);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static void OnBeginExtraction(ezUInt64 uiFrameCounter);
  static void OnEndExtraction(ezUInt64 uiFrameCounter);

  struct Data;
  static Data* s_pData;
};
