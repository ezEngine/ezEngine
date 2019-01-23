#include <PCH.h>

#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Age.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Volume.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Point/ParticleTypePoint.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleSystemDescriptor, 1, ezRTTIDefaultAllocator<ezParticleSystemDescriptor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("Visible", m_bVisible)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("LifeTime", m_LifeTime)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(2)), new ezClampValueAttribute(ezTime::Seconds(0.0), ezVariant())),
    EZ_MEMBER_PROPERTY("LifeScaleParam", m_sLifeScaleParameter),
    EZ_MEMBER_PROPERTY("OnDeathEvent", m_sOnDeathEvent),
    EZ_ARRAY_MEMBER_PROPERTY("Emitters", m_EmitterFactories)->AddFlags(ezPropertyFlags::PointerOwner)->AddAttributes(new ezMaxArraySizeAttribute(1)),
    EZ_SET_ACCESSOR_PROPERTY("Initializers", GetInitializerFactories, AddInitializerFactory, RemoveInitializerFactory)->AddFlags(ezPropertyFlags::PointerOwner)->AddAttributes(new ezPreventDuplicatesAttribute()),
    EZ_SET_ACCESSOR_PROPERTY("Behaviors", GetBehaviorFactories, AddBehaviorFactory, RemoveBehaviorFactory)->AddFlags(ezPropertyFlags::PointerOwner)->AddAttributes(new ezPreventDuplicatesAttribute()),
    EZ_SET_ACCESSOR_PROPERTY("Types", GetTypeFactories, AddTypeFactory, RemoveTypeFactory)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleSystemDescriptor::ezParticleSystemDescriptor()
{
  m_bVisible = true;
}

ezParticleSystemDescriptor::~ezParticleSystemDescriptor()
{
  ClearEmitters();
  ClearInitializers();
  ClearBehaviors();
  ClearFinalizers();
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

void ezParticleSystemDescriptor::ClearFinalizers()
{
  for (auto pFactory : m_FinalizerFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_FinalizerFactories.Clear();
}

void ezParticleSystemDescriptor::SetupDefaultProcessors()
{
  // Age Behavior
  {
    ezParticleFinalizerFactory_Age* pFactory =
        ezParticleFinalizerFactory_Age::GetStaticRTTI()->GetAllocator()->Allocate<ezParticleFinalizerFactory_Age>();
    pFactory->m_LifeTime = m_LifeTime;
    pFactory->m_sOnDeathEvent = m_sOnDeathEvent;
    pFactory->m_sLifeScaleParameter = m_sLifeScaleParameter;
    m_FinalizerFactories.PushBack(pFactory);
  }

  // Bounding Volume Update Behavior
  {
    ezParticleFinalizerFactory_Volume* pFactory =
        ezParticleFinalizerFactory_Volume::GetStaticRTTI()->GetAllocator()->Allocate<ezParticleFinalizerFactory_Volume>();
    m_FinalizerFactories.PushBack(pFactory);
  }

  if (m_TypeFactories.IsEmpty())
  {
    ezParticleTypePointFactory* pFactory =
        ezParticleTypePointFactory::GetStaticRTTI()->GetAllocator()->Allocate<ezParticleTypePointFactory>();
    m_TypeFactories.PushBack(pFactory);
  }

  ezSet<const ezRTTI*> finalizers;
  for (const auto* pFactory : m_InitializerFactories)
  {
    pFactory->QueryFinalizerDependencies(finalizers);
  }

  for (const auto* pFactory : m_BehaviorFactories)
  {
    pFactory->QueryFinalizerDependencies(finalizers);
  }

  for (const auto* pFactory : m_TypeFactories)
  {
    pFactory->QueryFinalizerDependencies(finalizers);
  }

  for (const ezRTTI* pRtti : finalizers)
  {
    EZ_ASSERT_DEBUG(pRtti->IsDerivedFrom<ezParticleFinalizerFactory>(), "Invalid finalizer factory added as a dependency: '{0}'",
                    pRtti->GetTypeName());
    EZ_ASSERT_DEBUG(pRtti->GetAllocator()->CanAllocate(), "Finalizer factory cannot be allocated: '{0}'", pRtti->GetTypeName());

    m_FinalizerFactories.PushBack(pRtti->GetAllocator()->Allocate<ezParticleFinalizerFactory>());
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
  Version_7, // added life scale param

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


ezTime ezParticleSystemDescriptor::GetAvgLifetime() const
{
  ezTime time = m_LifeTime.m_Value + m_LifeTime.m_Value * (m_LifeTime.m_fVariance * 2.0f / 3.0f);

  // we actively prevent values outside the [0;2] range for the life-time scale parameter, when it is applied
  // so this is the accurate worst case value
  // effects should be authored with the maximum lifetime, and at runtime the lifetime should only be scaled down
  if (!m_sLifeScaleParameter.IsEmpty())
  {
    time = time * 2.0f;
  }

  return time;
}

void ezParticleSystemDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)ParticleSystemVersion::Version_Current;

  stream << uiVersion;

  const ezUInt32 uiNumEmitters = m_EmitterFactories.GetCount();
  const ezUInt32 uiNumInitializers = m_InitializerFactories.GetCount();
  const ezUInt32 uiNumBehaviors = m_BehaviorFactories.GetCount();
  const ezUInt32 uiNumTypes = m_TypeFactories.GetCount();

  ezUInt32 uiMaxParticles = 0;
  stream << m_bVisible;
  stream << uiMaxParticles;
  stream << m_LifeTime.m_Value;
  stream << m_LifeTime.m_fVariance;
  stream << m_sOnDeathEvent;
  stream << m_sLifeScaleParameter;
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
  ClearFinalizers();
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
    // now unused
    ezUInt32 uiMaxParticles = 0;
    stream >> uiMaxParticles;
  }

  if (uiVersion >= 5)
  {
    stream >> m_LifeTime.m_Value;
    stream >> m_LifeTime.m_fVariance;
    stream >> m_sOnDeathEvent;
  }

  if (uiVersion >= 7)
  {
    stream >> m_sLifeScaleParameter;
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
    EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown emitter factory type '{0}'", sType);

    pEmitter = pRtti->GetAllocator()->Allocate<ezParticleEmitterFactory>();

    pEmitter->Load(stream);
  }

  if (uiVersion >= 2)
  {
    for (auto& pInitializer : m_InitializerFactories)
    {
      stream >> sType;

      const ezRTTI* pRtti = ezRTTI::FindTypeByName(sType);
      EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown initializer factory type '{0}'", sType);

      pInitializer = pRtti->GetAllocator()->Allocate<ezParticleInitializerFactory>();

      pInitializer->Load(stream);
    }
  }

  for (auto& pBehavior : m_BehaviorFactories)
  {
    stream >> sType;

    const ezRTTI* pRtti = ezRTTI::FindTypeByName(sType);
    EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown behavior factory type '{0}'", sType);

    pBehavior = pRtti->GetAllocator()->Allocate<ezParticleBehaviorFactory>();

    pBehavior->Load(stream);
  }

  if (uiVersion >= 4)
  {
    for (auto& pType : m_TypeFactories)
    {
      stream >> sType;

      const ezRTTI* pRtti = ezRTTI::FindTypeByName(sType);
      EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown type factory type '{0}'", sType);

      pType = pRtti->GetAllocator()->Allocate<ezParticleTypeFactory>();

      pType->Load(stream);
    }
  }

  SetupDefaultProcessors();
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_System_ParticleSystemDescriptor);
