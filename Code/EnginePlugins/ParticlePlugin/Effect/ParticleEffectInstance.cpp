#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Events/ParticleEventReaction.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
ezCVarBool cvar_ParticlesDebugWindSamples("Particles.DebugWindSamples", false, ezCVarFlags::Default, "Enables debug visualization for wind sampling on particle effects.");
#endif

ezParticleEffectInstance::ezParticleEffectInstance()
{
  m_pTask = EZ_DEFAULT_NEW(ezParticleEffectUpdateTask, this);
  m_pTask->ConfigureTask("Particle Effect Update", ezTaskNesting::Maybe);

  m_pOwnerModule = nullptr;

  Destruct();
}

ezParticleEffectInstance::~ezParticleEffectInstance()
{
  Destruct();
}

void ezParticleEffectInstance::Construct(ezParticleEffectHandle hEffectHandle, const ezParticleEffectResourceHandle& hResource, ezWorld* pWorld, ezParticleWorldModule* pOwnerModule, ezUInt64 uiRandomSeed, bool bIsShared, ezArrayPtr<ezParticleEffectFloatParam> floatParams, ezArrayPtr<ezParticleEffectColorParam> colorParams)
{
  m_hEffectHandle = hEffectHandle;
  m_pWorld = pWorld;
  m_pOwnerModule = pOwnerModule;
  m_hResource = hResource;
  m_bIsSharedEffect = bIsShared;
  m_bEmitterEnabled = true;
  m_bIsFinishing = false;
  m_BoundingVolume = ezBoundingBoxSphere::MakeInvalid();
  m_ElapsedTimeSinceUpdate = ezTime::MakeZero();
  m_EffectIsVisible = ezTime::MakeZero();
  m_iMinSimStepsToDo = 4;
  m_Transform.SetIdentity();
  m_TransformForNextFrame.SetIdentity();
  m_vVelocity.SetZero();
  m_vVelocityForNextFrame.SetZero();
  m_TotalEffectLifeTime = ezTime::MakeZero();
  m_pVisibleIf = nullptr;
  m_uiRandomSeed = uiRandomSeed;

  if (uiRandomSeed == 0)
    m_Random.InitializeFromCurrentTime();
  else
    m_Random.Initialize(uiRandomSeed);

  Reconfigure(true, floatParams, colorParams);
}

void ezParticleEffectInstance::Destruct()
{
  Interrupt();

  m_SharedInstances.Clear();
  m_hEffectHandle.Invalidate();

  m_Transform.SetIdentity();
  m_TransformForNextFrame.SetIdentity();
  m_bIsSharedEffect = false;
  m_pWorld = nullptr;
  m_hResource.Invalidate();
  m_hEffectHandle.Invalidate();
  m_uiReviveTimeout = 5;

  m_WindSampleGrids[0] = nullptr;
  m_WindSampleGrids[1] = nullptr;
}

void ezParticleEffectInstance::Interrupt()
{
  ClearParticleSystems();
  ClearEventReactions();
  m_bEmitterEnabled = false;
}

void ezParticleEffectInstance::SetEmitterEnabled(bool bEnable)
{
  m_bEmitterEnabled = bEnable;

  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      m_ParticleSystems[i]->SetEmitterEnabled(m_bEmitterEnabled);
    }
  }
}


bool ezParticleEffectInstance::HasActiveParticles() const
{
  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      if (m_ParticleSystems[i]->HasActiveParticles())
        return true;
    }
  }

  return false;
}


void ezParticleEffectInstance::ClearParticleSystem(ezUInt32 index)
{
  if (m_ParticleSystems[index])
  {
    m_pOwnerModule->DestroySystemInstance(m_ParticleSystems[index]);
    m_ParticleSystems[index] = nullptr;
  }
}

void ezParticleEffectInstance::ClearParticleSystems()
{
  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    ClearParticleSystem(i);
  }

  m_ParticleSystems.Clear();
}


void ezParticleEffectInstance::ClearEventReactions()
{
  for (ezUInt32 i = 0; i < m_EventReactions.GetCount(); ++i)
  {
    if (m_EventReactions[i])
    {
      m_EventReactions[i]->GetDynamicRTTI()->GetAllocator()->Deallocate(m_EventReactions[i]);
    }
  }

  m_EventReactions.Clear();
}

