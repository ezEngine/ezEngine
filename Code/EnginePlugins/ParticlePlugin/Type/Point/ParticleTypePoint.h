#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Point/PointRenderer.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleTypePointFactory final : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypePointFactory, ezParticleTypeFactory);

public:
  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream) override;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypePoint final : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypePoint, ezParticleType);

public:
  ezParticleTypePoint() {}

  virtual void CreateRequiredStreams() override;

  virtual void ExtractTypeRenderData(ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const override;

  virtual float GetMaxParticleRadius(float fParticleSize) const override { return 0.0f; }

protected:
  virtual void Process(ezUInt64 uiNumElements) override {}

  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamColor;

  mutable ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
  mutable ezArrayPtr<ezBillboardQuadParticleShaderData> m_BillboardParticleData;
};
