#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

using ezCurve1DResourceHandle = ezTypedResourceHandle<class ezCurve1DResource>;

class EZ_PARTICLEPLUGIN_DLL ezParticleInitializerFactory_RandomRotationSpeed final : public ezParticleInitializerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory_RandomRotationSpeed, ezParticleInitializerFactory);

public:
  virtual const ezRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  bool m_bRandomStartAngle = false;
  ezVarianceTypeAngle m_RotationSpeed;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_RandomRotationSpeed final : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_RandomRotationSpeed, ezParticleInitializer);

public:
  bool m_bRandomStartAngle = false;
  ezVarianceTypeAngle m_RotationSpeed;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  bool m_bPositiveSign = false;
  ezProcessingStream* m_pStreamRotationSpeed = nullptr;
  ezProcessingStream* m_pStreamRotationOffset = nullptr;
};