bool ezParticleEffectInstance::IsContinuous() const
{
  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      if (m_ParticleSystems[i]->IsContinuous())
        return true;
    }
  }

  return false;
}

void ezParticleEffectInstance::PreSimulate()
{
  if (m_PreSimulateDuration.GetSeconds() == 0.0)
    return;

  PassTransformToSystems();

  // Pre-simulate the effect, if desired, to get it into a 'good looking' state

  // simulate in large steps to get close
  {
    const ezTime tDiff = ezTime::MakeFromSeconds(0.5);
    while (m_PreSimulateDuration.GetSeconds() > 10.0)
    {
      StepSimulation(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // finer steps
  {
    const ezTime tDiff = ezTime::MakeFromSeconds(0.2);
    while (m_PreSimulateDuration.GetSeconds() > 5.0)
    {
      StepSimulation(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // even finer
  {
    const ezTime tDiff = ezTime::MakeFromSeconds(0.1);
    while (m_PreSimulateDuration.GetSeconds() >= 0.1)
    {
      StepSimulation(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // final step if necessary
  if (m_PreSimulateDuration.GetSeconds() > 0.0)
  {
    StepSimulation(m_PreSimulateDuration);
    m_PreSimulateDuration = ezTime::MakeFromSeconds(0);
  }

  if (!IsContinuous())
  {
    // Can't check this at the beginning, because the particle systems are only set up during StepSimulation.
    ezLog::Warning("Particle pre-simulation is enabled on an effect that is not continuous.");
  }
}

void ezParticleEffectInstance::SetIsVisible() const
{
  // if it is visible this frame, also render it the next few frames
  // this has multiple purposes:
  // 1) it fixes the transition when handing off an effect from a
  //    ezParticleComponent to a ezParticleFinisherComponent
  //    though this would only need one frame overlap
  // 2) The bounding volume for culling is only computed every couple of frames
  //    so it may be too small and culling could be imprecise
  //    by just rendering it the next 100ms, no matter what, the bounding volume
  //    does not need to be updated so frequently
  m_EffectIsVisible = ezClock::GetGlobalClock()->GetAccumulatedTime() + ezTime::MakeFromSeconds(0.1);
}


void ezParticleEffectInstance::SetVisibleIf(ezParticleEffectInstance* pOtherVisible)
{
  EZ_ASSERT_DEV(pOtherVisible != this, "Invalid effect");
  m_pVisibleIf = pOtherVisible;
}

bool ezParticleEffectInstance::IsVisible() const
{
  if (m_pVisibleIf != nullptr)
  {
    return m_pVisibleIf->IsVisible();
  }

  return m_EffectIsVisible >= ezClock::GetGlobalClock()->GetAccumulatedTime();
}

void ezParticleEffectInstance::Reconfigure(bool bFirstTime, ezArrayPtr<ezParticleEffectFloatParam> floatParams, ezArrayPtr<ezParticleEffectColorParam> colorParams)
{
  if (!m_hResource.IsValid())
  {
    ezLog::Error("Effect Reconfigure: Effect Resource is invalid");
    return;
  }

  ezResourceLock<ezParticleEffectResource> pResource(m_hResource, ezResourceAcquireMode::BlockTillLoaded);

  const auto& desc = pResource->GetDescriptor().m_Effect;
  const auto& systems = desc.GetParticleSystems();

  m_Transform.SetIdentity();
  m_TransformForNextFrame.SetIdentity();
  m_vVelocity.SetZero();
  m_vVelocityForNextFrame.SetZero();
  m_fApplyInstanceVelocity = desc.m_fApplyInstanceVelocity;
  m_bSimulateInLocalSpace = desc.m_bSimulateInLocalSpace;
  m_InvisibleUpdateRate = desc.m_InvisibleUpdateRate;

  m_vNumWindSamples.x = desc.m_vNumWindSamples.x;
  m_vNumWindSamples.y = desc.m_vNumWindSamples.y;
  m_vNumWindSamples.z = desc.m_vNumWindSamples.z;
  m_vNumWindSamples.w = desc.m_vNumWindSamples.x * desc.m_vNumWindSamples.y * desc.m_vNumWindSamples.z;
  m_WindSampleGrids[0] = nullptr;
  m_WindSampleGrids[1] = nullptr;

  // parameters
  {
    m_FloatParameters.Clear();
    m_ColorParameters.Clear();

    for (auto it = desc.m_FloatParameters.GetIterator(); it.IsValid(); ++it)
    {
      SetParameter(ezTempHashedString(it.Key().GetData()), it.Value());
    }

    for (auto it = desc.m_ColorParameters.GetIterator(); it.IsValid(); ++it)
    {
      SetParameter(ezTempHashedString(it.Key().GetData()), it.Value());
    }

    // shared effects do not support per-instance parameters
    if (m_bIsSharedEffect)
    {
      if (!floatParams.IsEmpty() || !colorParams.IsEmpty())
      {
        ezLog::Warning("Shared particle effects do not support effect parameters");
      }
    }
    else
    {
      for (ezUInt32 p = 0; p < floatParams.GetCount(); ++p)
      {
        SetParameter(floatParams[p].m_sName, floatParams[p].m_Value);
      }

      for (ezUInt32 p = 0; p < colorParams.GetCount(); ++p)
      {
        SetParameter(colorParams[p].m_sName, colorParams[p].m_Value);
      }
    }
  }

  if (bFirstTime)
  {
    m_PreSimulateDuration = desc.m_PreSimulateDuration;
  }

  // TODO Check max number of particles etc. to reset

  if (m_ParticleSystems.GetCount() != systems.GetCount())
  {
    // reset everything
    ClearParticleSystems();
  }

  m_ParticleSystems.SetCount(systems.GetCount());

  struct MulCount
  {
    EZ_DECLARE_POD_TYPE();

    float m_fMultiplier = 1.0f;
    ezUInt32 m_uiCount = 0;
  };

  ezHybridArray<MulCount, 8> systemMaxParticles;
  {
    systemMaxParticles.SetCountUninitialized(systems.GetCount());
    for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      ezUInt32 uiMaxParticlesAbs = 0, uiMaxParticlesPerSec = 0;
      for (const ezParticleEmitterFactory* pEmitter : systems[i]->GetEmitterFactories())
      {
        ezUInt32 uiMaxParticlesAbs0 = 0, uiMaxParticlesPerSec0 = 0;
        pEmitter->QueryMaxParticleCount(uiMaxParticlesAbs0, uiMaxParticlesPerSec0);

        uiMaxParticlesAbs += uiMaxParticlesAbs0;
        uiMaxParticlesPerSec += uiMaxParticlesPerSec0;
      }

      const ezTime tLifetime = systems[i]->GetAvgLifetime();

      const ezUInt32 uiMaxParticles = ezMath::Max(32u, ezMath::Max(uiMaxParticlesAbs, (ezUInt32)(uiMaxParticlesPerSec * tLifetime.GetSeconds())));

      float fMultiplier = 1.0f;

      for (const ezParticleInitializerFactory* pInitializer : systems[i]->GetInitializerFactories())
      {
        fMultiplier *= pInitializer->GetSpawnCountMultiplier(this);
      }

      systemMaxParticles[i].m_fMultiplier = ezMath::Max(0.0f, fMultiplier);
      systemMaxParticles[i].m_uiCount = (ezUInt32)(uiMaxParticles * systemMaxParticles[i].m_fMultiplier);
    }
  }
  // delete all that have important changes
  {
    for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      if (m_ParticleSystems[i] != nullptr)
      {
        if (m_ParticleSystems[i]->GetMaxParticles() != systemMaxParticles[i].m_uiCount)
          ClearParticleSystem(i);
      }
    }
  }

  // recreate where necessary
  {
    for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      if (m_ParticleSystems[i] == nullptr)
      {
        m_ParticleSystems[i] = m_pOwnerModule->CreateSystemInstance(systemMaxParticles[i].m_uiCount, m_pWorld, this, systemMaxParticles[i].m_fMultiplier);
      }
    }
  }

  const ezVec3 vStartVelocity = m_vVelocity * m_fApplyInstanceVelocity;

  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    m_ParticleSystems[i]->ConfigureFromTemplate(systems[i]);
    m_ParticleSystems[i]->SetTransform(m_Transform, vStartVelocity);
    m_ParticleSystems[i]->SetEmitterEnabled(m_bEmitterEnabled);
    m_ParticleSystems[i]->Finalize();
  }

  // recreate event reactions
  {
    ClearEventReactions();

    m_EventReactions.SetCount(desc.GetEventReactions().GetCount());

    const auto& er = desc.GetEventReactions();
    for (ezUInt32 i = 0; i < er.GetCount(); ++i)
    {
      if (m_EventReactions[i] == nullptr)
      {
        m_EventReactions[i] = er[i]->CreateEventReaction(this);
      }
    }
  }
}

bool ezParticleEffectInstance::Update(const ezTime& diff)
{
  EZ_PROFILE_SCOPE("PFX: Effect Update");

  ezTime tMinStep = ezTime::MakeFromSeconds(0);

  if (!IsVisible() && m_iMinSimStepsToDo == 0)
  {
    // shared effects always get paused when they are invisible
    if (IsSharedEffect())
      return true;

    switch (m_InvisibleUpdateRate)
    {
      case ezEffectInvisibleUpdateRate::FullUpdate:
        tMinStep = ezTime::MakeFromSeconds(1.0 / 60.0);
        break;

      case ezEffectInvisibleUpdateRate::Max20fps:
        tMinStep = ezTime::MakeFromMilliseconds(50);
        break;

      case ezEffectInvisibleUpdateRate::Max10fps:
        tMinStep = ezTime::MakeFromMilliseconds(100);
        break;

      case ezEffectInvisibleUpdateRate::Max5fps:
        tMinStep = ezTime::MakeFromMilliseconds(200);
        break;

      case ezEffectInvisibleUpdateRate::Pause:
      {
        if (m_bEmitterEnabled)
        {
          // during regular operation, pause
          return m_uiReviveTimeout > 0;
        }

        // otherwise do infrequent updates to shut the effect down
        tMinStep = ezTime::MakeFromMilliseconds(200);
        break;
      }

      case ezEffectInvisibleUpdateRate::Discard:
        Interrupt();
        return false;
    }
  }

  m_ElapsedTimeSinceUpdate += diff;
  PassTransformToSystems();

  // if the time step is too big, iterate multiple times
  {
    const ezTime tMaxTimeStep = ezTime::MakeFromMilliseconds(200); // in sync with Max5fps
    while (m_ElapsedTimeSinceUpdate > tMaxTimeStep)
    {
      m_ElapsedTimeSinceUpdate -= tMaxTimeStep;

      if (!StepSimulation(tMaxTimeStep))
        return false;
    }
  }

  if (m_ElapsedTimeSinceUpdate < tMinStep)
    return m_uiReviveTimeout > 0;

  // do the remainder
  const ezTime tUpdateDiff = m_ElapsedTimeSinceUpdate;
  m_ElapsedTimeSinceUpdate = ezTime::MakeZero();

  return StepSimulation(tUpdateDiff);
}

bool ezParticleEffectInstance::StepSimulation(const ezTime& tDiff)
{
  m_TotalEffectLifeTime += tDiff;

  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i] != nullptr)
    {
      auto state = m_ParticleSystems[i]->Update(tDiff);

      if (state == ezParticleSystemState::Inactive)
      {
        ClearParticleSystem(i);
      }
      else if (state != ezParticleSystemState::OnlyReacting)
      {
        // this is used to delay particle effect death by a couple of frames
        // that way, if an event is in the pipeline that might trigger a reacting emitter,
        // or particles are in the spawn queue, but not yet created, we don't kill the effect too early
        m_uiReviveTimeout = 3;
      }
    }
  }

  m_iMinSimStepsToDo = ezMath::Max<ezInt8>(m_iMinSimStepsToDo - 1, 0);

  --m_uiReviveTimeout;
  return m_uiReviveTimeout > 0;
}


void ezParticleEffectInstance::AddParticleEvent(const ezParticleEvent& pe)
{
  // drop events when the capacity is full
  if (m_EventQueue.GetCount() == m_EventQueue.GetCapacity())
    return;

  m_EventQueue.PushBack(pe);
}

void ezParticleEffectInstance::RequestWindSamples()
{
  const ezUInt32 uiTotalNumSamples = m_vNumWindSamples.w;

  for (auto& grid : m_WindSampleGrids)
  {
    if (grid == nullptr)
    {
      grid = EZ_NEW(ezFoundation::GetAlignedAllocator(), WindSampleGrid);
      grid->m_vMinPos.Set(1000.0f);
      grid->m_vMaxPos.Set(-1000.0f);
      grid->m_vInvCellSize.SetZero();
      grid->m_Samples.SetCountUninitialized(uiTotalNumSamples);
      ezMemoryUtils::ZeroFill(grid->m_Samples.GetData(), uiTotalNumSamples);
    }
  }
}

void ezParticleEffectInstance::UpdateWindSamples(ezTime diff)
{
  const ezUInt64 uiFrameCounter = ezRenderWorld::GetFrameCounter();
  const ezUInt32 uiDataIdx = uiFrameCounter & 1;
  if (m_WindSampleGrids[uiDataIdx] == nullptr || m_BoundingVolume.IsValid() == false)
    return;

  auto& grid = *m_WindSampleGrids[uiDataIdx];
  const auto& oldGrid = *m_WindSampleGrids[(uiDataIdx + 1) & 1];

  const ezUInt32 uiNumSamplesX = m_vNumWindSamples.x;
  const ezUInt32 uiNumSamplesY = m_vNumWindSamples.y;
  const ezUInt32 uiNumSamplesZ = m_vNumWindSamples.z;
  const ezUInt32 uiTotalNumSamples = m_vNumWindSamples.w;
  EZ_ASSERT_DEBUG(grid.m_Samples.GetCount() == uiTotalNumSamples && oldGrid.m_Samples.GetCount() == uiTotalNumSamples, "Invalid number of samples");

  const ezSimdVec4f interpolationFactor = ezSimdVec4f(1.0f - ezMath::Pow(0.1f, diff.AsFloatInSeconds()));

  const ezSimdBBox boundingBox = ezSimdConversion::ToBBox(m_BoundingVolume.GetBox());
  const ezSimdVec4f boundsSize = boundingBox.GetExtents();
  const ezSimdVec4f gridSize = ezSimdVec4f(uiNumSamplesX, uiNumSamplesY, uiNumSamplesZ);
  ezSimdVec4f cellSize = boundsSize.CompDiv(gridSize);
  const ezSimdVec4f minPos = boundingBox.m_Min + cellSize * 0.5f;
  const ezSimdVec4f maxPos = boundingBox.m_Max - cellSize * 0.5f;

  const ezSimdVec4b oldGridValid = oldGrid.m_vMinPos < oldGrid.m_vMaxPos;
  grid.m_vMinPos = ezSimdVec4f::Select(oldGridValid, ezSimdVec4f::Lerp(oldGrid.m_vMinPos, minPos, interpolationFactor), minPos);
  grid.m_vMaxPos = ezSimdVec4f::Select(oldGridValid, ezSimdVec4f::Lerp(oldGrid.m_vMaxPos, maxPos, interpolationFactor), maxPos);

  const ezSimdVec4f finalGridSize = grid.m_vMaxPos - grid.m_vMinPos;
  const ezSimdVec4f maxIndices = gridSize - ezSimdVec4f(1.0f);
  cellSize = ezSimdVec4f::Select(maxIndices != ezSimdVec4f::MakeZero(), finalGridSize.CompDiv(maxIndices), ezSimdVec4f::MakeZero());
  grid.m_vInvCellSize = ezSimdVec4f::Select(cellSize != ezSimdVec4f::MakeZero(), ezSimdVec4f(1.0f).CompDiv(cellSize), ezSimdVec4f::MakeZero());
  EZ_ASSERT_DEBUG(grid.m_vInvCellSize.IsValid<3>(), "");

  if (auto pWind = GetWorld()->GetModuleReadOnly<ezWindWorldModuleInterface>())
  {
    for (ezUInt32 i = 0; i < uiTotalNumSamples; ++i)
    {
      ezUInt32 index = i;
      const ezUInt32 z = i / (uiNumSamplesX * uiNumSamplesY);
      index -= z * (uiNumSamplesX * uiNumSamplesY);
      const ezUInt32 y = index / uiNumSamplesX;
      const ezUInt32 x = index - (y * uiNumSamplesX);

      const ezSimdVec4f samplePos = grid.m_vMinPos + cellSize.CompMul(ezSimdVec4f(x, y, z));

      grid.m_Samples[i] = ezSimdVec4f::Lerp(oldGrid.m_Samples[i], pWind->GetWindAtSimd(samplePos), interpolationFactor);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      if (cvar_ParticlesDebugWindSamples)
      {
        const ezColor c = ezColorScheme::GetColor(ezColorScheme::Blue, 8);

        const ezVec3 samplePos0 = ezSimdConversion::ToVec3(samplePos);
        ezDebugRenderer::DrawCross(GetWorld(), samplePos0, 0.1f, c);

        const ezVec3 vWind = ezSimdConversion::ToVec3(grid.m_Samples[i]);
        const float fWindStrength = vWind.GetLength();
        ezDebugRenderer::Draw3DText(GetWorld(), ezFmt("{} m/s", ezArgF(fWindStrength, 2)), samplePos0, c);

        if (fWindStrength > 0.01f)
        {
          const ezQuat q = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), vWind);
          ezDebugRenderer::DrawArrow(GetWorld(), fWindStrength, c, ezTransform::Make(samplePos0, q));
        }
      }
#endif
    }
  }
  else
  {
    for (auto& sample : grid.m_Samples)
    {
      sample.SetZero();
    }
  }
}

