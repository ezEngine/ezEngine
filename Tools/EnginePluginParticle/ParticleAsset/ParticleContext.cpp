#include <PCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EnginePluginParticle/ParticleAsset/ParticleContext.h>
#include <EnginePluginParticle/ParticleAsset/ParticleView.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <SharedPluginAssets/Common/Messages.h>
#include <RendererCore/Material/MaterialResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleContext, 1, ezRTTIDefaultAllocator<ezParticleContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Particle Effect Asset"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleContext::ezParticleContext()
{
  m_pComponent = nullptr;
}

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

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineResourceUpdateMsg>())
  {
    const ezEditorEngineResourceUpdateMsg* pMsg2 = static_cast<const ezEditorEngineResourceUpdateMsg*>(pMsg);

    if (pMsg2->m_sResourceType == "Particle")
    {
      ezUniquePtr<ezResourceLoaderFromMemory> loader(EZ_DEFAULT_NEW(ezResourceLoaderFromMemory));
      loader->m_ModificationTimestamp = ezTimestamp::CurrentTimestamp();
      loader->m_sResourceDescription = "ParticleImmediateEditorUpdate";
      ezMemoryStreamWriter memoryWriter(&loader->m_CustomData);
      memoryWriter.WriteBytes(pMsg2->m_Data.GetData(), pMsg2->m_Data.GetCount());

      ezResourceManager::UpdateResourceWithCustomLoader(m_hParticle, std::move(loader));

      // force loading of the resource (not needed here, works well without it)
      // ezResourceLock<ezParticleEffectResource> pResource(m_hParticle, ezResourceAcquireMode::NoFallback);
    }
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineRestoreResourceMsg>())
  {
    ezResourceManager::RestoreResource(m_hParticle);
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
    m_pComponent->m_MinRestartDelay = ezTime::Seconds(0.5);

    ezStringBuilder sParticleGuid;
    ezConversionUtils::ToString(GetDocumentGuid(), sParticleGuid);
    m_hParticle = ezResourceManager::LoadResource<ezParticleEffectResource>(sParticleGuid);

    m_pComponent->SetParticleEffect(m_hParticle);
  }

  // Lights
  {
    ezGameObjectDesc obj;
    obj.m_sName.Assign("DirLight");
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0.0f, 1.0f, 0.0f), ezAngle::Degree(120.0f));

    ezGameObject* pObj;
    pWorld->CreateObject(obj, pObj);

    ezDirectionalLightComponent* pDirLight;
    ezDirectionalLightComponent::CreateComponent(pObj, pDirLight);
    pDirLight->SetCastShadows(true);

    ezAmbientLightComponent* pAmbLight;
    ezAmbientLightComponent::CreateComponent(pObj, pAmbLight);
    pAmbLight->SetTopColor(ezColor(0.2f, 0.2f, 0.2f));
    pAmbLight->SetBottomColor(ezColor(0.1f, 0.1f, 0.1f));
    pAmbLight->SetIntensity(5.0f);
  }

  const char* szMeshName = "ParticlePreviewBackgroundMesh";
  ezMeshResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshName);

  if (!hMesh.IsValid())
  {
    const char* szMeshBufferName = "ParticlePreviewBackgroundMeshBuffer";

    ezMeshBufferResourceHandle hMeshBuffer;
    {
      // Build geometry
      ezGeometry geom;

      geom.AddTexturedBox(ezVec3(1, 4, 4), ezColor::White, ezMat4::IdentityMatrix());
      geom.ComputeTangents();

      ezMeshBufferResourceDescriptor desc;
      desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::XYFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::TexCoord1, ezGALResourceFormat::XYFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::XYZFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::Color, ezGALResourceFormat::RGBAUByteNormalized);
      desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

      hMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>(szMeshBufferName, desc, szMeshBufferName);
    }
    {
      ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer);

      ezMeshResourceDescriptor md;
      md.UseExistingMeshBuffer(hMeshBuffer);
      md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
      md.SetMaterial(0, "{ 6bd5e7e6-b7be-9801-e032-14226cba1e96 }"); 
      md.ComputeBounds();

      hMesh = ezResourceManager::CreateResource<ezMeshResource>(szMeshName, md, pMeshBuffer->GetResourceDescription());
    }
  }

  // Background Mesh
  {
    ezMaterialResourceHandle hBgTop = ezResourceManager::LoadResource<ezMaterialResource>("{ aa1c5601-bc43-fbf8-4e07-6a3df3af51e7 }");//{ 6bd5e7e6-b7be-9801-e032-14226cba1e96 }"); // PrototypeGrey.ezMaterialAssetkd
    ezMaterialResourceHandle hBgBottom =
        ezResourceManager::LoadResource<ezMaterialResource>("{ 6bd5e7e6-b7be-9801-e032-14226cba1e96 }"); // PrototypeBlack.ezMaterialAssetkd

    ezGameObjectDesc obj;
    obj.m_sName.Assign("ParticleBackground");

    for (int y = 0; y <= 5; ++y)
    {
      for (int x = -5; x <= 5; ++x)
      {
        ezGameObject* pObj;
        obj.m_LocalPosition.Set(5, (float)x * 4, 2 + (float)y * 4);
        pWorld->CreateObject(obj, pObj);

        ezMeshComponent* pMesh;
        ezMeshComponent::CreateComponent(pObj, pMesh);
        pMesh->SetMesh(hMesh);

        pMesh->SetMaterial(0, hBgTop);
      }
    }

    for (int y = -5; y < 0; ++y)
    {
      for (int x = -5; x <= 5; ++x)
      {
        ezGameObject* pObj;
        obj.m_LocalPosition.Set(5, (float)x * 4, 2 + (float)y * 4);
        pWorld->CreateObject(obj, pObj);

        ezMeshComponent* pMesh;
        ezMeshComponent::CreateComponent(pObj, pMesh);
        pMesh->SetMesh(hMesh);

        pMesh->SetMaterial(0, hBgBottom);
      }
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

bool ezParticleContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  ezParticleViewContext* pParticleViewContext = static_cast<ezParticleViewContext*>(pThumbnailViewContext);
  pParticleViewContext->PositionThumbnailCamera();

  EZ_LOCK(m_pWorld->GetWriteMarker());
  m_pWorld->SetWorldSimulationEnabled(true);
  m_pWorld->Update();
  m_pWorld->SetWorldSimulationEnabled(false);

  if (m_pComponent && !m_pComponent->m_EffectController.IsAlive())
  {
    for (ezUInt32 i = 0; i < 15; ++i)
    {
      m_pComponent->m_EffectController.Tick(ezTime::Seconds(0.1));
    }
  }

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
