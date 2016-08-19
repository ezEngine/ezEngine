#include <PCH.h>
#include <EnginePluginParticle/ParticleAsset/ParticleContext.h>
#include <EnginePluginParticle/ParticleAsset/ParticleView.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <CoreUtils/Geometry/GeomUtils.h>
#include <GameFoundation/GameApplication/GameApplication.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <SharedPluginAssets/Common/Messages.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleContext, 1, ezRTTIDefaultAllocator<ezParticleContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Particle Effect Asset"),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE


ezParticleContext::ezParticleContext()
{
  m_pComponent = nullptr;
}

void ezParticleContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
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
    }
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineRestoreResourceMsg>())
  {
    ezResourceManager::ReloadResource(m_hParticle, true);
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}

void ezParticleContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  EZ_LOCK(pWorld->GetWriteMarker());

  ezParticleComponentManager* pCompMan = pWorld->GetOrCreateComponentManager<ezParticleComponentManager>();

  ezGameObjectDesc obj;
  ezGameObject* pObj;

  // Preview Effect
  {
    obj.m_sName.Assign("ParticlePreview");
    pWorld->CreateObject(obj, pObj);

    pCompMan->CreateComponent(m_pComponent);

    ezString sParticleGuid = ezConversionUtils::ToString(GetDocumentGuid());
    m_hParticle = ezResourceManager::LoadResource<ezParticleEffectResource>(sParticleGuid);

    m_pComponent->SetParticleEffect(m_hParticle);

    pObj->AttachComponent(m_pComponent);
  }
}

void ezParticleContext::OnDeinitialize()
{
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

  if (m_pComponent && m_pComponent->m_pParticleEffect)
  {
    for (ezUInt32 i = 0; i < 15; ++i)
    {
      m_pComponent->m_pParticleEffect->Update(ezTime::Seconds(0.1));
    }
  }

  return true;
}
