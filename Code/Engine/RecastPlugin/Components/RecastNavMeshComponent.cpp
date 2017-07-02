#include <PCH.h>
#include <RecastPlugin/Components/RecastNavMeshComponent.h>
#include <GameEngine/AI/NavMesh/NavMeshDescription.h>
#include <GameEngine/Messages/BuildNavMeshMessage.h>
#include <Foundation/Time/Stopwatch.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <ThirdParty/Recast/Recast.h>
#include <Foundation/Configuration/CVar.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

ezCVarBool g_AiShowNavMesh("ai_ShowNavMesh", false, ezCVarFlags::Default, "Draws the navmesh, if one is available");

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezRcComponent, 1)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI"),
  }
  EZ_END_ATTRIBUTES
}

EZ_END_ABSTRACT_COMPONENT_TYPE

ezRcComponent::ezRcComponent() { }
ezRcComponent::~ezRcComponent() { }

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezRcNavMeshComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ShowNavMesh", m_bShowNavMesh),
    EZ_MEMBER_PROPERTY("NavMeshConfig", m_NavMeshConfig),
  }
  EZ_END_PROPERTIES
}
EZ_END_COMPONENT_TYPE

ezRcNavMeshComponent::ezRcNavMeshComponent() { }
ezRcNavMeshComponent::~ezRcNavMeshComponent() { }

void ezRcNavMeshComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_bShowNavMesh;
}

void ezRcNavMeshComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_bShowNavMesh;
}

void ezRcNavMeshComponent::OnSimulationStarted()
{
  m_uiDelay = 2;
}

void ezRcNavMeshComponent::Update()
{
  if (!IsActiveAndSimulating())
    return;

  VisualizeNavMesh();
  VisualizePointsOfInterest();

  --m_uiDelay;

  if (m_uiDelay > 0)
    return;

  if (m_NavMeshBuilder.Build(m_NavMeshConfig, *GetWorld()).Failed())
    return;

  // empty navmesh
  if (m_NavMeshBuilder.m_pNavMesh == nullptr)
    return;

  GetManager()->GetRecastWorldModule()->SetNavMesh(m_NavMeshBuilder.m_pNavMesh);
  GetManager()->GetRecastWorldModule()->m_NavMeshPointsOfInterest.ExtractInterestPointsFromMesh(*m_NavMeshBuilder.m_polyMesh, true);
}

EZ_ALWAYS_INLINE static ezVec3 GetNavMeshVertex(const rcPolyMesh* pMesh, ezUInt16 uiVertex, const ezVec3& vMeshOrigin, float fCellSize, float fCellHeight)
{
  const ezUInt16* v = &pMesh->verts[uiVertex * 3];
  const float x = vMeshOrigin.x + v[0] * fCellSize;
  const float y = vMeshOrigin.y + v[2] * fCellSize;
  const float z = vMeshOrigin.z + v[1] * fCellHeight;

  return ezVec3(x, y, z);
}

