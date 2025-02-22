#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Type/ParticleType.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeFactory, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleType, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleType* ezParticleTypeFactory::CreateType(ezParticleSystemInstance* pOwner) const
{
  const ezRTTI* pRtti = GetTypeType();

  ezParticleType* pType = pRtti->GetAllocator()->Allocate<ezParticleType>();
  pType->Reset(pOwner);

  CopyTypeProperties(pType, true);
  pType->CreateRequiredStreams();

  return pType;
}

ezParticleType::ezParticleType()
{
  m_uiLastExtractedFrame = 0;

  // run these as the last, after all the initializers and behaviors
  m_fPriority = +1000.0f;
}

ezUInt32 ezParticleType::ComputeSortingKey(ezParticleTypeRenderMode::Enum mode, ezUInt64 uiTextureHash)
{
  ezUInt32 key = 0;

  switch (mode)
  {
    case ezParticleTypeRenderMode::Additive:
      key = ezParticleTypeSortingKey::Additive;
      break;

    case ezParticleTypeRenderMode::Blended:
      key = ezParticleTypeSortingKey::Blended;
      break;

    case ezParticleTypeRenderMode::BlendedForeground:
      key = ezParticleTypeSortingKey::BlendedForeground;
      break;

    case ezParticleTypeRenderMode::BlendedBackground:
      key = ezParticleTypeSortingKey::BlendedBackground;
      break;

    case ezParticleTypeRenderMode::Opaque:
      key = ezParticleTypeSortingKey::Opaque;
      break;

    case ezParticleTypeRenderMode::BlendAdd:
      key = ezParticleTypeSortingKey::BlendAdd;
      break;

    case ezParticleTypeRenderMode::Distortion:
      key = ezParticleTypeSortingKey::Distortion;
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  key <<= 32 - 3; // require 3 bits for the values above
  key |= ezHashingUtils::StringHashTo32(uiTextureHash) & 0x1FFFFFFFu;

  return key;
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_ParticleType);
