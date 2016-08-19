#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <CoreUtils/DataProcessing/Stream/StreamElementSpawner.h>
#include <CoreUtils/DataProcessing/Stream/StreamProcessor.h>
#include <CoreUtils/DataProcessing/Stream/StreamElementIterator.h>
#include <GameUtils/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>


ezParticleSystemInstance::ezParticleSystemInstance()
{
  m_bEmitterEnabled = true;
  m_pStreamPosition = nullptr;
  m_pStreamVelocity = nullptr;
  m_pStreamColor = nullptr;
  m_pStreamLifeTime = nullptr;
}


ezParticleSystemInstance::~ezParticleSystemInstance()
{
}

bool ezParticleSystemInstance::HasActiveParticles() const
{
  return m_StreamGroup.GetNumActiveElements() > 0;
}

void ezParticleSystemInstance::ConfigureFromTemplate(const ezParticleSystemDescriptor* pTemplate)
{
  bool allEqual = true;

  // emitters
  {
    const auto& factories = pTemplate->GetEmitterFactories();

    if (factories.GetCount() == m_Emitters.GetCount())
    {
      for (ezUInt32 i = 0; i < factories.GetCount(); ++i)
      {
        if (factories[i]->GetEmitterType() != m_Emitters[i]->GetDynamicRTTI())
        {
          allEqual = false;
          break;
        }
      }
    }
    else
    {
      allEqual = false;
    }
  }

  // initializers
  {
    const auto& factories = pTemplate->GetInitializerFactories();

    if (factories.GetCount() == m_Initializers.GetCount())
    {
      for (ezUInt32 i = 0; i < factories.GetCount(); ++i)
      {
        if (factories[i]->GetInitializerType() != m_Initializers[i]->GetDynamicRTTI())
        {
          allEqual = false;
          break;
        }
      }
    }
    else
    {
      allEqual = false;
    }
  }

  if (!allEqual)
  {
    // recreate emitters and initializers

    m_StreamGroup.ClearStreamElementSpawners();

    // emitters
    {
      m_Emitters.Clear();

      for (const auto pFactory : pTemplate->GetEmitterFactories())
      {
        ezParticleEmitter* pEmitter = pFactory->CreateEmitter(this);
        m_StreamGroup.AddStreamElementSpawner(pEmitter);
        m_Emitters.PushBack(pEmitter);
      }
    }

    // initializers
    {
      m_Initializers.Clear();

      for (const auto pFactory : pTemplate->GetInitializerFactories())
      {
        ezParticleInitializer* pInitializer = pFactory->CreateInitializer(this);
        m_StreamGroup.AddStreamElementSpawner(pInitializer);
        m_Initializers.PushBack(pInitializer);
      }
    }
  }
  else
  {
    // just re-initialize the emitters with the new properties

    // emitters
    {
      const auto& factories = pTemplate->GetEmitterFactories();

      for (ezUInt32 i = 0; i < factories.GetCount(); ++i)
      {
        factories[i]->CopyEmitterProperties(m_Emitters[i]);
        m_Emitters[i]->AfterPropertiesConfigured(false);
      }
    }

    // initializers
    {
      const auto& factories = pTemplate->GetInitializerFactories();

      for (ezUInt32 i = 0; i < factories.GetCount(); ++i)
      {
        factories[i]->CopyInitializerProperties(m_Initializers[i]);
        m_Initializers[i]->AfterPropertiesConfigured();
      }
    }
  }

  // behaviors
  {
    m_Behaviors.Clear();
    m_StreamGroup.ClearStreamProcessors();

    for (const auto pFactory : pTemplate->GetBehaviorFactories())
    {
      ezParticleBehavior* pBehavior = pFactory->CreateBehavior(this);
      m_StreamGroup.AddStreamProcessor(pBehavior);
      m_Behaviors.PushBack(pBehavior);
    }
  }
}

void ezParticleSystemInstance::Initialize(ezUInt32 uiMaxParticles, ezWorld* pWorld)
{
  m_Random.InitializeFromCurrentTime();

  m_bEmitterEnabled = true;
  m_pWorld = pWorld;

  m_StreamGroup.Clear();
  m_Emitters.Clear();
  m_Initializers.Clear();
  m_Behaviors.Clear();

  m_pStreamPosition = m_StreamGroup.AddStream("Position", ezStream::DataType::Float3);
  m_pStreamVelocity = m_StreamGroup.AddStream("Velocity", ezStream::DataType::Float3);
  m_pStreamColor = m_StreamGroup.AddStream("Color", ezStream::DataType::Float4);
  m_pStreamLifeTime = m_StreamGroup.AddStream("LifeTime", ezStream::DataType::Float2);

  m_StreamGroup.SetSize(uiMaxParticles);
}

void ezParticleSystemInstance::Update(const ezTime& tDiff)
{
  if (m_bEmitterEnabled)
  {
    for (auto pEmitter : m_Emitters)
    {
      const ezUInt32 uiSpawn = pEmitter->ComputeSpawnCount(tDiff);

      if (uiSpawn > 0)
      {
        m_StreamGroup.SpawnElements(uiSpawn);
      }
    }
  }

  for (auto pBehavior : m_Behaviors)
  {
    pBehavior->StepParticleSystem(tDiff);
  }

  m_StreamGroup.Process();
}

