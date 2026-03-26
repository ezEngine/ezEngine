#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <ParticlePlugin/Behavior/ParticleAttributes.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezParticleExpressionMode, 1)
  EZ_ENUM_CONSTANT(ezParticleExpressionMode::Set),
  EZ_ENUM_CONSTANT(ezParticleExpressionMode::LookupCurve),
  EZ_ENUM_CONSTANT(ezParticleExpressionMode::LookupSharedCurve),
  EZ_ENUM_CONSTANT(ezParticleExpressionMode::Discard),
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezParticleAttributeRead, 1)
  EZ_ENUM_CONSTANT(ezParticleAttributeRead::Age),
  EZ_ENUM_CONSTANT(ezParticleAttributeRead::PositionX),
  EZ_ENUM_CONSTANT(ezParticleAttributeRead::PositionY),
  EZ_ENUM_CONSTANT(ezParticleAttributeRead::PositionZ),
  EZ_ENUM_CONSTANT(ezParticleAttributeRead::VelocityX),
  EZ_ENUM_CONSTANT(ezParticleAttributeRead::VelocityY),
  EZ_ENUM_CONSTANT(ezParticleAttributeRead::VelocityZ),
  EZ_ENUM_CONSTANT(ezParticleAttributeRead::Speed),
  EZ_ENUM_CONSTANT(ezParticleAttributeRead::Size),
  EZ_ENUM_CONSTANT(ezParticleAttributeRead::RotationSpeed),
  EZ_ENUM_CONSTANT(ezParticleAttributeRead::ColorR),
  EZ_ENUM_CONSTANT(ezParticleAttributeRead::ColorG),
  EZ_ENUM_CONSTANT(ezParticleAttributeRead::ColorB),
  EZ_ENUM_CONSTANT(ezParticleAttributeRead::ColorA),
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezParticleAttributeWrite, 1)
  EZ_ENUM_CONSTANT(ezParticleAttributeWrite::PositionX),
  EZ_ENUM_CONSTANT(ezParticleAttributeWrite::PositionY),
  EZ_ENUM_CONSTANT(ezParticleAttributeWrite::PositionZ),
  EZ_ENUM_CONSTANT(ezParticleAttributeWrite::VelocityX),
  EZ_ENUM_CONSTANT(ezParticleAttributeWrite::VelocityY),
  EZ_ENUM_CONSTANT(ezParticleAttributeWrite::VelocityZ),
  EZ_ENUM_CONSTANT(ezParticleAttributeWrite::Speed),
  EZ_ENUM_CONSTANT(ezParticleAttributeWrite::Size),
  EZ_ENUM_CONSTANT(ezParticleAttributeWrite::RotationSpeed),
  EZ_ENUM_CONSTANT(ezParticleAttributeWrite::ColorR),
  EZ_ENUM_CONSTANT(ezParticleAttributeWrite::ColorG),
  EZ_ENUM_CONSTANT(ezParticleAttributeWrite::ColorB),
  EZ_ENUM_CONSTANT(ezParticleAttributeWrite::ColorA),
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