ezUInt64 ezParticleEffectInstance::GetNumActiveParticles() const
{
  ezUInt64 num = 0;

  for (auto pSystem : m_ParticleSystems)
  {
    if (pSystem)
    {
      num += pSystem->GetNumActiveParticles();
    }
  }

  return num;
}

void ezParticleEffectInstance::SetTransform(const ezTransform& transform, const ezVec3& vParticleStartVelocity)
{
  m_Transform = transform;
  m_TransformForNextFrame = transform;

  m_vVelocity = vParticleStartVelocity;
  m_vVelocityForNextFrame = vParticleStartVelocity;
}

void ezParticleEffectInstance::SetTransformForNextFrame(const ezTransform& transform, const ezVec3& vParticleStartVelocity)
{
  m_TransformForNextFrame = transform;
  m_vVelocityForNextFrame = vParticleStartVelocity;
}

ezSimdVec4f ezParticleEffectInstance::GetWindAt(const ezSimdVec4f& vPosition) const
{
  const ezUInt64 uiFrameCounter = ezRenderWorld::GetFrameCounter();
  const ezUInt32 uiDataIdx = (uiFrameCounter + 1) & 1;
  if (m_WindSampleGrids[uiDataIdx] == nullptr)
  {
    return ezSimdVec4f::MakeZero();
  }

  auto& grid = *m_WindSampleGrids[uiDataIdx];

  const ezUInt32 uiTotalNumSamples = m_vNumWindSamples.w;
  EZ_ASSERT_DEBUG(grid.m_Samples.GetCount() == uiTotalNumSamples, "Invalid sample count");

  // Sample grid with trilinear interpolation
  ezSimdVec4f gridSpacePos = (vPosition - grid.m_vMinPos).CompMul(grid.m_vInvCellSize);
  gridSpacePos = gridSpacePos.CompMax(ezSimdVec4f::MakeZero());

  const ezSimdVec4f gridSpacePosFloor = gridSpacePos.Floor();
  const ezSimdVec4f weights = gridSpacePos - gridSpacePosFloor;

  const ezSimdVec4i maxIndices = ezSimdConversion::ToVec4i(m_vNumWindSamples) - ezSimdVec4i(1);
  const ezSimdVec4i pos0 = ezSimdVec4i::Truncate(gridSpacePosFloor).CompMin(maxIndices);
  const ezSimdVec4i pos1 = (pos0 + ezSimdVec4i(1)).CompMin(maxIndices);

  const ezInt32 xCount = m_vNumWindSamples.x;
  const ezInt32 xyCount = xCount * m_vNumWindSamples.y;
  const ezSimdVec4i cXcXYcXcXY = ezSimdVec4i(xCount, xyCount, xCount, xyCount);
  const ezSimdVec4i y0z0y1z1 = pos0.GetCombined<ezSwizzle::YZYZ>(pos1).CompMul(cXcXYcXcXY);
  const ezSimdVec4i y0y0y1y1 = y0z0y1z1.Get<ezSwizzle::XXZZ>();
  const ezSimdVec4i x0x0x1x1 = pos0.GetCombined<ezSwizzle::XXXX>(pos1);
  const ezSimdVec4i x0x1x0x1 = x0x0x1x1.Get<ezSwizzle::XZXZ>();
  const ezSimdVec4i y0y0y1y1_plus_x0x1x0x1 = y0y0y1y1 + x0x1x0x1;

  const ezSimdVec4f wX = weights.Get<ezSwizzle::XXXX>();
  const ezSimdVec4f wY = weights.Get<ezSwizzle::YYYY>();

  const ezSimdVec4f* pSamples = grid.m_Samples.GetData();

  const ezSimdVec4i indices_z0 = y0z0y1z1.Get<ezSwizzle::YYYY>() + y0y0y1y1_plus_x0x1x0x1;
  const ezSimdVec4f sample_z0y0x0 = pSamples[indices_z0.x()];
  const ezSimdVec4f sample_z0y0x1 = pSamples[indices_z0.y()];
  const ezSimdVec4f res_z0y0 = ezSimdVec4f::Lerp(sample_z0y0x0, sample_z0y0x1, wX);

  const ezSimdVec4f sample_z0y1x0 = pSamples[indices_z0.z()];
  const ezSimdVec4f sample_z0y1x1 = pSamples[indices_z0.w()];
  const ezSimdVec4f res_z0y1 = ezSimdVec4f::Lerp(sample_z0y1x0, sample_z0y1x1, wX);

  const ezSimdVec4f res_z0 = ezSimdVec4f::Lerp(res_z0y0, res_z0y1, wY);

  const ezSimdVec4i indices_z1 = y0z0y1z1.Get<ezSwizzle::WWWW>() + y0y0y1y1_plus_x0x1x0x1;
  const ezSimdVec4f sample_z1y0x0 = pSamples[indices_z1.x()];
  const ezSimdVec4f sample_z1y0x1 = pSamples[indices_z1.y()];
  const ezSimdVec4f res_z1y0 = ezSimdVec4f::Lerp(sample_z1y0x0, sample_z1y0x1, wX);

  const ezSimdVec4f sample_z1y1x0 = pSamples[indices_z1.z()];
  const ezSimdVec4f sample_z1y1x1 = pSamples[indices_z1.w()];
  const ezSimdVec4f res_z1y1 = ezSimdVec4f::Lerp(sample_z1y1x0, sample_z1y1x1, wX);

  const ezSimdVec4f res_z1 = ezSimdVec4f::Lerp(res_z1y0, res_z1y1, wY);

  const ezSimdVec4f wZ = weights.Get<ezSwizzle::ZZZZ>();
  const ezSimdVec4f res = ezSimdVec4f::Lerp(res_z0, res_z1, wZ);

  return res;
}