void ezRcNavMeshComponent::VisualizeNavMesh()
{
  if ((m_bShowNavMesh || g_AiShowNavMesh) && (m_NavMeshBuilder.m_polyMesh != nullptr))
  {
    const auto& mesh = *m_NavMeshBuilder.m_polyMesh;

    ezDynamicArray<ezDebugRenderer::Triangle> triangles;
    triangles.Reserve(mesh.npolys * 3);

    ezDynamicArray<ezDebugRenderer::Line> contourLines;
    contourLines.Reserve(mesh.npolys * 2);
    ezDynamicArray<ezDebugRenderer::Line> innerLines;
    innerLines.Reserve(mesh.npolys * 3);

    const ezInt32 iMaxNumVertInPoly = m_NavMeshBuilder.m_polyMesh->nvp;
    const float fCellSize = m_NavMeshBuilder.m_polyMesh->cs;
    const float fCellHeight = m_NavMeshBuilder.m_polyMesh->ch;
    // add a little height offset to move the visualization up a little
    const ezVec3 vMeshOrigin(m_NavMeshBuilder.m_polyMesh->bmin[0], m_NavMeshBuilder.m_polyMesh->bmin[2], m_NavMeshBuilder.m_polyMesh->bmin[1] + fCellHeight * 0.3f);

    for (ezInt32 i = 0; i < mesh.npolys; ++i)
    {
      const ezUInt16* polyVtxIndices = &mesh.polys[i * (iMaxNumVertInPoly * 2)];
      const ezUInt16* neighborData = &mesh.polys[i * (iMaxNumVertInPoly * 2) + iMaxNumVertInPoly];

      //const ezUInt8 areaType = mesh.areas[i];
      //if (areaType == RC_WALKABLE_AREA)
      //  color = duRGBA(0, 192, 255, 64);
      //else if (areaType == RC_NULL_AREA)
      //  color = duRGBA(0, 0, 0, 64);
      //else
      //  color = dd->areaToCol(area);

      ezInt32 j;
      for (j = 1; j < iMaxNumVertInPoly; ++j)
      {
        if (polyVtxIndices[j] == RC_MESH_NULL_IDX)
          break;

        const bool bIsContour = neighborData[j - 1] == 0xffff;

        {
          auto& line = bIsContour ? contourLines.ExpandAndGetRef() : innerLines.ExpandAndGetRef();
          line.m_start = GetNavMeshVertex(&mesh, polyVtxIndices[j - 1], vMeshOrigin, fCellSize, fCellHeight);
          line.m_end = GetNavMeshVertex(&mesh, polyVtxIndices[j], vMeshOrigin, fCellSize, fCellHeight);
        }
      }

      // close the loop
      const bool bIsContour = neighborData[j - 1] == 0xffff;
      {
        auto& line = bIsContour ? contourLines.ExpandAndGetRef() : innerLines.ExpandAndGetRef();
        line.m_start = GetNavMeshVertex(&mesh, polyVtxIndices[j - 1], vMeshOrigin, fCellSize, fCellHeight);
        line.m_end = GetNavMeshVertex(&mesh, polyVtxIndices[0], vMeshOrigin, fCellSize, fCellHeight);
      }

      for (j = 2; j < iMaxNumVertInPoly; ++j)
      {
        if (polyVtxIndices[j] == RC_MESH_NULL_IDX)
          break;

        auto& triangle = triangles.ExpandAndGetRef();

        triangle.m_p0 = GetNavMeshVertex(&mesh, polyVtxIndices[0], vMeshOrigin, fCellSize, fCellHeight);
        triangle.m_p2 = GetNavMeshVertex(&mesh, polyVtxIndices[j - 1], vMeshOrigin, fCellSize, fCellHeight);
        triangle.m_p1 = GetNavMeshVertex(&mesh, polyVtxIndices[j], vMeshOrigin, fCellSize, fCellHeight);
      }
    }

    ezDebugRenderer::DrawSolidTriangles(GetWorld(), triangles, ezColor::CadetBlue.WithAlpha(0.25f));
    ezDebugRenderer::DrawLines(GetWorld(), contourLines, ezColor::DarkOrange);
    ezDebugRenderer::DrawLines(GetWorld(), innerLines, ezColor::CadetBlue);
  }
}

void ezRcNavMeshComponent::VisualizePointsOfInterest()
{
  if (!m_bShowNavMesh && !g_AiShowNavMesh)
    return;

  const auto& poi = GetManager()->GetRecastWorldModule()->m_NavMeshPointsOfInterest;
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

ezRcNavMeshComponentManager::ezRcNavMeshComponentManager(ezWorld* pWorld) : SUPER(pWorld) { }
ezRcNavMeshComponentManager::~ezRcNavMeshComponentManager() { }

void ezRcNavMeshComponentManager::Initialize()
{
  SUPER::Initialize();

  // make sure this world module exists
  m_pWorldModule = GetWorld()->GetOrCreateModule<ezRecastWorldModule>();

  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezRcNavMeshComponentManager::Update, this);

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
