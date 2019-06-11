#include <RecastPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Time/Stopwatch.h>
#include <Recast/Recast.h>
#include <RecastPlugin/Components/RecastNavMeshComponent.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

ezCVarBool g_AiShowNavMesh("ai_ShowNavMesh", false, ezCVarFlags::Default, "Draws the navmesh, if one is available");

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezRcComponent, 1)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI"),
  }
  EZ_END_ATTRIBUTES;
}

EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezRcComponent::ezRcComponent() {}
ezRcComponent::~ezRcComponent() {}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRcNavMeshComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezLongOpAttribute("ezLongOpProxy_BuildNavMesh"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ShowNavMesh", m_bShowNavMesh),
    EZ_MEMBER_PROPERTY("NavMeshConfig", m_NavMeshConfig),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_FUNCTION_PROPERTY(OnObjectCreated),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezRcNavMeshComponent::ezRcNavMeshComponent() {}
ezRcNavMeshComponent::~ezRcNavMeshComponent() {}

void ezRcNavMeshComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_bShowNavMesh;
  s << m_hNavMesh;
}

void ezRcNavMeshComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_bShowNavMesh;

  if (uiVersion >= 2)
  {
    s >> m_hNavMesh;
  }
}

void ezRcNavMeshComponent::OnObjectCreated(const ezAbstractObjectNode& node)
{
  ezStringBuilder sComponentGuid, sNavMeshFile;
  ezConversionUtils::ToString(node.GetGuid(), sComponentGuid);

  // this is where the editor will put the file for this component
  sNavMeshFile.Format(":project/AssetCache/Generated/{0}.ezRecastNavMesh", sComponentGuid);

  m_hNavMesh = ezResourceManager::LoadResource<ezRecastNavMeshResource>(sNavMeshFile);
}

void ezRcNavMeshComponent::Update()
{
  VisualizeNavMesh();
  VisualizePointsOfInterest();
}

EZ_ALWAYS_INLINE static ezVec3 GetNavMeshVertex(
  const rcPolyMesh* pMesh, ezUInt16 uiVertex, const ezVec3& vMeshOrigin, float fCellSize, float fCellHeight)
{
  const ezUInt16* v = &pMesh->verts[uiVertex * 3];
  const float x = vMeshOrigin.x + v[0] * fCellSize;
  const float y = vMeshOrigin.y + v[2] * fCellSize;
  const float z = vMeshOrigin.z + v[1] * fCellHeight;

  return ezVec3(x, y, z);
}

void ezRcNavMeshComponent::OnActivated()
{
  if (m_hNavMesh.IsValid())
  {
    GetWorld()->GetOrCreateModule<ezRecastWorldModule>()->SetNavMeshResource(m_hNavMesh);
  }
}

