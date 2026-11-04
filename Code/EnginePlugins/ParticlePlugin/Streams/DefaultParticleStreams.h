#pragma once

#include <ParticlePlugin/Streams/ParticleStream.h>

/// Initializes new particles at the particle system's transform position.
/// Stores positions as ezVec4 (Float4 stream type).
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Position final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Position, ezParticleStream);

public:
  ezParticleStream_Position();

  virtual void InitializeElements(ezParticleSystemInstance* pSystem, ezProcessingStream* pStream, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;
};

/// Initializes new particles with size 1.0.
/// Uses half-precision floats to reduce memory usage.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Size final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Size, ezParticleStream);

public:
  ezParticleStream_Size();

  virtual void InitializeElements(ezParticleSystemInstance* pSystem, ezProcessingStream* pStream, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;
};

/// Initializes new particles with white color (1, 1, 1, 1).
/// Uses half-precision floats (ezColorLinear16f) to reduce memory usage.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Color final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Color, ezParticleStream);

public:
  ezParticleStream_Color();

  virtual void InitializeElements(ezParticleSystemInstance* pSystem, ezProcessingStream* pStream, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;
};

/// Initializes new particles with the particle system's start velocity.
/// Stores velocity as direction (xyz) and speed (w) in an ezVec4 using half-precision floats.
/// If the start velocity is zero, defaults to direction (0, 0, 1) with speed 0.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Velocity final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Velocity, ezParticleStream);

public:
  ezParticleStream_Velocity();

  virtual void InitializeElements(ezParticleSystemInstance* pSystem, ezProcessingStream* pStream, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;
};

/// Used for trail rendering and motion blur effects to track the previous frame's particle positions.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_LastPosition final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_LastPosition, ezParticleStream);

public:
  ezParticleStream_LastPosition();
};

/// Stores the angular velocity for rotating billboard particles.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_RotationSpeed final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_RotationSpeed, ezParticleStream);

public:
  ezParticleStream_RotationSpeed();
};

/// Stores the initial rotation angle offset for particles.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_RotationOffset final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_RotationOffset, ezParticleStream);

public:
  ezParticleStream_RotationOffset();
};

/// Used to track which effect instance spawned a particle, useful for event reactions and debugging.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_EffectID final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_EffectID, ezParticleStream);

public:
  ezParticleStream_EffectID();
};

/// Used to enable or disable individual particles without removing them from the system.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_OnOff final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_OnOff, ezParticleStream);

public:
  ezParticleStream_OnOff();
};

/// Initializes new particles with axis (1, 0, 0).
/// Used for oriented particle rendering where particles need a direction vector.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Axis final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Axis, ezParticleStream);

public:
  ezParticleStream_Axis();

  virtual void InitializeElements(ezParticleSystemInstance* pSystem, ezProcessingStream* pStream, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;
};

/// Stores trail-specific data for trail renderers to connect particles into ribbons.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_TrailData final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_TrailData, ezParticleStream);

public:
  ezParticleStream_TrailData();
};

/// Initializes new particles with random unsigned integers.
/// Used for texture atlas variations, flipbook animations, or other per-particle randomization.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Variation final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Variation, ezParticleStream);

public:
  ezParticleStream_Variation();

  virtual void InitializeElements(ezParticleSystemInstance* pSystem, ezProcessingStream* pStream, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;
};
