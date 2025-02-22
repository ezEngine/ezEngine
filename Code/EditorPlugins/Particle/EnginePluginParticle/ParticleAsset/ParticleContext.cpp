#include <EnginePluginParticle/EnginePluginParticlePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <EnginePluginParticle/ParticleAsset/ParticleContext.h>
#include <EnginePluginParticle/ParticleAsset/ParticleView.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleContext, 1, ezRTTIDefaultAllocator<ezParticleContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Particle Effect"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleContext::ezParticleContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::CreateWorld)
{
}

ezParticleContext::~ezParticleContext() = default;

void ezParticleContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezSimulationSettingsMsgToEngine>())
  {
    // this message comes exactly once per 'update', afterwards there will be 1 to n redraw messages

    auto msg = static_cast<const ezSimulationSettingsMsgToEngine*>(pMsg);

    m_pWorld->SetWorldSimulationEnabled(msg->m_bSimulateWorld);
    m_pWorld->GetClock().SetSpeed(msg->m_fSimulationSpeed);
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineRestartSimulationMsg>())
  {
    RestartEffect();
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineLoopAnimationMsg>())
  {
    SetAutoRestartEffect(((const ezEditorEngineLoopAnimationMsg*)pMsg)->m_bLoop);
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}

void ezParticleContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  EZ_LOCK(pWorld->GetWriteMarker());

  ezParticleComponentManager* pCompMan = pWorld->GetOrCreateComponentManager<ezParticleComponentManager>();


  // Preview Effect
  {
    ezGameObjectDesc obj;
    ezGameObject* pObj;
    obj.m_sName.Assign("ParticlePreview");
    pWorld->CreateObject(obj, pObj);

    pCompMan->CreateComponent(pObj, m_pComponent);
    m_pComponent->m_OnFinishedAction = ezOnComponentFinishedAction2::Restart;
    m_pComponent->m_MinRestartDelay = ezTime::MakeFromSeconds(0.5);

    ezStringBuilder sParticleGuid;
    ezConversionUtils::ToString(GetDocumentGuid(), sParticleGuid);
    m_hParticle = ezResourceManager::LoadResource<ezParticleEffectResource>(sParticleGuid);

    m_pComponent->SetParticleEffect(m_hParticle);
  }

  const char* szMeshName = "ParticlePreviewBackgroundMesh";
  m_hPreviewMeshResource = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshName);

  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "ParticlePreviewBackgroundMeshBuffer";

    ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szMeshBufferName);

    if (!hMeshBuffer.IsValid())
    {
      // Build geometry
      ezGeometry geom;

      geom.AddBox(ezVec3(4, 4, 4), true);
      geom.ComputeTangents();

      ezMeshBufferResourceDescriptor desc;
      desc.AddCommonStreams();
      desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

      hMeshBuffer = ezResourceManager::GetOrCreateResource<ezMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
    }
    {
      ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer, ezResourceAcquireMode::AllowLoadingFallback);

      ezMeshResourceDescriptor md;
      md.UseExistingMeshBuffer(hMeshBuffer);
      md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
      md.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }"); // Pattern.ezMaterialAsset
      md.ComputeBounds();

      m_hPreviewMeshResource = ezResourceManager::GetOrCreateResource<ezMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
    }
  }

  ezPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>();

  // Background Mesh
  {
    ezGameObjectDesc obj;
    obj.m_sName.Assign("ParticleBackground");

    const ezColor bgColor(0.3f, 0.3f, 0.3f);

    for (int y = -1; y <= 5; ++y)
    {
      for (int x = -5; x <= 5; ++x)
      {
        ezGameObject* pObj;
        obj.m_LocalPosition.Set(6, (float)x * 4, 1 + (float)y * 4);
        pWorld->CreateObject(obj, pObj);

        ezMeshComponent* pMesh;
        ezMeshComponent::CreateComponent(pObj, pMesh);
        pMesh->SetMesh(m_hPreviewMeshResource);
        pMesh->SetColor(bgColor);

        if (pPhysicsInterface)
          pPhysicsInterface->AddStaticCollisionBox(pObj, ezVec3(4, 4, 4));
      }
    }

    for (int y = -5; y <= 5; ++y)
    {
      for (int x = -5; x <= 1; ++x)
      {
        ezGameObject* pObj;
        obj.m_LocalPosition.Set((float)x * 4, (float)y * 4, -3);
        pWorld->CreateObject(obj, pObj);

        ezMeshComponent* pMesh;
        ezMeshComponent::CreateComponent(pObj, pMesh);
        pMesh->SetMesh(m_hPreviewMeshResource);
        pMesh->SetColor(bgColor);

        if (pPhysicsInterface)
          pPhysicsInterface->AddStaticCollisionBox(pObj, ezVec3(4, 4, 4));
      }
    }

    for (int x = -5; x <= 5; ++x)
    {
      ezGameObject* pObj;
      obj.m_LocalPosition.Set(4, (float)x * 4, -2);
      obj.m_LocalRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::MakeFromDegree(45));
      pWorld->CreateObject(obj, pObj);

      ezMeshComponent* pMesh;
      ezMeshComponent::CreateComponent(pObj, pMesh);
      pMesh->SetMesh(m_hPreviewMeshResource);
      pMesh->SetColor(bgColor);

      if (pPhysicsInterface)
        pPhysicsInterface->AddStaticCollisionBox(pObj, ezVec3(4, 4, 4));
    }
  }
}

