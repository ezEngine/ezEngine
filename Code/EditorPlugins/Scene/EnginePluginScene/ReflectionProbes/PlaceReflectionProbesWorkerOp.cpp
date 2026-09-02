#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/ReflectionProbes/PlaceReflectionProbesWorkerOp.h>

#ifdef BUILDSYSTEM_ENABLE_RECAST_SUPPORT

#  include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#  include <Foundation/Utilities/Progress.h>
#  include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpWorker_PlaceReflectionProbes, 1, ezRTTIDefaultAllocator<ezLongOpWorker_PlaceReflectionProbes>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezLongOpWorker_PlaceReflectionProbes::InitializeExecution(ezStreamReader& ref_config, const ezUuid& documentGuid)
{
  ezEngineProcessDocumentContext* pDocContext = ezEngineProcessDocumentContext::GetDocumentContext(documentGuid);

  if (pDocContext == nullptr)
    return EZ_FAILURE;

  m_Settings.Deserialize(ref_config);

  if (!m_Settings.m_Bounds.IsValid())
    return EZ_FAILURE;

  // Geometry extraction sends messages to components, so it has to happen here on the main thread,
  // not in Execute(), which runs on a worker thread.
  ezWorldGeoExtractionUtil::MeshObjectList objects;
  {
    ezWorld* pWorld = pDocContext->GetWorld();
    EZ_LOCK(pWorld->GetWriteMarker());

    ezTagSet excludeTags;
    excludeTags.SetByName("Editor");

    ezWorldGeoExtractionUtil::ExtractWorldGeometry(objects, *pWorld, ezWorldGeoExtractionUtil::ExtractionMode::CollisionMesh, &excludeTags);
  }

  return m_Placer.BuildInputGeometry(objects, m_Settings);
}

ezResult ezLongOpWorker_PlaceReflectionProbes::Execute(ezProgress& ref_progress, ezStreamWriter& ref_proxydata)
{
  EZ_SUCCEED_OR_RETURN(m_Placer.Compute(m_Settings, ref_progress));

  const auto results = m_Placer.GetResults();

  ref_proxydata << results.GetCount();

  for (const auto& result : results)
  {
    ref_proxydata << result.m_vPosition;
    ref_proxydata << result.m_vExtents;
    ref_proxydata << result.m_bIsBox;
  }

  return EZ_SUCCESS;
}

#endif
