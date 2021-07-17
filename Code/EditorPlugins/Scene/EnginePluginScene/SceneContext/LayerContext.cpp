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
  m_Context.m_uiNextComponentPickingID = m_pParentSceneContext->m_Context.m_uiNextComponentPickingID;
  ezEngineProcessDocumentContext::HandleMessage(pMsg);
  m_pParentSceneContext->m_Context.m_uiNextComponentPickingID = m_Context.m_uiNextComponentPickingID;

  //#TODO: these are not sent as the layer is not connected to a window that triggers these so rerouting does not work.
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezObjectSelectionMsgToEngine>())
  {
    m_pParentSceneContext->HandleMessage(pMsg);
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezQuerySelectionBBoxMsgToEngine>())
  {
    m_pParentSceneContext->HandleMessage(pMsg);
    return;
  }
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
