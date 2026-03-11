#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

struct ezMsgExtractRenderData;

/// Sorting key values used to order particles during rendering.
enum ezParticleTypeSortingKey
{
  Opaque,
  BlendedBackground,
  Additive,
  Blended,
  BlendedForeground,
};

/// Factory for creating particle type instances.
///
/// Each particle type factory stores the configuration for a particle type
/// and can create instances of that type for particle system instances.
class EZ_PARTICLEPLUGIN_DLL ezParticleTypeFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeFactory, ezReflectedClass);

public:
  virtual const ezRTTI* GetTypeType() const = 0;
  virtual void CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const = 0;

  ezParticleType* CreateType(ezParticleSystemInstance* pOwner) const;

  /// Allows the type to register any finalizers it depends on.
  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const {}

  virtual void Save(ezStreamWriter& inout_stream) const = 0;
  virtual void Load(ezStreamReader& inout_stream) = 0;
};

/// Base class for particle types that define how particles are rendered.
///
/// Each particle type handles a specific rendering method such as billboards,
/// trails, meshes, or lights. Types process particle data each frame and
/// generate render data for the renderer.
class EZ_PARTICLEPLUGIN_DLL ezParticleType : public ezParticleModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleType, ezParticleModule);

  friend class ezParticleSystemInstance;

public:
  /// Returns the maximum radius a particle can occupy for culling purposes.
  ///
  /// Used to compute bounding volumes for frustum culling. The default implementation
  /// returns half the particle size, assuming spherical particles.
  virtual float GetMaxParticleRadius(float fParticleSize) const { return fParticleSize * 0.5f; }

  /// Generates render data for all active particles.
  ///
  /// Called during render data extraction to create render objects for this particle type.
  virtual void ExtractTypeRenderData(ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const = 0;

protected:
  ezParticleType();

  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override {}
  virtual void StepParticleSystem(const ezTime& tDiff, ezUInt32 uiNumNewParticles) { m_TimeDiff = tDiff; }

  static ezUInt32 ComputeSortingKey(ezParticleTypeRenderMode::Enum mode, ezUInt64 uiResource1Hash, ezUInt64 uiResource2Hash);

  ezTime m_TimeDiff;
  mutable ezUInt64 m_uiLastExtractedFrame;
};
