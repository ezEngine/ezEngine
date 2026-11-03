#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Point/PointRenderer.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// Renders particles as single-pixel points.
class EZ_PARTICLEPLUGIN_DLL ezParticleTypePoint final : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypePoint, ezParticleType);

public:
  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void ExtractTypeRenderData(const ezParticleModuleContext& ctx, ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const override;

  /// Point particles have no radius for culling purposes.
  virtual float GetMaxParticleRadius(const ezParticleModuleContext& ctx, float fParticleSize) const override { return 0.0f; }

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamPosition;
    ezProcessingStream* m_pStreamColor;

    ezUInt64 m_uiLastExtractedFrame;
    ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
    ezArrayPtr<ezBillboardQuadParticleShaderData> m_BillboardParticleData;
  };
};
