#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleSystemDescriptor, 1, ezRTTIDefaultAllocator<ezParticleSystemDescriptor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Visible", m_bVisible)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Max Particles", m_uiMaxParticles)->AddAttributes(new ezDefaultValueAttribute(64)),
    EZ_SET_ACCESSOR_PROPERTY("Emitters", GetEmitterFactories, AddEmitterFactory, RemoveEmitterFactory)->AddFlags(ezPropertyFlags::PointerOwner),
    EZ_SET_ACCESSOR_PROPERTY("Initializers", GetInitializerFactories, AddInitializerFactory, RemoveInitializerFactory)->AddFlags(ezPropertyFlags::PointerOwner),
    EZ_SET_ACCESSOR_PROPERTY("Behaviors", GetBehaviorFactories, AddBehaviorFactory, RemoveBehaviorFactory)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleSystemDescriptor::ezParticleSystemDescriptor()
{
  m_bVisible = true;
  m_uiMaxParticles = 64;
}

ezParticleSystemDescriptor::~ezParticleSystemDescriptor()
{
  ClearEmitters();
  ClearInitializers();
  ClearBehaviors();
}

void ezParticleSystemDescriptor::ClearEmitters()
{
  for (auto pFactory : m_EmitterFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_EmitterFactories.Clear();
}

void ezParticleSystemDescriptor::ClearInitializers()
{
  for (auto pFactory : m_InitializerFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_InitializerFactories.Clear();
}

void ezParticleSystemDescriptor::ClearBehaviors()
{
  for (auto pFactory : m_BehaviorFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_BehaviorFactories.Clear();
}

enum ParticleSystemVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  Version_3,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void ezParticleSystemDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = ParticleSystemVersion::Version_Current;

  stream << uiVersion;

  const ezUInt32 uiNumEmitters = m_EmitterFactories.GetCount();
  const ezUInt32 uiNumInitializers = m_InitializerFactories.GetCount();
  const ezUInt32 uiNumBehaviors = m_BehaviorFactories.GetCount();

  stream << m_bVisible;
  stream << m_uiMaxParticles;
  stream << uiNumEmitters;
  stream << uiNumInitializers;
  stream << uiNumBehaviors;

  for (auto pEmitter : m_EmitterFactories)
  {
    stream << pEmitter->GetDynamicRTTI()->GetTypeName();

    pEmitter->Save(stream);
  }

  for (auto pInitializer : m_InitializerFactories)
  {
    stream << pInitializer->GetDynamicRTTI()->GetTypeName();

    pInitializer->Save(stream);
  }

  for (auto pBehavior : m_BehaviorFactories)
  {
    stream << pBehavior->GetDynamicRTTI()->GetTypeName();

    pBehavior->Save(stream);
  }
}


void ezParticleSystemDescriptor::Load(ezStreamReader& stream)
{
  ClearEmitters();
  ClearInitializers();
  ClearBehaviors();

  ezUInt8 uiVersion = 0;
  stream >> uiVersion;
  EZ_ASSERT_DEV(uiVersion <= ParticleSystemVersion::Version_Current, "Unknown particle template version %u", uiVersion);

  ezUInt32 uiNumEmitters = 0;
  ezUInt32 uiNumInitializers = 0;
  ezUInt32 uiNumBehaviors = 0;

  if (uiVersion >= 3)
  {
    stream >> m_bVisible;
  }

  if (uiVersion >= 2)
  {
    stream >> m_uiMaxParticles;
  }

  stream >> uiNumEmitters;

  if (uiVersion >= 2)
  {
    stream >> uiNumInitializers;
  }

  stream >> uiNumBehaviors;

  m_EmitterFactories.SetCountUninitialized(uiNumEmitters);
  m_InitializerFactories.SetCountUninitialized(uiNumInitializers);
  m_BehaviorFactories.SetCountUninitialized(uiNumBehaviors);

  ezStringBuilder sType;

  for (auto& pEmitter : m_EmitterFactories)
  {
    stream >> sType;

    const ezRTTI* pRtti = ezRTTI::FindTypeByName(sType);
    EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown emitter factory type '%s'", sType.GetData());

    pEmitter = static_cast<ezParticleEmitterFactory*>(pRtti->GetAllocator()->Allocate());

    pEmitter->Load(stream);
  }

  if (uiVersion >= 2)
  {
    for (auto& pInitializer : m_InitializerFactories)
    {
      stream >> sType;

      const ezRTTI* pRtti = ezRTTI::FindTypeByName(sType);
      EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown initializer factory type '%s'", sType.GetData());

      pInitializer = static_cast<ezParticleInitializerFactory*>(pRtti->GetAllocator()->Allocate());

      pInitializer->Load(stream);
    }
  }

  for (auto& pBehavior : m_BehaviorFactories)
  {
    stream >> sType;

    const ezRTTI* pRtti = ezRTTI::FindTypeByName(sType);
    EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown behavior factory type '%s'", sType.GetData());

    pBehavior = static_cast<ezParticleBehaviorFactory*>(pRtti->GetAllocator()->Allocate());

    pBehavior->Load(stream);
  }
}
