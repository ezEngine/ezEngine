#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <ParticlePlugin/Type/Point/PointRenderer.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleTypePointFactory final : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypePointFactory, ezParticleTypeFactory);

public:
  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypePoint final : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypePoint, ezParticleType);

public:
  ezParticleTypePoint() {}

  virtual void CreateRequiredStreams() override;

  virtual void ExtractTypeRenderData(const ezView& view, ezExtractedRenderData& extractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const override;

  virtual float GetMaxParticleRadius(float fParticleSize) const { return 0.0f; }

protected:
  virtual void Process(ezUInt64 uiNumElements) override {}

  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamColor;

  mutable ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
  mutable ezArrayPtr<ezBillboardQuadParticleShaderData> m_BillboardParticleData;
};