float ezReadParticleAttribute(ezParticleAttributeRead::Enum attr, ezUInt32 uiIndex,
  const ezProcessingStream* pPosition,
  const ezProcessingStream* pVelocity,
  const ezProcessingStream* pSize,
  const ezProcessingStream* pColor,
  const ezProcessingStream* pLifeTime,
  const ezProcessingStream* pRotationSpeed)
{
  switch (attr)
  {
    case ezParticleAttributeRead::Age:
    {
      if (pLifeTime)
      {
        // calculates normalized age from 0 to 1
        const ezVec2& lt = pLifeTime->GetData<ezFloat16Vec2>()[uiIndex];
        return 1.0f - lt.x * lt.y;
      }
      return 0.0f;
    }
    case ezParticleAttributeRead::PositionX:
      return pPosition ? static_cast<float>(pPosition->GetData<ezSimdVec4f>()[uiIndex].GetComponent<0>()) : 0.0f;
    case ezParticleAttributeRead::PositionY:
      return pPosition ? static_cast<float>(pPosition->GetData<ezSimdVec4f>()[uiIndex].GetComponent<1>()) : 0.0f;
    case ezParticleAttributeRead::PositionZ:
      return pPosition ? static_cast<float>(pPosition->GetData<ezSimdVec4f>()[uiIndex].GetComponent<2>()) : 0.0f;
    case ezParticleAttributeRead::VelocityX:
    {
      ezFloat16Vec4 vel = pVelocity ? pVelocity->GetData<ezFloat16Vec4>()[uiIndex] : ezFloat16Vec4();
      return vel.x * vel.w; // dirX * speed
    }
    case ezParticleAttributeRead::VelocityY:
    {
      ezFloat16Vec4 vel = pVelocity ? pVelocity->GetData<ezFloat16Vec4>()[uiIndex] : ezFloat16Vec4();
      return vel.y * vel.w; // dirY * speed
    }
    case ezParticleAttributeRead::VelocityZ:
    {
      ezFloat16Vec4 vel = pVelocity ? pVelocity->GetData<ezFloat16Vec4>()[uiIndex] : ezFloat16Vec4();
      return vel.z * vel.w; // dirZ * speed
    }
    case ezParticleAttributeRead::Speed:
      return pVelocity ? static_cast<float>(pVelocity->GetData<ezFloat16Vec4>()[uiIndex].w) : 0.0f;
    case ezParticleAttributeRead::Size:
      return pSize ? static_cast<float>(pSize->GetData<ezFloat16>()[uiIndex]) : 1.0f;
    case ezParticleAttributeRead::RotationSpeed:
      return pRotationSpeed ? static_cast<float>(pRotationSpeed->GetData<ezFloat16>()[uiIndex]) : 0.0f;
    case ezParticleAttributeRead::ColorR:
      return pColor ? static_cast<float>(pColor->GetData<ezFloat16Vec4>()[uiIndex].x) : 1.0f;
    case ezParticleAttributeRead::ColorG:
      return pColor ? static_cast<float>(pColor->GetData<ezFloat16Vec4>()[uiIndex].y) : 1.0f;
    case ezParticleAttributeRead::ColorB:
      return pColor ? static_cast<float>(pColor->GetData<ezFloat16Vec4>()[uiIndex].z) : 1.0f;
    case ezParticleAttributeRead::ColorA:
      return pColor ? static_cast<float>(pColor->GetData<ezFloat16Vec4>()[uiIndex].w) : 1.0f;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0.0f;
}

void ezWriteParticleAttribute(ezParticleAttributeWrite::Enum attr, ezUInt32 uiIndex, float fValue,
  ezProcessingStream* pPosition,
  ezProcessingStream* pVelocity,
  ezProcessingStream* pSize,
  ezProcessingStream* pColor,
  ezProcessingStream* pRotationSpeed)
{
  switch (attr)
  {
    case ezParticleAttributeWrite::PositionX:
    {
      if (pPosition)
      {
        ezSimdVec4f& pos = pPosition->GetWritableData<ezSimdVec4f>()[uiIndex];
        pos.SetX(ezSimdFloat(fValue));
      }
      break;
    }
    case ezParticleAttributeWrite::PositionY:
    {
      if (pPosition)
      {
        ezSimdVec4f& pos = pPosition->GetWritableData<ezSimdVec4f>()[uiIndex];
        pos.SetY(ezSimdFloat(fValue));
      }
      break;
    }
    case ezParticleAttributeWrite::PositionZ:
    {
      if (pPosition)
      {
        ezSimdVec4f& pos = pPosition->GetWritableData<ezSimdVec4f>()[uiIndex];
        pos.SetZ(ezSimdFloat(fValue));
      }
      break;
    }
    case ezParticleAttributeWrite::VelocityX:
    case ezParticleAttributeWrite::VelocityY:
    case ezParticleAttributeWrite::VelocityZ:
    {
      ezFloat16Vec4& v = pVelocity->GetWritableData<ezFloat16Vec4>()[uiIndex];

      ezVec3 newVel;
      newVel.x = static_cast<float>(v.x) * static_cast<float>(v.w);
      newVel.y = static_cast<float>(v.y) * static_cast<float>(v.w);
      newVel.z = static_cast<float>(v.z) * static_cast<float>(v.w);

      if (attr == ezParticleAttributeWrite::VelocityX)
        newVel.x = fValue;
      else if (attr == ezParticleAttributeWrite::VelocityY)
        newVel.y = fValue;
      else
        newVel.z = fValue;

      const float newSpeed = newVel.GetLength();
      const ezVec3 newDir = newSpeed > 0.0f ? newVel / newSpeed : ezVec3(0, 0, 1);
      v.x = newDir.x;
      v.y = newDir.y;
      v.z = newDir.z;
      v.w = newSpeed;
      break;
    }

    case ezParticleAttributeWrite::Speed:
    {
      if (pVelocity)
      {
        ezFloat16Vec4& v = pVelocity->GetWritableData<ezFloat16Vec4>()[uiIndex];
        v.w = fValue;
      }
      break;
    }
    case ezParticleAttributeWrite::Size:
    {
      if (pSize)
        pSize->GetWritableData<ezFloat16>()[uiIndex] = fValue;
      break;
    }
    case ezParticleAttributeWrite::RotationSpeed:
    {
      if (pRotationSpeed)
        pRotationSpeed->GetWritableData<ezFloat16>()[uiIndex] = fValue;
      break;
    }
    case ezParticleAttributeWrite::ColorR:
    {
      if (pColor)
      {
        ezFloat16Vec4& c = pColor->GetWritableData<ezFloat16Vec4>()[uiIndex];
        c.x = fValue;
      }
      break;
    }
    case ezParticleAttributeWrite::ColorG:
    {
      if (pColor)
      {
        ezFloat16Vec4& c = pColor->GetWritableData<ezFloat16Vec4>()[uiIndex];
        c.y = fValue;
      }
      break;
    }
    case ezParticleAttributeWrite::ColorB:
    {
      if (pColor)
      {
        ezFloat16Vec4& c = pColor->GetWritableData<ezFloat16Vec4>()[uiIndex];
        c.z = fValue;
      }
      break;
    }
    case ezParticleAttributeWrite::ColorA:
    {
      if (pColor)
      {
        ezFloat16Vec4& c = pColor->GetWritableData<ezFloat16Vec4>()[uiIndex];
        c.w = fValue;
      }
      break;
    }

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleConditionalCommon);
