#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

/// Initializer that spawns particles in a cylinder volume
///
/// Can spawn throughout the volume or only on the surface.
/// Optionally sets initial velocity pointing outward from the cylinder axis.
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializerFactory_CylinderPosition final : public ezParticleInitializerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory_CylinderPosition, ezParticleInitializerFactory);

public:
  ezParticleInitializerFactory_CylinderPosition();

  virtual const ezRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer, bool bFirstTime) const override;
  virtual float GetSpawnCountMultiplier(const ezParticleEffectInstance* pEffect) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

public:
  ezVec3 m_vPositionOffset;         ///< Center of the cylinder
  float m_fRadius;                  ///< Cylinder radius
  float m_fHeight;                  ///< Cylinder height
  bool m_bSpawnOnSurface;           ///< If true, spawn only on cylinder surface
  bool m_bSetVelocity;              ///< If true, set velocity pointing outward from axis
  ezVarianceTypeFloat m_Speed;      ///< Speed value when setting velocity
  ezString m_sScaleRadiusParameter; ///< Optional parameter name to scale radius
  ezString m_sScaleHeightParameter; ///< Optional parameter name to scale height
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_CylinderPosition final : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_CylinderPosition, ezParticleInitializer);

public:
  ezVec3 m_vPositionOffset;
  float m_fRadius;
  float m_fHeight;
  bool m_bSpawnOnSurface;
  bool m_bSetVelocity;
  ezVarianceTypeFloat m_Speed;

protected:
  virtual void CreateRequiredStreams() override;
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamVelocity;
};
