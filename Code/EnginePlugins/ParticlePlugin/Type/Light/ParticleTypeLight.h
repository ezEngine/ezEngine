#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezView;
class ezExtractedRenderData;

/// Renders particles as dynamic point lights.
///
/// Each particle creates a point light with range based on particle size.
/// Not all particles need to emit light - the percentage can be controlled
/// to reduce performance cost. Light properties can be modulated by effect parameters.
class EZ_PARTICLEPLUGIN_DLL ezParticleTypeLight final : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeLight, ezParticleType);

public:
  ezParticleTypeLight();


  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void ExtractTypeRenderData(const ezParticleModuleContext& ctx, ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const override;

  virtual float GetMaxParticleRadius(const ezParticleModuleContext& ctx, float fParticleSize) const override { return 0.5f * fParticleSize * m_fSizeFactor; }

  float m_fSizeFactor;
  float m_fIntensity;
  ezUInt32 m_uiPercentage;
  ezHashedString m_sTintColorParameter;
  ezHashedString m_sIntensityParameter;
  ezHashedString m_sSizeScaleParameter;
  bool m_bScaleIntensityByAlpha = false; ///< If true, the particle alpha (e.g. from a color gradient) scales the light intensity. Otherwise lights ignore alpha.

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamPosition;
    ezProcessingStream* m_pStreamSize;
    ezProcessingStream* m_pStreamColor;
    ezProcessingStream* m_pStreamOnOff;
  };
};
