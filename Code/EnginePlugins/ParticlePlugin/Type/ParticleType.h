#pragma once

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

/// Base class for particle types that define how particles are rendered.
///
/// Each particle type handles a specific rendering method such as billboards,
/// trails, meshes, or lights. Types process particle data each frame and
/// generate render data for the renderer.
class EZ_PARTICLEPLUGIN_DLL ezParticleType : public ezParticleModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleType, ezParticleModule);

public:
  ezParticleType();

  /// Allows the type to register any finalizers it depends on.
  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const {}

  virtual void Save(ezStreamWriter& inout_stream) const = 0;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) = 0;

  /// Returns the maximum radius a particle can occupy for culling purposes.
  ///
  /// Used to compute bounding volumes for frustum culling. The default implementation
  /// returns half the particle size, assuming spherical particles.
  virtual float GetMaxParticleRadius(const ezParticleModuleContext& ctx, float fParticleSize) const { return fParticleSize * 0.5f; }

  /// Generates render data for all active particles.
  ///
  /// Called during render data extraction to create render objects for this particle type.
  /// Not called during the particle update, but it may be called multiple times per frame for shared effects.
  virtual void ExtractTypeRenderData(const ezParticleModuleContext& ctx, ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const = 0;

protected:
  static ezUInt32 ComputeSortingKey(ezParticleTypeRenderMode::Enum mode, ezUInt64 uiResource1Hash, ezUInt64 uiResource2Hash);
};
