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
class EZ_PARTICLEPLUGIN_DLL ezParticleRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezParticleRenderer);

public:
  ezParticleRenderer();
  ~ezParticleRenderer();

  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const override;

protected:
  struct TempSystemCB
  {
    TempSystemCB(ezRenderContext* pRenderContext);
    ~TempSystemCB();

    void SetGenericData(bool bApplyObjectTransform, const ezTransform& objectTransform, ezTime effectLifeTime, ezUInt8 uiNumVariationsX, ezUInt8 uiNumVariationsY, ezUInt8 uiNumFlipbookAnimsX, ezUInt8 uiNumFlipbookAnimsY, float fDistortionStrength = 0, float fNormalCurvature = 0, float fLightDirectionality = 0);
    void SetTrailData(float fSnapshotFraction, ezInt32 iNumUsedTrailPoints);

    ezConstantBufferStorage<ezParticleSystemConstants>* m_pConstants;
    ezConstantBufferStorageHandle m_hConstantBuffer;
  };

  void CreateParticleDataBuffer(ezGALBufferPool& inout_Buffer, ezUInt32 uiDataTypeSize, ezUInt32 uiNumParticlesPerBatch);
  void DestroyParticleDataBuffer(ezGALBufferPool& inout_Buffer);
  void BindParticleShader(ezRenderContext* pRenderContext, const char* szShader) const;

protected:
  ezShaderResourceHandle m_hShader;
};
