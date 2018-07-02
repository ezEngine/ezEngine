#include <PCH.h>

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

  CopyTypeProperties(pType);
  pType->AfterPropertiesConfigured(true);
  pType->CreateRequiredStreams();

  return pType;
}


ezParticleType::ezParticleType()
{
  m_uiLastExtractedFrame = 0;

  // run these as the last, after all the initializers and behaviors
  m_fPriority = +1000.0f;
}


ezUInt32 ezParticleType::ComputeSortingKey(ezParticleTypeRenderMode::Enum mode)
{
  switch (mode)
  {
    case ezParticleTypeRenderMode::Additive:
      return ezParticleTypeSortingKey::Additive;

    case ezParticleTypeRenderMode::Blended:
      return ezParticleTypeSortingKey::Blended;

    case ezParticleTypeRenderMode::Opaque:
      return ezParticleTypeSortingKey::Opaque;
  }

  return 0;
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_ParticleType);
