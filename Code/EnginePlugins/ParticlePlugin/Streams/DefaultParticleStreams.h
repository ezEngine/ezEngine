#pragma once

#include <ParticlePlugin/Streams/ParticleStream.h>

//////////////////////////////////////////////////////////////////////////
// ZERO-INIT STREAM
//////////////////////////////////////////////////////////////////////////

/// Stream that initializes particle data to zero.
///
/// Uses the base class implementation which zero-fills all elements.
/// This stream type is used for data that should start at zero without custom initialization.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_ZeroInit final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_ZeroInit, ezParticleStream);

protected:
  // base class implementation already zero fills the stream data
  // virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// POSITION STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating position streams (Float4 data type).
class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_Position final : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_Position, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_Position();
};

/// Stream storing particle positions.
///
/// Initializes new particles at the particle system's transform position.
/// Stores positions as ezVec4 (Float4 stream type).
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Position final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Position, ezParticleStream);

protected:
  virtual void Initialize(ezParticleSystemInstance* pOwner) override;
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezParticleSystemInstance* m_pOwner;
};

//////////////////////////////////////////////////////////////////////////
// SIZE STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating size streams (Half data type).
class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_Size final : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_Size, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_Size();
};

/// Stream storing particle sizes.
///
/// Initializes new particles with size 1.0.
/// Uses half-precision floats to reduce memory usage.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Size final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Size, ezParticleStream);

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// COLOR STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating color streams (Half4 data type).
class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_Color final : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_Color, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_Color();
};

/// Stream storing particle colors.
///
/// Initializes new particles with white color (1, 1, 1, 1).
/// Uses half-precision floats (ezColorLinear16f) to reduce memory usage.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Color final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Color, ezParticleStream);

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// VELOCITY STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating velocity streams (Half4 data type).
class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_Velocity final : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_Velocity, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_Velocity();
};

/// Stream storing particle velocities.
///
/// Initializes new particles with the particle system's start velocity.
/// Stores velocity as direction (xyz) and speed (w) in an ezVec4 using half-precision floats.
/// If the start velocity is zero, defaults to direction (0, 0, 1) with speed 0.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Velocity final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Velocity, ezParticleStream);

protected:
  virtual void Initialize(ezParticleSystemInstance* pOwner) override;
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezParticleSystemInstance* m_pOwner;
};

//////////////////////////////////////////////////////////////////////////
// LIFETIME STREAM
//////////////////////////////////////////////////////////////////////////

// always default initialized by the behavior

//////////////////////////////////////////////////////////////////////////
// LAST POSITION STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating last position streams (Float3 data type).
///
/// Used for trail rendering and motion blur effects to track the previous frame's particle positions.
class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_LastPosition final : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_LastPosition, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_LastPosition();
};

//////////////////////////////////////////////////////////////////////////
// ROTATION SPEED STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating rotation speed streams (Half data type).
///
/// Stores the angular velocity for rotating billboard particles.
class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_RotationSpeed final : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_RotationSpeed, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_RotationSpeed();
};

//////////////////////////////////////////////////////////////////////////
// ROTATION OFFSET STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating rotation offset streams (Half data type).
///
/// Stores the initial rotation angle offset for particles.
class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_RotationOffset final : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_RotationOffset, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_RotationOffset();
};

//////////////////////////////////////////////////////////////////////////
// EFFECT ID STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating effect ID streams (Int data type).
///
/// Used to track which effect instance spawned a particle, useful for event reactions and debugging.
class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_EffectID final : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_EffectID, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_EffectID();
};

//////////////////////////////////////////////////////////////////////////
// ON OFF STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating on/off streams (Byte data type).
///
/// Used to enable or disable individual particles without removing them from the system.
class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_OnOff final : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_OnOff, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_OnOff();
};

//////////////////////////////////////////////////////////////////////////
// AXIS STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating axis streams (Float3 data type).
class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_Axis final : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_Axis, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_Axis();
};

/// Stream storing particle orientation axes.
///
/// Initializes new particles with axis (1, 0, 0).
/// Used for oriented particle rendering where particles need a direction vector.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Axis final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Axis, ezParticleStream);

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// TRAIL DATA STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating trail data streams (Short2 data type).
///
/// Stores trail-specific data for trail renderers to connect particles into ribbons.
class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_TrailData final : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_TrailData, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_TrailData();
};

//////////////////////////////////////////////////////////////////////////
// VARIATION STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating variation streams (Int data type).
class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_Variation final : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_Variation, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_Variation();
};

/// Stream storing particle variation values.
///
/// Initializes new particles with random unsigned integers.
/// Used for texture atlas variations, flipbook animations, or other per-particle randomization.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Variation final : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Variation, ezParticleStream);

protected:
  virtual void Initialize(ezParticleSystemInstance* pOwner) override;
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezParticleSystemInstance* m_pOwner;
};
