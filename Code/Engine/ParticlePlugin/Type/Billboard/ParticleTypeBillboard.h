#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/Billboard/BillboardRenderer.h>

typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;
struct ezBillboardParticleData;

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeBillboardFactory : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeBillboardFactory, ezParticleTypeFactory);

public:
  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezString m_sTexture;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeBillboard : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeBillboard, ezParticleType);

public:
  ezParticleTypeBillboard();

  virtual void CreateRequiredStreams() override;

  ezTexture2DResourceHandle m_hTexture;

  virtual void ExtractTypeRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const override;

protected:
  virtual void Process(ezUInt64 uiNumElements) override {}

  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamSize;
  ezProcessingStream* m_pStreamColor;
  ezProcessingStream* m_pStreamRotationSpeed;

  mutable ezArrayPtr<ezBillboardParticleData> m_ParticleData;
};


