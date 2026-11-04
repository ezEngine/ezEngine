#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Age.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Volume.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Point/ParticleTypePoint.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleSystemDescriptor, 2, ezRTTIDefaultAllocator<ezParticleSystemDescriptor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("Visible", m_bVisible)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("LifeTime", m_LifeTime)->AddAttributes(new ezDefaultValueAttribute(ezVarianceTypeTime(ezTime::MakeFromSeconds(2))), new ezClampValueAttribute(ezTime::MakeFromSeconds(0.0), ezVariant())),
    EZ_MEMBER_PROPERTY("LifeScaleParam", m_sLifeScaleParameter),
    EZ_MEMBER_PROPERTY("OnDeathEvent", m_sOnDeathEvent)->AddAttributes(new ezDynamicStringEnumAttribute("ParticleEventNamesEnum")),
    EZ_ARRAY_MEMBER_PROPERTY("Emitters", m_Emitters)->AddFlags(ezPropertyFlags::PointerOwner)->AddAttributes(new ezMaxArraySizeAttribute(1)),
    EZ_SET_ACCESSOR_PROPERTY("Initializers", GetInitializers, AddInitializer, RemoveInitializer)->AddFlags(ezPropertyFlags::PointerOwner)->AddAttributes(new ezPreventDuplicatesAttribute()),
    EZ_SET_ACCESSOR_PROPERTY("Behaviors", GetBehaviors, AddBehavior, RemoveBehavior)->AddFlags(ezPropertyFlags::PointerOwner),
    EZ_SET_ACCESSOR_PROPERTY("Types", GetTypes, AddType, RemoveType)->AddFlags(ezPropertyFlags::PointerOwner),
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
  for (auto pModule : m_Emitters)
  {
    pModule->GetDynamicRTTI()->GetAllocator()->Deallocate(pModule);
  }

  m_Emitters.Clear();
}

void ezParticleSystemDescriptor::ClearInitializers()
{
  for (auto pModule : m_Initializers)
  {
    pModule->GetDynamicRTTI()->GetAllocator()->Deallocate(pModule);
  }

  m_Initializers.Clear();
}

void ezParticleSystemDescriptor::ClearBehaviors()
{
  for (auto pModule : m_Behaviors)
  {
    pModule->GetDynamicRTTI()->GetAllocator()->Deallocate(pModule);
  }

  m_Behaviors.Clear();
}

void ezParticleSystemDescriptor::ClearTypes()
{
  for (auto pModule : m_Types)
  {
    pModule->GetDynamicRTTI()->GetAllocator()->Deallocate(pModule);
  }

  m_Types.Clear();
}

void ezParticleSystemDescriptor::ClearFinalizers()
{
  for (auto pModule : m_Finalizers)
  {
    pModule->GetDynamicRTTI()->GetAllocator()->Deallocate(pModule);
  }

  m_Finalizers.Clear();
}

void ezParticleSystemDescriptor::SetupDefaultProcessors()
{
  // Age Behavior
  {
    ezParticleFinalizer_Age* pModule =
      ezParticleFinalizer_Age::GetStaticRTTI()->GetAllocator()->Allocate<ezParticleFinalizer_Age>();
    pModule->m_LifeTime = m_LifeTime;
    pModule->m_sOnDeathEvent = m_sOnDeathEvent;
    pModule->m_sLifeScaleParameter = m_sLifeScaleParameter;
    m_Finalizers.PushBack(pModule);
  }

  // Bounding Volume Update Behavior
  {
    ezParticleFinalizer_Volume* pModule =
      ezParticleFinalizer_Volume::GetStaticRTTI()->GetAllocator()->Allocate<ezParticleFinalizer_Volume>();
    m_Finalizers.PushBack(pModule);
  }

  if (m_Types.IsEmpty())
  {
    ezParticleTypePoint* pModule = ezParticleTypePoint::GetStaticRTTI()->GetAllocator()->Allocate<ezParticleTypePoint>();
    m_Types.PushBack(pModule);
  }

  ezSet<const ezRTTI*> finalizers;
  for (const auto* pModule : m_Initializers)
  {
    pModule->QueryFinalizerDependencies(finalizers);
  }

  for (const auto* pModule : m_Behaviors)
  {
    pModule->QueryFinalizerDependencies(finalizers);
  }

  for (const auto* pModule : m_Types)
  {
    pModule->QueryFinalizerDependencies(finalizers);
  }

  for (const ezRTTI* pRtti : finalizers)
  {
    EZ_ASSERT_DEBUG(
      pRtti->IsDerivedFrom<ezParticleFinalizer>(), "Invalid finalizer added as a dependency: '{0}'", pRtti->GetTypeName());
    EZ_ASSERT_DEBUG(pRtti->GetAllocator()->CanAllocate(), "Finalizer cannot be allocated: '{0}'", pRtti->GetTypeName());

    m_Finalizers.PushBack(pRtti->GetAllocator()->Allocate<ezParticleFinalizer>());
  }
}