ezEngineProcessViewContext* ezParticleContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezParticleViewContext, this);
}

void ezParticleContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

void ezParticleContext::OnThumbnailViewContextRequested()
{
  m_ThumbnailBoundingVolume = ezBoundingBoxSphere::MakeInvalid();
}

bool ezParticleContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  ezParticleViewContext* pParticleViewContext = static_cast<ezParticleViewContext*>(pThumbnailViewContext);

  if (!m_ThumbnailBoundingVolume.IsValid())
  {
    EZ_LOCK(m_pWorld->GetWriteMarker());

    const bool bWorldPaused = m_pWorld->GetClock().GetPaused();

    // make sure the component restarts as soon as possible
    {
      const ezTime restartDelay = m_pComponent->m_MinRestartDelay;
      const auto onFinished = m_pComponent->m_OnFinishedAction;
      const double fClockSpeed = m_pWorld->GetClock().GetSpeed();

      m_pComponent->m_MinRestartDelay = ezTime::MakeZero();
      m_pComponent->m_OnFinishedAction = ezOnComponentFinishedAction2::Restart;

      m_pWorld->SetWorldSimulationEnabled(true);
      m_pWorld->GetClock().SetPaused(false);
      m_pWorld->GetClock().SetSpeed(10);
      m_pWorld->Update();
      m_pWorld->GetClock().SetPaused(bWorldPaused);
      m_pWorld->GetClock().SetSpeed(fClockSpeed);
      m_pWorld->SetWorldSimulationEnabled(false);

      m_pComponent->m_MinRestartDelay = restartDelay;
      m_pComponent->m_OnFinishedAction = onFinished;
    }

    if (m_pComponent && !m_pComponent->m_EffectController.IsAlive())
    {
      // if this happens, the effect has finished and we need to wait for it to restart, so that it can be reconfigured for the screenshot
      // not very clean solution

      pParticleViewContext->PositionThumbnailCamera(m_ThumbnailBoundingVolume);
      return false;
    }

    if (m_pComponent && m_pComponent->m_EffectController.IsAlive())
    {
      // set a fixed random seed
      m_pComponent->m_uiRandomSeed = 11;

      const ezUInt32 uiMinSimSteps = 3;
      ezUInt32 uiSimStepsNeeded = uiMinSimSteps;

      if (m_pComponent->m_EffectController.IsContinuousEffect())
      {
        uiSimStepsNeeded = 30;
      }
      else
      {
        m_pComponent->InterruptEffect();
        m_pComponent->StartEffect();

        ezUInt64 uiMostParticles = 0;
        ezUInt64 uiMostParticlesStep = 0;

        for (ezUInt32 step = 0; step < 30; ++step)
        {
          // step once, to get the initial bbox out of the way
          m_pComponent->m_EffectController.ForceVisible();
          m_pComponent->m_EffectController.Tick(ezTime::MakeFromSeconds(0.05));

          if (!m_pComponent->m_EffectController.IsAlive())
            break;

          const ezUInt64 numParticles = m_pComponent->m_EffectController.GetNumActiveParticles();

          if (step == uiMinSimSteps && numParticles > 0)
          {
            uiMostParticles = 0;
          }

          if (numParticles > uiMostParticles)
          {
            // this is the step with the largest number of particles
            // but usually a few steps later is the best step to capture

            uiMostParticles = numParticles;
            uiMostParticlesStep = step;
            uiSimStepsNeeded = step;
          }
          else if ((numParticles > uiMostParticles * 0.8f) && (step < uiMostParticlesStep + 5))
          {
            // if a few steps later we still have a decent amount of particles (so it didn't drop significantly),
            // prefer to use that step
            uiSimStepsNeeded = step;
          }
        }
      }

      m_pComponent->InterruptEffect();
      m_pComponent->StartEffect();

      for (ezUInt32 step = 0; step < uiSimStepsNeeded; ++step)
      {
        m_pComponent->m_EffectController.ForceVisible();
        m_pComponent->m_EffectController.Tick(ezTime::MakeFromSeconds(0.05));

        if (m_pComponent->m_EffectController.IsAlive())
        {
          m_pComponent->m_EffectController.GetBoundingVolume(m_ThumbnailBoundingVolume);

          // shrink the bbox to zoom in
          m_ThumbnailBoundingVolume.m_fSphereRadius *= 0.7f;
          m_ThumbnailBoundingVolume.m_vBoxHalfExtents *= 0.7f;
        }
      }

      m_pComponent->m_uiRandomSeed = 0;

      // tick the world once more, so that the effect passes on its bounding box to the culling system
      // otherwise the effect is not rendered, when only the thumbnail is updated, but the document is not open
      m_pWorld->SetWorldSimulationEnabled(true);
      m_pWorld->GetClock().SetPaused(true);
      m_pWorld->Update();
      m_pWorld->GetClock().SetPaused(bWorldPaused);
      m_pWorld->SetWorldSimulationEnabled(false);
    }
  }

  pParticleViewContext->PositionThumbnailCamera(m_ThumbnailBoundingVolume);
  return true;
}

void ezParticleContext::RestartEffect()
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  if (m_pComponent)
  {
    m_pComponent->InterruptEffect();
    m_pComponent->StartEffect();
  }
}

void ezParticleContext::SetAutoRestartEffect(bool loop)
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  if (m_pComponent)
  {
    m_pComponent->m_OnFinishedAction = loop ? ezOnComponentFinishedAction2::Restart : ezOnComponentFinishedAction2::None;
  }
}