void ezParticleEffectInstance::PassTransformToSystems()
{
  if (!m_bSimulateInLocalSpace)
  {
    const ezVec3 vStartVel = m_vVelocity * m_fApplyInstanceVelocity;

    for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      if (m_ParticleSystems[i] != nullptr)
      {
        m_ParticleSystems[i]->SetTransform(m_Transform, vStartVel);
      }
    }
  }
}

void ezParticleEffectInstance::AddSharedInstance(const void* pSharedInstanceOwner)
{
  m_SharedInstances.Insert(pSharedInstanceOwner);
}

void ezParticleEffectInstance::RemoveSharedInstance(const void* pSharedInstanceOwner)
{
  m_SharedInstances.Remove(pSharedInstanceOwner);
}

bool ezParticleEffectInstance::ShouldBeUpdated() const
{
  if (m_hEffectHandle.IsInvalidated())
    return false;

  // do not update shared instances when there is no one watching
  if (m_bIsSharedEffect && m_SharedInstances.GetCount() == 0)
    return false;

  return true;
}

void ezParticleEffectInstance::GetBoundingVolume(ezBoundingBoxSphere& ref_volume) const
{
  if (!m_BoundingVolume.IsValid())
  {
    ref_volume = ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), 0.25f);
    return;
  }

  ref_volume = m_BoundingVolume;

  if (!m_bSimulateInLocalSpace)
  {
    // transform the bounding volume to local space, unless it was already created there
    const ezMat4 invTrans = GetTransform().GetAsMat4().GetInverse();
    ref_volume.Transform(invTrans);
  }
}