void ezRcNavMeshComponent::VisualizeNavMesh()
{
  if (!m_bShowNavMesh && !g_AiShowNavMesh)
    return;

  auto hNavMesh = GetWorld()->GetOrCreateModule<ezRecastWorldModule>()->GetNavMeshResource();
  if (!hNavMesh.IsValid())
    return;

  ezResourceLock<ezRecastNavMeshResource> pNavMesh(hNavMesh, ezResourceAcquireMode::NoFallbackAllowMissing);
  if (pNavMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  const auto* pMesh = pNavMesh->GetNavMeshPolygons();

  if (pMesh == nullptr)
    return;

  ezDynamicArray<ezDebugRenderer::Triangle> triangles;
  triangles.Reserve(pMesh->npolys * 3);

  ezDynamicArray<ezDebugRenderer::Line> contourLines;
  contourLines.Reserve(pMesh->npolys * 2);
  ezDynamicArray<ezDebugRenderer::Line> innerLines;
  innerLines.Reserve(pMesh->npolys * 3);

  const ezInt32 iMaxNumVertInPoly = pMesh->nvp;
  const float fCellSize = pMesh->cs;
  const float fCellHeight = pMesh->ch;
  // add a little height offset to move the visualization up a little
  const ezVec3 vMeshOrigin(pMesh->bmin[0], pMesh->bmin[2], pMesh->bmin[1] + fCellHeight * 0.3f);

  for (ezInt32 i = 0; i < pMesh->npolys; ++i)
  {
    const ezUInt16* polyVtxIndices = &pMesh->polys[i * (iMaxNumVertInPoly * 2)];
    const ezUInt16* neighborData = &pMesh->polys[i * (iMaxNumVertInPoly * 2) + iMaxNumVertInPoly];

    // const ezUInt8 areaType = pMesh->areas[i];
    // if (areaType == RC_WALKABLE_AREA)
    //  color = duRGBA(0, 192, 255, 64);
    // else if (areaType == RC_NULL_AREA)
    //  color = duRGBA(0, 0, 0, 64);
    // else
    //  color = dd->areaToCol(area);

    ezInt32 j;
    for (j = 1; j < iMaxNumVertInPoly; ++j)
    {
      if (polyVtxIndices[j] == RC_MESH_NULL_IDX)
        break;

      const bool bIsContour = neighborData[j - 1] == 0xffff;

      {
        auto& line = bIsContour ? contourLines.ExpandAndGetRef() : innerLines.ExpandAndGetRef();
        line.m_start = GetNavMeshVertex(pMesh, polyVtxIndices[j - 1], vMeshOrigin, fCellSize, fCellHeight);
        line.m_end = GetNavMeshVertex(pMesh, polyVtxIndices[j], vMeshOrigin, fCellSize, fCellHeight);
      }
    }

    // close the loop
    const bool bIsContour = neighborData[j - 1] == 0xffff;
    {
      auto& line = bIsContour ? contourLines.ExpandAndGetRef() : innerLines.ExpandAndGetRef();
      line.m_start = GetNavMeshVertex(pMesh, polyVtxIndices[j - 1], vMeshOrigin, fCellSize, fCellHeight);
      line.m_end = GetNavMeshVertex(pMesh, polyVtxIndices[0], vMeshOrigin, fCellSize, fCellHeight);
    }

    for (j = 2; j < iMaxNumVertInPoly; ++j)
    {
      if (polyVtxIndices[j] == RC_MESH_NULL_IDX)
        break;

      auto& triangle = triangles.ExpandAndGetRef();

      triangle.m_p0 = GetNavMeshVertex(pMesh, polyVtxIndices[0], vMeshOrigin, fCellSize, fCellHeight);
      triangle.m_p2 = GetNavMeshVertex(pMesh, polyVtxIndices[j - 1], vMeshOrigin, fCellSize, fCellHeight);
      triangle.m_p1 = GetNavMeshVertex(pMesh, polyVtxIndices[j], vMeshOrigin, fCellSize, fCellHeight);
    }
  }

  ezDebugRenderer::DrawSolidTriangles(GetWorld(), triangles, ezColor::CadetBlue.WithAlpha(0.25f));
  ezDebugRenderer::DrawLines(GetWorld(), contourLines, ezColor::DarkOrange);
  ezDebugRenderer::DrawLines(GetWorld(), innerLines, ezColor::CadetBlue);
}

void ezRcNavMeshComponent::VisualizePointsOfInterest()
{
  if (!m_bShowNavMesh && !g_AiShowNavMesh)
    return;

  auto pPoiGraph = GetWorld()->GetOrCreateModule<ezRecastWorldModule>()->GetNavMeshPointsOfInterestGraph();

  if (pPoiGraph == nullptr)
    return;

  const auto& poi = *pPoiGraph;
  const auto& graph = poi.GetGraph();

  const ezUInt32 uiCheckTimeStamp = poi.GetCheckVisibilityTimeStamp();
  const ezUInt32 uiConsiderInvisibleTimeStamp = uiCheckTimeStamp - 20;

  ezDynamicArray<ezDebugRenderer::Line> visibleLines;
  ezDynamicArray<ezDebugRenderer::Line> hiddenLines;

  for (const auto& point : graph.GetPoints())
  {
    if (point.m_uiVisibleMarker < uiConsiderInvisibleTimeStamp || (point.m_uiVisibleMarker & 3U) == 0)
    {
      // not updated for too long -> consider invisible

      auto& line = hiddenLines.ExpandAndGetRef();

      line.m_start = point.m_vFloorPosition;
      line.m_end = point.m_vFloorPosition + ezVec3(0, 0, 1.8f);
      continue;
    }

    if ((point.m_uiVisibleMarker & 3U) == 3U) // fully visible
    {
      auto& line = visibleLines.ExpandAndGetRef();

      line.m_start = point.m_vFloorPosition;
      line.m_end = point.m_vFloorPosition + ezVec3(0, 0, 1.8f);
      continue;
    }

    // else bottom half invisible
    {
      auto& line = hiddenLines.ExpandAndGetRef();
      line.m_start = point.m_vFloorPosition;
      line.m_end = point.m_vFloorPosition + ezVec3(0, 0, 1.0f);
    }

    // top half visible
    {
      auto& line = visibleLines.ExpandAndGetRef();
      line.m_start = point.m_vFloorPosition + ezVec3(0, 0, 1.0f);
      line.m_end = point.m_vFloorPosition + ezVec3(0, 0, 1.8f);
    }
  }

  ezDebugRenderer::DrawLines(GetWorld(), visibleLines, ezColor::DeepSkyBlue);
  ezDebugRenderer::DrawLines(GetWorld(), hiddenLines, ezColor::SlateGrey);
}

//////////////////////////////////////////////////////////////////////////

ezRcNavMeshComponentManager::ezRcNavMeshComponentManager(ezWorld* pWorld)
  : SUPER(pWorld)
  , m_pWorldModule(nullptr)
{
}

ezRcNavMeshComponentManager::~ezRcNavMeshComponentManager() = default;

void ezRcNavMeshComponentManager::Initialize()
{
  SUPER::Initialize();

  // make sure this world module exists
  m_pWorldModule = GetWorld()->GetOrCreateModule<ezRecastWorldModule>();

  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezRcNavMeshComponentManager::Update, this);
  desc.m_bOnlyUpdateWhenSimulating = false;

  RegisterUpdateFunction(desc);
}

void ezRcNavMeshComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActive())
      it->Update();
  }
}
