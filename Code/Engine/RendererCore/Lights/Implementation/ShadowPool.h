#pragma once

#include <RendererCore/Declarations.h>

class ezDirectionalLightComponent;
class ezPointLightComponent;
class ezSpotLightComponent;
class ezGALTextureHandle;
class ezGALBufferHandle;

class EZ_RENDERERCORE_DLL ezShadowPool
{
public:

  static ezUInt32 AddDirectionalLight(const ezDirectionalLightComponent* pDirLight, const ezView* pReferenceView);
  static ezUInt32 AddPointLight(const ezPointLightComponent* pPointLight, float fScreenSpaceSize);
  static ezUInt32 AddSpotLight(const ezSpotLightComponent* pSpotLight, float fScreenSpaceSize);

  static ezGALTextureHandle GetShadowAtlasTexture();
  static ezGALBufferHandle UpdateShadowDataBuffer(ezGALContext* pGALContext);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, ShadowPool);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static void OnBeginFrame(ezUInt64 uiFrameNumber);
};

