#pragma once

#include <Foundation/Math/Declarations.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class ezProcessingStream;

/// Particle attribute that can be read by behaviors.
struct EZ_PARTICLEPLUGIN_DLL ezParticleAttributeRead
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Age,
    PositionX,
    PositionY,
    PositionZ,
    VelocityX,
    VelocityY,
    VelocityZ,
    Speed,
    Size,
    RotationSpeed,
    ColorR,
    ColorG,
    ColorB,
    ColorA,

    Default = Age
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezParticleAttributeRead);

/// Particle attribute that can be written by behaviors.
struct EZ_PARTICLEPLUGIN_DLL ezParticleAttributeWrite
{
  using StorageType = ezUInt8;

  enum Enum
  {
    // note that modifying position or velocity really only makes sense when simulating in local space
    // as otherwise these values are in global coordinates
    PositionX,
    PositionY,
    PositionZ,
    VelocityX,
    VelocityY,
    VelocityZ,

    Speed,
    Size,
    RotationSpeed,
    ColorR,
    ColorG,
    ColorB,
    ColorA,

    Default = Size
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezParticleAttributeWrite);

/// Controls the output mode of the Expression behavior.
struct EZ_PARTICLEPLUGIN_DLL ezParticleExpressionMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Set,               ///< Writes the expression result to the chosen output attribute.
    LookupCurve,       ///< Uses the expression result as the X position to sample an embedded curve; writes the curve output to the chosen attribute.
    LookupSharedCurve, ///< Uses the expression result as the X position to sample a shared curve resource; writes the curve output to the chosen attribute.
    Discard,           ///< Kills the particle when the expression result satisfies the comparison against the threshold.

    Default = Set
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezParticleExpressionMode);

/// Reads a particle attribute value at the given index.
/// Returns 0 if the required stream is null.
EZ_PARTICLEPLUGIN_DLL float ezReadParticleAttribute(
  ezParticleAttributeRead::Enum attr, ezUInt32 uiIndex,
  const ezProcessingStream* pPosition,
  const ezProcessingStream* pVelocity,
  const ezProcessingStream* pSize,
  const ezProcessingStream* pColor,
  const ezProcessingStream* pLifeTime,
  const ezProcessingStream* pRotationSpeed);

/// Writes a float value to a particle attribute at the given index.
EZ_PARTICLEPLUGIN_DLL void ezWriteParticleAttribute(
  ezParticleAttributeWrite::Enum attr, ezUInt32 uiIndex, float fValue,
  ezProcessingStream* pPosition,
  ezProcessingStream* pVelocity,
  ezProcessingStream* pSize,
  ezProcessingStream* pColor,
  ezProcessingStream* pRotationSpeed);
