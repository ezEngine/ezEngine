#pragma once

#include <Foundation/Math/Color16f.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/ParticleSystemConstants.h>

class ezGALBufferPool;
class ezRenderContext;

/// \brief Implements rendering of particle systems
///
/// Base class for all particle renderers. Provides common functionality for uploading
/// particle data to the GPU and binding shaders with particle system constants.
class EZ_PARTICLEPLUGIN_DLL ezParticleRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezParticleRenderer);

public:
  ezParticleRenderer();
  ~ezParticleRenderer();

  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const override;

protected:
  /// Helper for managing per-particle-system constant buffer data during rendering.
  ///
  /// Automatically binds the constant buffer on construction and unbinds on destruction.
  /// Provides methods to fill in system-specific rendering parameters.
  struct TempSystemCB
  {
    TempSystemCB(ezRenderContext* pRenderContext);
    ~TempSystemCB();

    void SetGenericData(const ezTransform& objectTransform, ezTime effectLifeTime, ezUInt8 uiNumVariationsX, ezUInt8 uiNumVariationsY, ezUInt8 uiNumFlipbookAnimsX, ezUInt8 uiNumFlipbookAnimsY, float fDistortionStrength = 0, float fNormalCurvature = 0, float fLightDirectionality = 0);

    /// Sets trail-specific rendering parameters.
    void SetTrailData(float fSnapshotFraction, ezInt32 iNumUsedTrailPoints);

    ezConstantBufferStorage<ezParticleSystemConstants>* m_pConstants;
    ezConstantBufferStorageHandle m_hConstantBuffer;
  };

  /// Allocates a GPU buffer from the pool for uploading particle data.
  void CreateParticleDataBuffer(ezGALBufferPool& inout_Buffer, ezUInt32 uiDataTypeSize, ezUInt32 uiNumParticlesPerBatch);

  /// Returns a particle data buffer to the pool.
  void DestroyParticleDataBuffer(ezGALBufferPool& inout_Buffer);

  /// Loads and binds the specified particle shader for rendering.
  void BindParticleShader(ezRenderContext* pRenderContext, const char* szShader) const;

protected:
  ezShaderResourceHandle m_hShader;
};