void ezParticleSystemDescriptor::SetupInstanceData()
{
  m_InstanceDataAllocator.ClearDescs();

  auto addModules = [&](const auto& modules)
  {
    for (ezParticleModule* pModule : modules)
    {
      ezInstanceDataDesc desc;
      if (pModule->GetInstanceDataDesc(desc))
      {
        pModule->m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(desc);
      }
    }
  };

  addModules(m_Emitters);
  addModules(m_Initializers);
  addModules(m_Behaviors);
  addModules(m_Finalizers);
  addModules(m_Types);
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

void ezParticleSystemDescriptor::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)ParticleSystemVersion::Version_Current;

  inout_stream << uiVersion;

  const ezUInt32 uiNumEmitters = m_Emitters.GetCount();
  const ezUInt32 uiNumInitializers = m_Initializers.GetCount();
  const ezUInt32 uiNumBehaviors = m_Behaviors.GetCount();
  const ezUInt32 uiNumTypes = m_Types.GetCount();

  ezUInt32 uiMaxParticles = 0;
  inout_stream << m_bVisible;
  inout_stream << uiMaxParticles;
  inout_stream << m_LifeTime.m_Value;
  inout_stream << m_LifeTime.m_fVariance;
  inout_stream << m_sOnDeathEvent;
  inout_stream << m_sLifeScaleParameter;
  inout_stream << uiNumEmitters;
  inout_stream << uiNumInitializers;
  inout_stream << uiNumBehaviors;
  inout_stream << uiNumTypes;

  for (auto pEmitter : m_Emitters)
  {
    inout_stream << pEmitter->GetDynamicRTTI()->GetTypeName();

    pEmitter->Save(inout_stream);
  }

  for (auto pInitializer : m_Initializers)
  {
    inout_stream << pInitializer->GetDynamicRTTI()->GetTypeName();

    pInitializer->Save(inout_stream);
  }

  for (auto pBehavior : m_Behaviors)
  {
    inout_stream << pBehavior->GetDynamicRTTI()->GetTypeName();

    pBehavior->Save(inout_stream);
  }

  for (auto pType : m_Types)
  {
    inout_stream << pType->GetDynamicRTTI()->GetTypeName();

    pType->Save(inout_stream);
  }
}


void ezParticleSystemDescriptor::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor)
{
  ClearEmitters();
  ClearInitializers();
  ClearBehaviors();
  ClearFinalizers();
  ClearTypes();

  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;
  EZ_ASSERT_DEV(uiVersion <= (int)ParticleSystemVersion::Version_Current, "Unknown particle template version {0}", uiVersion);

  ezUInt32 uiNumEmitters = 0;
  ezUInt32 uiNumInitializers = 0;
  ezUInt32 uiNumBehaviors = 0;
  ezUInt32 uiNumTypes = 0;

  if (uiVersion >= 3)
  {
    inout_stream >> m_bVisible;
  }

  if (uiVersion >= 2)
  {
    // now unused
    ezUInt32 uiMaxParticles = 0;
    inout_stream >> uiMaxParticles;
  }

  if (uiVersion >= 5)
  {
    inout_stream >> m_LifeTime.m_Value;
    inout_stream >> m_LifeTime.m_fVariance;
    inout_stream >> m_sOnDeathEvent;
  }

  if (uiVersion >= 7)
  {
    inout_stream >> m_sLifeScaleParameter;
  }

  inout_stream >> uiNumEmitters;

  if (uiVersion >= 2)
  {
    inout_stream >> uiNumInitializers;
  }

  inout_stream >> uiNumBehaviors;

  if (uiVersion >= 4)
  {
    inout_stream >> uiNumTypes;
  }

  m_Emitters.SetCountUninitialized(uiNumEmitters);
  m_Initializers.SetCountUninitialized(uiNumInitializers);
  m_Behaviors.SetCountUninitialized(uiNumBehaviors);
  m_Types.SetCountUninitialized(uiNumTypes);

  ezStringBuilder sType;

  for (auto& pEmitter : m_Emitters)
  {
    inout_stream >> sType;

    const ezRTTI* pRtti = ezParticleEffectDescriptor::FindModuleType(sType);
    EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown emitter type '{0}'", sType);

    pEmitter = pRtti->GetAllocator()->Allocate<ezParticleEmitter>();

    pEmitter->Load(inout_stream, ownerEffectDescriptor, *this);
  }

  if (uiVersion >= 2)
  {
    for (auto& pInitializer : m_Initializers)
    {
      inout_stream >> sType;

      const ezRTTI* pRtti = ezParticleEffectDescriptor::FindModuleType(sType);
      EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown initializer type '{0}'", sType);

      pInitializer = pRtti->GetAllocator()->Allocate<ezParticleInitializer>();

      pInitializer->Load(inout_stream, ownerEffectDescriptor, *this);
    }
  }

  for (auto& pBehavior : m_Behaviors)
  {
    inout_stream >> sType;

    const ezRTTI* pRtti = ezParticleEffectDescriptor::FindModuleType(sType);
    EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown behavior type '{0}'", sType);

    pBehavior = pRtti->GetAllocator()->Allocate<ezParticleBehavior>();

    pBehavior->Load(inout_stream, ownerEffectDescriptor, *this);
  }

  if (uiVersion >= 4)
  {
    for (auto& pType : m_Types)
    {
      inout_stream >> sType;

      const ezRTTI* pRtti = ezParticleEffectDescriptor::FindModuleType(sType);
      EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown particle type '{0}'", sType);

      pType = pRtti->GetAllocator()->Allocate<ezParticleType>();

      pType->Load(inout_stream, ownerEffectDescriptor, *this);
    }
  }

  SetupDefaultProcessors();
  SetupInstanceData();
}

//////////////////////////////////////////////////////////////////////////

class ezParticleSystemDescriptor_1_2 : public ezGraphPatch
{
public:
  ezParticleSystemDescriptor_1_2()
    : ezGraphPatch("ezParticleSystemDescriptor", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("LifeTime").IgnoreResult();
  }
};

ezParticleSystemDescriptor_1_2 g_ezParticleSystemDescriptor_1_2;


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_System_ParticleSystemDescriptor);
