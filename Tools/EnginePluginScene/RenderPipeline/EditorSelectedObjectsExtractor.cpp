#include <PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <EnginePluginScene/RenderPipeline/EditorSelectedObjectsExtractor.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorSelectedObjectsExtractor, 1, ezRTTIDefaultAllocator<ezEditorSelectedObjectsExtractor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("SceneContext", GetSceneContext, SetSceneContext),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE


ezEditorSelectedObjectsExtractor::ezEditorSelectedObjectsExtractor()
{
  m_pSceneContext = nullptr;
}

const ezDeque<ezGameObjectHandle>* ezEditorSelectedObjectsExtractor::GetSelection()
{
  if (m_pSceneContext == nullptr)
    return nullptr;

  return &m_pSceneContext->GetSelectionWithChildren();
}