void ezParticleEffectInstance::CombineSystemBoundingVolumes()
{
  ezBoundingBoxSphere effectVolume = ezBoundingBoxSphere::MakeInvalid();

  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      const ezBoundingBoxSphere& systemVolume = m_ParticleSystems[i]->GetBoundingVolume();
      if (systemVolume.IsValid())
      {
        effectVolume.ExpandToInclude(systemVolume);
      }
    }
  }

  m_BoundingVolume = effectVolume;
}

void ezParticleEffectInstance::ProcessEventQueues()
{
  m_Transform = m_TransformForNextFrame;
  m_vVelocity = m_vVelocityForNextFrame;

  if (m_EventQueue.IsEmpty())
    return;

  EZ_PROFILE_SCOPE("PFX: Effect Event Queue");
  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      m_ParticleSystems[i]->ProcessEventQueue(m_EventQueue);
    }
  }

  for (const ezParticleEvent& e : m_EventQueue)
  {
    ezUInt32 rnd = m_Random.UIntInRange(100);

    for (ezParticleEventReaction* pReaction : m_EventReactions)
    {
      if (pReaction->m_sEventName != e.m_EventType)
        continue;

      if (pReaction->m_uiProbability > rnd)
      {
        pReaction->ProcessEvent(e);
        break;
      }

      rnd -= pReaction->m_uiProbability;
    }
  }

  m_EventQueue.Clear();
}

