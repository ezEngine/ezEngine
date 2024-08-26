#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/SceneContext/LayerContext.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>


// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLayerContext, 1, ezRTTIDefaultAllocator<ezLayerContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Layer"),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_FUNCTION_PROPERTY(AllocateContext),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezEngineProcessDocumentContext* ezLayerContext::AllocateContext(const ezDocumentOpenMsgToEngine* pMsg)
{
  if (pMsg->m_DocumentMetaData.IsA<ezUuid>())
  {
    return ezGetStaticRTTI<ezLayerContext>()->GetAllocator()->Allocate<ezEngineProcessDocumentContext>();
  }
  else
  {
    return ezGetStaticRTTI<ezSceneContext>()->GetAllocator()->Allocate<ezEngineProcessDocumentContext>();
  }
}

ezLayerContext::ezLayerContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::None)
{
}

ezLayerContext::~ezLayerContext() = default;

void ezLayerContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  // Everything in the picking buffer needs a unique ID. As layers and scene share the same world we need to make sure no id is used twice.
  // To achieve this the scene's next ID is retrieved on every change and written back in base new IDs were used up.
  m_Context.m_uiNextComponentPickingID = m_pParentSceneContext->m_Context.m_uiNextComponentPickingID;
  ezEngineProcessDocumentContext::HandleMessage(pMsg);
  m_pParentSceneContext->m_Context.m_uiNextComponentPickingID = m_Context.m_uiNextComponentPickingID;

  if (pMsg->IsInstanceOf<ezEntityMsgToEngine>())
  {
    EZ_LOCK(m_pWorld->GetWriteMarker());
    m_pParentSceneContext->AddLayerIndexTag(*static_cast<const ezEntityMsgToEngine*>(pMsg), m_Context, m_LayerTag);
  }
}

void ezLayerContext::SceneDeinitialized()
{
  // If the scene is deinitialized the world is destroyed so there is no use tracking anything further.
  m_pWorld = nullptr;
  m_Context.Clear();
}

const ezTag& ezLayerContext::GetLayerTag() const
{
  return m_LayerTag;
}

void ezLayerContext::OnInitialize()
{
  ezUuid parentScene = m_MetaData.Get<ezUuid>();
  ezEngineProcessDocumentContext* pContext = GetDocumentContext(parentScene);
  m_pParentSceneContext = ezDynamicCast<ezSceneContext*>(pContext);

  m_pWorld = m_pParentSceneContext->GetWorld();
  m_Context.m_pWorld = m_pWorld;
  m_Mirror.InitReceiver(&m_Context);

  ezUInt32 uiLayerID = m_pParentSceneContext->RegisterLayer(this);
  ezStringBuilder sVisibilityTag;
  sVisibilityTag.SetFormat("Layer_{}", uiLayerID);
  m_LayerTag = ezTagRegistry::GetGlobalRegistry().RegisterTag(sVisibilityTag);

  ezShadowPool::AddExcludeTagToWhiteList(m_LayerTag);
}

void ezLayerContext::OnDeinitialize()
{
  if (m_pWorld)
  {
    // If the world still exists we are just unloading the layer not the scene that owns the world.
    // Thus, we need to make sure the layer objects are removed from the still existing world.
    m_Context.DeleteExistingObjects();
  }

  m_LayerTag = ezTag();
  m_pParentSceneContext->UnregisterLayer(this);
  m_pParentSceneContext = nullptr;
}

ezEngineProcessViewContext* ezLayerContext::CreateViewContext()
{
  EZ_REPORT_FAILURE("Layers should not create views.");
  return nullptr;
}

void ezLayerContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_REPORT_FAILURE("Layers should not create views.");
}

ezStatus ezLayerContext::ExportDocument(const ezExportDocumentMsgToEngine* pMsg)
{
  EZ_REPORT_FAILURE("Layers do not support export yet. THe layer content is baked into the main scene instead.");
  return ezStatus("Nope");
}

void ezLayerContext::UpdateDocumentContext()
{
  SUPER::UpdateDocumentContext();
}
