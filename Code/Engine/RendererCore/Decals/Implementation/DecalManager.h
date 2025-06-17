#pragma once

#include <RendererCore/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct ezRenderWorldExtractionEvent;
struct ezRenderWorldRenderEvent;
class ezView;

class EZ_RENDERERCORE_DLL ezDecalManager
{
public:
  static ezDecalId GetOrCreateRuntimeDecal(const ezTexture2DResourceHandle& hTexture);
  static ezDecalId GetOrCreateRuntimeDecal(const ezMaterialResourceHandle& hMaterial, ezUInt32 uiResolution, ezTime updateInterval);
  static void DeleteRuntimeDecal(ezDecalId& ref_decalId);

  /// \brief Marks the runtime decal as in use with the given screen space size by the given reference view. Should be called every frame.
  /// This is used to calculate how much space the decal needs in the atlas.
  static void MarkRuntimeDecalAsUsed(ezDecalId decalId, float fScreenSpaceSize, const ezView* pReferenceView);

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