ezParticleEffectUpdateTask::ezParticleEffectUpdateTask(ezParticleEffectInstance* pEffect)
{
  m_pEffect = pEffect;
  m_UpdateDiff = ezTime::MakeZero();
}

void ezParticleEffectUpdateTask::Execute()
{
  if (HasBeenCanceled())
    return;

  if (m_UpdateDiff.GetSeconds() != 0.0)
  {
    m_pEffect->PreSimulate();

    if (!m_pEffect->Update(m_UpdateDiff))
    {
      const ezParticleEffectHandle hEffect = m_pEffect->GetHandle();
      EZ_ASSERT_DEBUG(!hEffect.IsInvalidated(), "Invalid particle effect handle");

      m_pEffect->GetOwnerWorldModule()->DestroyEffectInstance(hEffect, true, nullptr);
    }
  }
}

void ezParticleEffectInstance::SetParameter(const ezTempHashedString& sName, float value)
{
  // shared effects do not support parameters
  if (m_bIsSharedEffect)
    return;

  for (ezUInt32 i = 0; i < m_FloatParameters.GetCount(); ++i)
  {
    if (m_FloatParameters[i].m_uiNameHash == sName.GetHash())
    {
      m_FloatParameters[i].m_fValue = value;
      return;
    }
  }

  auto& ref = m_FloatParameters.ExpandAndGetRef();
  ref.m_uiNameHash = sName.GetHash();
  ref.m_fValue = value;
}

