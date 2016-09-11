#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/Basics.h>

typedef ezTypedResourceHandle<class ezTextureResource> ezTextureResourceHandle;

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
  virtual void CreateRequiredStreams() override;

  virtual void Render(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass) const override;

  ezTextureResourceHandle m_hTexture;

protected:
  virtual void Process(ezUInt64 uiNumElements) override {}

  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamSize;
  ezProcessingStream* m_pStreamColor;

  //void CreateVertexBuffer(ezUInt32 uiVertexSize) const;
  void CreateDataBuffer(ezUInt32 uiStructSize) const;
  mutable ezGALBufferHandle m_hDataBuffer;
};


