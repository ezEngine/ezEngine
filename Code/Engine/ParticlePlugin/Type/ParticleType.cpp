#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Type/ParticleType.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeFactory, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleType, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleType* ezParticleTypeFactory::CreateType(ezParticleSystemInstance* pOwner) const
{
  const ezRTTI* pRtti = GetTypeType();

  ezParticleType* pType = (ezParticleType*)pRtti->GetAllocator()->Allocate();
  pType->Reset(pOwner);

  CopyTypeProperties(pType);
  pType->AfterPropertiesConfigured(true);
  pType->CreateRequiredStreams();

  return pType;
}


ezParticleType::ezParticleType()
{
  m_uiLastExtractedFrame = 0;
}