void ezParticleEffectInstance::SetParameter(const ezTempHashedString& sName, const ezColor& value)
{
  // shared effects do not support parameters
  if (m_bIsSharedEffect)
    return;

  for (ezUInt32 i = 0; i < m_ColorParameters.GetCount(); ++i)
  {
    if (m_ColorParameters[i].m_uiNameHash == sName.GetHash())
    {
      m_ColorParameters[i].m_Value = value;
      return;
    }
  }

  auto& ref = m_ColorParameters.ExpandAndGetRef();
  ref.m_uiNameHash = sName.GetHash();
  ref.m_Value = value;
}

ezInt32 ezParticleEffectInstance::FindFloatParameter(const ezTempHashedString& sName) const
{
  for (ezUInt32 i = 0; i < m_FloatParameters.GetCount(); ++i)
  {
    if (m_FloatParameters[i].m_uiNameHash == sName.GetHash())
      return i;
  }

  return -1;
}

float ezParticleEffectInstance::GetFloatParameter(const ezTempHashedString& sName, float fDefaultValue) const
{
  if (sName.IsEmpty())
    return fDefaultValue;

  for (ezUInt32 i = 0; i < m_FloatParameters.GetCount(); ++i)
  {
    if (m_FloatParameters[i].m_uiNameHash == sName.GetHash())
      return m_FloatParameters[i].m_fValue;
  }

  return fDefaultValue;
}

ezInt32 ezParticleEffectInstance::FindColorParameter(const ezTempHashedString& sName) const
{
  for (ezUInt32 i = 0; i < m_ColorParameters.GetCount(); ++i)
  {
    if (m_ColorParameters[i].m_uiNameHash == sName.GetHash())
      return i;
  }

  return -1;
}

const ezColor& ezParticleEffectInstance::GetColorParameter(const ezTempHashedString& sName, const ezColor& defaultValue) const
{
  if (sName.IsEmpty())
    return defaultValue;

  for (ezUInt32 i = 0; i < m_ColorParameters.GetCount(); ++i)
  {
    if (m_ColorParameters[i].m_uiNameHash == sName.GetHash())
      return m_ColorParameters[i].m_Value;
  }

  return defaultValue;
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Effect_ParticleEffectInstance);
