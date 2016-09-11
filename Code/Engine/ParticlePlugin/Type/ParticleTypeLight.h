#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/Basics.h>

class ezView;
class ezExtractedRenderData;

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeLightFactory : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeLightFactory, ezParticleTypeFactory);

public:
  ezParticleTypeLightFactory();

  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  float m_fSizeFactor;
  float m_fIntensity;
  ezUInt32 m_uiPercentage;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeLight : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeLight, ezParticleType);

public:
  virtual void CreateRequiredStreams() override;

  virtual void Render(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass) const override {}

  float m_fSizeFactor;
  float m_fIntensity;
  ezUInt32 m_uiPercentage;

  virtual void ExtractRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData) const override;

protected:
  virtual void Process(ezUInt64 uiNumElements) override {}

  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamSize;
  ezProcessingStream* m_pStreamColor;
  ezProcessingStream* m_pStreamOnOff;
};


