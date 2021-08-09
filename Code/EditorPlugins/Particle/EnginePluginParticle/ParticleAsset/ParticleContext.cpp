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

ezParticleContext::ezParticleContext() = default;
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
  auto pWorld = m_pWorld.Borrow();
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
    m_pComponent->m_MinRestartDelay = ezTime::Seconds(0.5);

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

      geom.AddTexturedBox(ezVec3(4, 4, 4), ezColor::White, ezMat4::IdentityMatrix());
      geom.ComputeTangents();

      ezMeshBufferResourceDescriptor desc;
      desc.AddCommonStreams();
      desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

      hMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
    }
    {
      ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer, ezResourceAcquireMode::AllowLoadingFallback);

      ezMeshResourceDescriptor md;
      md.UseExistingMeshBuffer(hMeshBuffer);
      md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
      md.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }"); // Pattern.ezMaterialAsset
      md.ComputeBounds();

      m_hPreviewMeshResource = ezResourceManager::CreateResource<ezMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
    }
  }

  ezPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>();

  // Background Mesh
  {
    ezGameObjectDesc obj;
    obj.m_sName.Assign("ParticleBackground");

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

        if (pPhysicsInterface)
          pPhysicsInterface->AddStaticCollisionBox(pObj, ezVec3(4, 4, 4));
      }
    }

    for (int x = -5; x <= 5; ++x)
    {
      ezGameObject* pObj;
      obj.m_LocalPosition.Set(4, (float)x * 4, -2);
      obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(45));
      pWorld->CreateObject(obj, pObj);

      ezMeshComponent* pMesh;
      ezMeshComponent::CreateComponent(pObj, pMesh);
      pMesh->SetMesh(m_hPreviewMeshResource);

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
  m_ThumbnailBoundingVolume.SetInvalid();
}

bool ezParticleContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  ezParticleViewContext* pParticleViewContext = static_cast<ezParticleViewContext*>(pThumbnailViewContext);

  if (!m_ThumbnailBoundingVolume.IsValid())
  {
    EZ_LOCK(m_pWorld->GetWriteMarker());
    m_pWorld->SetWorldSimulationEnabled(true);
    m_pWorld->Update();
    m_pWorld->SetWorldSimulationEnabled(false);

    if (m_pComponent && m_pComponent->m_EffectController.IsAlive())
    {
      m_pComponent->InterruptEffect();
      m_pComponent->StartEffect();

      // set a fixed random seed
      m_pComponent->m_uiRandomSeed = 11;

      float fLastVolume = 0;

      ezUInt32 uiSimStepsNeeded = 3;
      ezInt32 iBVolState = 0;

      for (ezUInt32 step = 0; step < 30; ++step)
      {
        m_pComponent->m_EffectController.Tick(ezTime::Seconds(0.05));

        if (!m_pComponent->m_EffectController.IsAlive())
        {
          break;
        }

        ezBoundingBoxSphere bvol;
        m_pComponent->m_EffectController.GetBoundingVolume(bvol);
        const ezVec3 ext = bvol.GetBox().GetExtents();
        const float volume = ext.x * ext.y * ext.z;

        if (iBVolState == 0) // initialize
        {
          fLastVolume = volume;
          iBVolState = 1;
        }
        else if (iBVolState == 1) // wait for grow
        {
          if (volume > fLastVolume)
          {
            fLastVolume = volume;
            uiSimStepsNeeded = step;
            iBVolState = 2;
          }
        }
        else if (iBVolState == 2) // wait for shrink
        {
          if (volume < fLastVolume)
          {
            break;
          }

          uiSimStepsNeeded = step;
          fLastVolume = volume;
        }
      }

      if (uiSimStepsNeeded > 3)
      {
        uiSimStepsNeeded -= 2;
      }

      m_pComponent->InterruptEffect();
      m_pComponent->StartEffect();

      for (ezUInt32 step = 0; step <= uiSimStepsNeeded; ++step)
      {
        m_pComponent->m_EffectController.Tick(ezTime::Seconds(0.05));

        if (m_pComponent->m_EffectController.IsAlive())
        {
          m_pComponent->m_EffectController.GetBoundingVolume(m_ThumbnailBoundingVolume);
        }
      }

      m_pComponent->m_uiRandomSeed = 0;
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
