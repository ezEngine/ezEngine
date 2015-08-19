#include <PCH.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <EnginePluginScene/SceneView/SceneView.h>

#include <RendererCore/Meshes/MeshComponent.h>
#include <GameUtils/Components/RotorComponent.h>
#include <GameUtils/Components/SliderComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneContext, ezEngineProcessDocumentContext, 1, ezRTTIDefaultAllocator<ezSceneContext>);
EZ_BEGIN_PROPERTIES
EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "ezScene")
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();


void ezSceneContext::OnInitialize()
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  /// \todo Plugin concept to allow custom initialization
  m_pWorld->CreateComponentManager<ezMeshComponentManager>();
  m_pWorld->CreateComponentManager<ezRotorComponentManager>();
  m_pWorld->CreateComponentManager<ezSliderComponentManager>();
}

ezEngineProcessViewContext* ezSceneContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezViewContext, this);
}

void ezSceneContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}
