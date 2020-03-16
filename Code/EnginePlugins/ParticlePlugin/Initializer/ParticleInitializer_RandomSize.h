#pragma once

#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>

typedef ezTypedResourceHandle<class ezCurve1DResource> ezCurve1DResourceHandle;

class EZ_PARTICLEPLUGIN_DLL ezParticleInitializerFactory_RandomSize final : public ezParticleInitializerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory_RandomSize, ezParticleInitializerFactory);

public:

  virtual const ezRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  void SetSizeCurveFile(const char* szFile);
  const char* GetSizeCurveFile() const;

  ezVarianceTypeFloat m_Size;
  ezCurve1DResourceHandle m_hCurve;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_RandomSize final : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_RandomSize, ezParticleInitializer);

public:

  ezVarianceTypeFloat m_Size;
  ezCurve1DResourceHandle m_hCurve;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamSize;
};
