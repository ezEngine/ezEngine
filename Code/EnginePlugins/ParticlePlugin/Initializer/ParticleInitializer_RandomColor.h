#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

using ezColorGradientResourceHandle = ezTypedResourceHandle<class ezColorGradientResource>;

class EZ_PARTICLEPLUGIN_DLL ezParticleInitializerFactory_RandomColor final : public ezParticleInitializerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory_RandomColor, ezParticleInitializerFactory);

public:
  virtual const ezRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream) override;

  ezColor m_Color1;
  ezColor m_Color2;
  ezColorGradientResourceHandle m_hGradient;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_RandomColor final : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_RandomColor, ezParticleInitializer);

public:
  ezColor m_Color1;
  ezColor m_Color2;

  ezColorGradientResourceHandle m_hGradient;


  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamColor;
};
