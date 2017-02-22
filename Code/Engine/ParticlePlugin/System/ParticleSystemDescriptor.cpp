#include <PCH.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Age.h>
#include <ParticlePlugin/Type/Point/ParticleTypePoint.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleSystemDescriptor, 1, ezRTTIDefaultAllocator<ezParticleSystemDescriptor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("Visible", m_bVisible)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("MaxParticles", m_uiMaxParticles)->AddAttributes(new ezDefaultValueAttribute(64), new ezClampValueAttribute(1, 65535)),
    EZ_MEMBER_PROPERTY("LifeTime", m_LifeTime),
    EZ_MEMBER_PROPERTY("OnDeathEvent", m_sOnDeathEvent),
    EZ_ARRAY_MEMBER_PROPERTY("Emitters", m_EmitterFactories)->AddFlags(ezPropertyFlags::PointerOwner)->AddAttributes(new ezMaxArraySizeAttribute(1)),
    EZ_SET_ACCESSOR_PROPERTY("Initializers", GetInitializerFactories, AddInitializerFactory, RemoveInitializerFactory)->AddFlags(ezPropertyFlags::PointerOwner)->AddAttributes(new ezPreventDuplicatesAttribute()),
    EZ_SET_ACCESSOR_PROPERTY("Behaviors", GetBehaviorFactories, AddBehaviorFactory, RemoveBehaviorFactory)->AddFlags(ezPropertyFlags::PointerOwner)->AddAttributes(new ezPreventDuplicatesAttribute()),
    EZ_SET_ACCESSOR_PROPERTY("Types", GetTypeFactories, AddTypeFactory, RemoveTypeFactory)->AddFlags(ezPropertyFlags::PointerOwner)->AddAttributes(new ezPreventDuplicatesAttribute()),
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
  ClearTypes();
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

void ezParticleSystemDescriptor::ClearTypes()
{
  for (auto pFactory : m_TypeFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_TypeFactories.Clear();
}


void ezParticleSystemDescriptor::SetupDefaultProcessors()
{
  // Age Behavior
  {
    ezParticleBehaviorFactory_Age* pFactory = (ezParticleBehaviorFactory_Age*)ezParticleBehaviorFactory_Age::GetStaticRTTI()->GetAllocator()->Allocate();
    pFactory->m_LifeTime = m_LifeTime;
    pFactory->m_sOnDeathEvent = m_sOnDeathEvent;
    m_BehaviorFactories.PushBack(pFactory);
  }

  if (m_TypeFactories.IsEmpty())
  {
    ezParticleTypePointFactory* pFactory = (ezParticleTypePointFactory*)ezParticleTypePointFactory::GetStaticRTTI()->GetAllocator()->Allocate();
    m_TypeFactories.PushBack(pFactory);
  }
}

enum class ParticleSystemVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  Version_3,
  Version_4, // added Types
  Version_5, // added default processors
  Version_6, // changed lifetime variance

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void ezParticleSystemDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)ParticleSystemVersion::Version_Current;

  stream << uiVersion;

  const ezUInt32 uiNumEmitters = m_EmitterFactories.GetCount();
  const ezUInt32 uiNumInitializers = m_InitializerFactories.GetCount();
  const ezUInt32 uiNumBehaviors = m_BehaviorFactories.GetCount();
  const ezUInt32 uiNumTypes = m_TypeFactories.GetCount();

  stream << m_bVisible;
  stream << m_uiMaxParticles;
  stream << m_LifeTime.m_Value;
  stream << m_LifeTime.m_fVariance;
  stream << m_sOnDeathEvent;
  stream << uiNumEmitters;
  stream << uiNumInitializers;
  stream << uiNumBehaviors;
  stream << uiNumTypes;

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

  for (auto pType : m_TypeFactories)
  {
    stream << pType->GetDynamicRTTI()->GetTypeName();

    pType->Save(stream);
  }
}


void ezParticleSystemDescriptor::Load(ezStreamReader& stream)
{
  ClearEmitters();
  ClearInitializers();
  ClearBehaviors();
  ClearTypes();

  ezUInt8 uiVersion = 0;
  stream >> uiVersion;
  EZ_ASSERT_DEV(uiVersion <= (int)ParticleSystemVersion::Version_Current, "Unknown particle template version {0}", uiVersion);

  ezUInt32 uiNumEmitters = 0;
  ezUInt32 uiNumInitializers = 0;
  ezUInt32 uiNumBehaviors = 0;
  ezUInt32 uiNumTypes = 0;

  if (uiVersion >= 3)
  {
    stream >> m_bVisible;
  }

  if (uiVersion >= 2)
  {
    stream >> m_uiMaxParticles;
  }

  if (uiVersion >= 5)
  {
    stream >> m_LifeTime.m_Value;
    stream >> m_LifeTime.m_fVariance;
    stream >> m_sOnDeathEvent;
  }

  stream >> uiNumEmitters;

  if (uiVersion >= 2)
  {
    stream >> uiNumInitializers;
  }

  stream >> uiNumBehaviors;

  if (uiVersion >= 4)
  {
    stream >> uiNumTypes;
  }

  m_EmitterFactories.SetCountUninitialized(uiNumEmitters);
  m_InitializerFactories.SetCountUninitialized(uiNumInitializers);
  m_BehaviorFactories.SetCountUninitialized(uiNumBehaviors);
  m_TypeFactories.SetCountUninitialized(uiNumTypes);

  ezStringBuilder sType;

  for (auto& pEmitter : m_EmitterFactories)
  {
    stream >> sType;

    const ezRTTI* pRtti = ezRTTI::FindTypeByName(sType);
    EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown emitter factory type '{0}'", sType.GetData());

    pEmitter = static_cast<ezParticleEmitterFactory*>(pRtti->GetAllocator()->Allocate());

    pEmitter->Load(stream);
  }

  if (uiVersion >= 2)
  {
    for (auto& pInitializer : m_InitializerFactories)
    {
      stream >> sType;

      const ezRTTI* pRtti = ezRTTI::FindTypeByName(sType);
      EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown initializer factory type '{0}'", sType.GetData());

      pInitializer = static_cast<ezParticleInitializerFactory*>(pRtti->GetAllocator()->Allocate());

      pInitializer->Load(stream);
    }
  }

  for (auto& pBehavior : m_BehaviorFactories)
  {
    stream >> sType;

    const ezRTTI* pRtti = ezRTTI::FindTypeByName(sType);
    EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown behavior factory type '{0}'", sType.GetData());

    pBehavior = static_cast<ezParticleBehaviorFactory*>(pRtti->GetAllocator()->Allocate());

    pBehavior->Load(stream);
  }

  if (uiVersion >= 4)
  {
    for (auto& pType : m_TypeFactories)
    {
      stream >> sType;

      const ezRTTI* pRtti = ezRTTI::FindTypeByName(sType);
      EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown type factory type '{0}'", sType.GetData());

      pType = static_cast<ezParticleTypeFactory*>(pRtti->GetAllocator()->Allocate());

      pType->Load(stream);
    }
  }

  SetupDefaultProcessors();
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_System_ParticleSystemDescriptor);

