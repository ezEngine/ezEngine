#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEffectDescriptor, 1, ezRTTIDefaultAllocator<ezParticleEffectDescriptor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_SET_ACCESSOR_PROPERTY("Particle Systems", GetParticleSystems, AddParticleSystem, RemoveParticleSystem)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleEffectDescriptor::ezParticleEffectDescriptor()
{
}

ezParticleEffectDescriptor::~ezParticleEffectDescriptor()
{
  ClearSystems();
}

void ezParticleEffectDescriptor::ClearSystems()
{
  for (auto pSystem : m_ParticleSystems)
  {
    pSystem->GetDynamicRTTI()->GetAllocator()->Deallocate(pSystem);
  }

  m_ParticleSystems.Clear();
}

void ezParticleEffectDescriptor::Save(ezStreamWriter& stream) const
{
  ezUInt8 uiVersion = 1;

  stream << uiVersion;

  const ezUInt32 uiNumSystems = m_ParticleSystems.GetCount();

  stream << uiNumSystems;

  for (auto pSystem : m_ParticleSystems)
  {
    stream << pSystem->GetDynamicRTTI()->GetTypeName();

    pSystem->Save(stream);
  }
}


void ezParticleEffectDescriptor::Load(ezStreamReader& stream)
{
  ClearSystems();

  ezUInt8 uiVersion = 0;
  stream >> uiVersion;
  EZ_ASSERT_DEV(uiVersion == 1, "Unknown particle effect template version %u", uiVersion);

  ezUInt32 uiNumSystems = 0;

  stream >> uiNumSystems;

  m_ParticleSystems.SetCountUninitialized(uiNumSystems);

  ezStringBuilder sType;

  for (auto& pSystem : m_ParticleSystems)
  {
    stream >> sType;

    const ezRTTI* pRtti = ezRTTI::FindTypeByName(sType);
    EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown particle effect type '%s'", sType.GetData());

    pSystem = static_cast<ezParticleSystemDescriptor*>(pRtti->GetAllocator()->Allocate());

    pSystem->Load(stream);
  }
}