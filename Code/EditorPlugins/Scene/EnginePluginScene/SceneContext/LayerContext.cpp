#include <EnginePluginScenePCH.h>

#include <EnginePluginScene/SceneContext/LayerContext.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>


// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLayerContext, 1, ezRTTIDefaultAllocator<ezLayerContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Layer"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezLayerContext::ezLayerContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::None)
{
}

ezLayerContext::~ezLayerContext()
{
}

void ezLayerContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  // Everything in the picking buffer needs a unique ID. As layers and scene share the same world we need to make sure no id is used twice.
  // To achieve this the scene's next ID is retrieved on every change and written back in base new IDs were used up.
  m_Context.m_uiNextComponentPickingID = m_pParentSceneContext->m_Context.m_uiNextComponentPickingID;
  ezEngineProcessDocumentContext::HandleMessage(pMsg);
  m_pParentSceneContext->m_Context.m_uiNextComponentPickingID = m_Context.m_uiNextComponentPickingID;
}

void ezLayerContext::SceneDeinitialized()
{
  // If the scene is deinitialized the world is destroyed so there is no use tracking anything further.
  m_pWorld = nullptr;
  m_Context.Clear();
}

void ezLayerContext::OnInitialize()
{
  ezUuid parentScene = m_MetaData.Get<ezUuid>();
  ezEngineProcessDocumentContext* pContext = GetDocumentContext(parentScene);
  m_pParentSceneContext = ezDynamicCast<ezSceneContext*>(pContext);

  m_pWorld = m_pParentSceneContext->GetWorld();
  m_Context.m_pWorld = m_pWorld;
  m_Mirror.InitReceiver(&m_Context);

  m_pParentSceneContext->RegisterLayer(this);
}

void ezLayerContext::OnDeinitialize()
{
  if (m_pWorld)
  {
    // If the world still exists we are just unloading the layer not the scene that owns the world.
    // Thus, we need to make sure the layer objects are removed from the still existing world.
    m_Context.DeleteExistingObjects();
  }

  m_pParentSceneContext->UnregisterLayer(this);
  m_pParentSceneContext = nullptr;
}

ezEngineProcessViewContext* ezLayerContext::CreateViewContext()
{
  EZ_REPORT_FAILURE("TODO");
  return nullptr;
}

void ezLayerContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_REPORT_FAILURE("TODO");
}

bool ezLayerContext::ExportDocument(const ezExportDocumentMsgToEngine* pMsg)
{
  EZ_REPORT_FAILURE("TODO");
  return false;
}

void ezLayerContext::UpdateDocumentContext()
{
  SUPER::UpdateDocumentContext();
 
}
