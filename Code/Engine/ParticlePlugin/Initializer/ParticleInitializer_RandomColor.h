#pragma once

#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <Core/ResourceManager/ResourceHandle.h>

typedef ezTypedResourceHandle<class ezColorGradientResource> ezColorGradientResourceHandle;

class ezParticleInitializerFactory_RandomColor : public ezParticleInitializerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory_RandomColor, ezParticleInitializerFactory);

public:
  ezParticleInitializerFactory_RandomColor();

  virtual const ezRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

public:

  void SetColorGradient(const ezColorGradientResourceHandle& hResource) { m_hGradient = hResource; }
  EZ_FORCE_INLINE const ezColorGradientResourceHandle& GetColorGradient() const { return m_hGradient; }

  void SetColorGradientFile(const char* szFile);
  const char* GetColorGradientFile() const;

  ezColor m_Color1;
  ezColor m_Color2;

private:
  ezColorGradientResourceHandle m_hGradient;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_RandomColor : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_RandomColor, ezParticleInitializer);

public:

  ezColor m_Color1;
  ezColor m_Color2;

  ezColorGradientResourceHandle m_hGradient;

protected:
  virtual void SpawnElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

};
