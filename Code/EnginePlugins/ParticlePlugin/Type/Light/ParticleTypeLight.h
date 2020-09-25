#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezView;
class ezExtractedRenderData;

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeLightFactory final : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeLightFactory, ezParticleTypeFactory);

public:
  ezParticleTypeLightFactory();

  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  float m_fSizeFactor;
  float m_fIntensity;
  ezUInt32 m_uiPercentage;
  ezString m_sTintColorParameter;
  ezString m_sIntensityParameter;
  ezString m_sSizeScaleParameter;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeLight final : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeLight, ezParticleType);

public:
  virtual void CreateRequiredStreams() override;

  float m_fSizeFactor;
  float m_fIntensity;
  ezUInt32 m_uiPercentage;
  ezTempHashedString m_sTintColorParameter;
  ezTempHashedString m_sIntensityParameter;
  ezTempHashedString m_sSizeScaleParameter;

  virtual float GetMaxParticleRadius(float fParticleSize) const override { return 0.5f * fParticleSize * m_fSizeFactor; }

  virtual void ExtractTypeRenderData(ezMsgExtractRenderData& msg, const ezTransform& instanceTransform) const override;

protected:
  virtual void Process(ezUInt64 uiNumElements) override {}

  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamSize;
  ezProcessingStream* m_pStreamColor;
  ezProcessingStream* m_pStreamOnOff;
};
