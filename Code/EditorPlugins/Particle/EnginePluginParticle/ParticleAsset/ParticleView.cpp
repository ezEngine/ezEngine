#include <EnginePluginParticlePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EnginePluginParticle/ParticleAsset/ParticleContext.h>
#include <EnginePluginParticle/ParticleAsset/ParticleView.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezParticleViewContext::ezParticleViewContext(ezParticleContext* pParticleContext)
    : ezEngineProcessViewContext(pParticleContext)
{
  m_pParticleContext = pParticleContext;
}

ezParticleViewContext::~ezParticleViewContext() {}

void ezParticleViewContext::PositionThumbnailCamera()
{
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(-1.8f, 1.8f, 1.0f), ezVec3::ZeroVector(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezViewHandle ezParticleViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("Particle Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}
