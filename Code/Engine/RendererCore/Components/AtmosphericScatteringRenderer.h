#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/Renderer.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <Shaders/Atmosphere/AtmosphereConstants.h>

class ezRenderDataBatch;
struct ezRenderWorldRenderEvent;
using ezShaderResourceHandle = ezTypedResourceHandle<class ezShaderResource>;

/// Render data for ezAtmosphericScatteringComponent.
///
/// Holds a copy of all shader parameters and the lookup tables to read from. The lookup tables are owned by ezAtmosphericScatteringLUTs.
class EZ_RENDERERCORE_DLL ezAtmosphericScatteringRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAtmosphericScatteringRenderData, ezRenderData);

public:
  ezAtmosphereConstants m_Constants;
  ezGALTextureHandle m_hTransmittanceLUT;
  ezGALTextureHandle m_hSkyViewLUT;
};

/// Owns and updates the lookup tables used to render atmospheric scattering.
///
/// Each atmosphere (identified by an arbitrary key, typically the component) gets its own set of lookup tables.
/// The transmittance and multi-scattering tables only depend on the atmosphere parameters, the sky-view table additionally
/// depends on the sun direction. Tables are only recomputed when their inputs change, on the GPU, right before the frame
/// in which they are used gets rendered.
///
/// Tables that haven't been requested for a few frames are deleted automatically, so there is no explicit release.
class EZ_RENDERERCORE_DLL ezAtmosphericScatteringLUTs
{
public:
  struct LUTs
  {
    ezGALTextureHandle m_hTransmittance;
    ezGALTextureHandle m_hSkyView;
  };

  /// Returns the lookup tables for the given key and schedules an update, if the constants differ from the last request.
  ///
  /// Meant to be called during render data extraction, every frame in which the atmosphere is rendered.
  /// Thread safe. Only the parameters that affect the lookup tables are compared, so changes to e.g. the sun intensity
  /// don't trigger an update.
  static LUTs Request(ezUInt64 uiKey, const ezAtmosphereConstants& constants);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, AtmosphericScatteringLUTs);

  static void OnEngineStartup();
  static void OnEngineShutdown();
  static void OnRenderEvent(const ezRenderWorldRenderEvent& e);
};

/// Renders ezAtmosphericScatteringRenderData as a fullscreen triangle at the far plane, using the precomputed sky-view lookup table.
class EZ_RENDERERCORE_DLL ezAtmosphericScatteringRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAtmosphericScatteringRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAtmosphericScatteringRenderer);

public:
  ezAtmosphericScatteringRenderer();
  ~ezAtmosphericScatteringRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezDynamicArray<const ezRTTI*>& out_types) const override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

private:
  ezShaderResourceHandle m_hShader;
  ezConstantBufferStorageHandle m_hConstants;
};
