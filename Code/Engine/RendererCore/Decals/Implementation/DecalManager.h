#pragma once

#include <RendererCore/Declarations.h>

struct ezRenderWorldExtractionEvent;
struct ezRenderWorldRenderEvent;
class ezView;

class EZ_RENDERERCORE_DLL ezDecalManager
{
public:
  static ezDecalId GetOrAddRuntimeDecal(ezTexture2DResourceHandle hTexture, float fScreenSpaceSize, const ezView* pReferenceView, ezTime inactiveTimeBeforeAutoRemove = ezTime::MakeFromSeconds(1));
  static ezDecalId GetOrAddRuntimeDecal(ezMaterialResourceHandle hMaterial, ezUInt32 uiResolution, ezTime updateInterval, float fScreenSpaceSize, const ezView* pReferenceView, ezTime inactiveTimeBeforeAutoRemove = ezTime::MakeFromSeconds(1));
  static void RemoveRuntimeDecal(ezDecalId decalId);

  static ezDecalAtlasResourceHandle GetBakedDecalAtlas();
  static ezGALTextureHandle GetRuntimeDecalAtlasTexture();

  static ezGALBufferHandle GetDecalAtlasDataBufferForRendering();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, DecalManager);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static void OnExtractionEvent(const ezRenderWorldExtractionEvent& e);
  static void OnRenderEvent(const ezRenderWorldRenderEvent& e);

  struct Data;
  static Data* s_pData;
};
