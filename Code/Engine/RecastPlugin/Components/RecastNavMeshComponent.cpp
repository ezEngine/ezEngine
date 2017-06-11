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

}

void ezRcNavMeshComponent::DeserializeComponent(ezWorldReader& stream)
{

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

  --m_uiDelay;

  if (m_uiDelay > 0)
    return;

  m_NavMeshBuilder.Build(m_NavMeshConfig, *GetWorld());

  GetManager()->GetRecastWorldModule()->SetNavMesh(m_NavMeshBuilder.m_pNavMesh);
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

    ezDynamicArray<ezDebugRenderer::Line> lines;
    lines.Reserve(mesh.npolys * 5);

    const ezInt32 iMaxNumVertInPoly = m_NavMeshBuilder.m_polyMesh->nvp;
    const float fCellSize = m_NavMeshBuilder.m_polyMesh->cs;
    const float fCellHeight = m_NavMeshBuilder.m_polyMesh->ch;
    // add a little height offset to move the visualization up a little
    const ezVec3 vMeshOrigin(m_NavMeshBuilder.m_polyMesh->bmin[0], m_NavMeshBuilder.m_polyMesh->bmin[2], m_NavMeshBuilder.m_polyMesh->bmin[1] + fCellHeight * 0.3f);

    for (ezInt32 i = 0; i < mesh.npolys; ++i)
    {
      const ezUInt16* polyVtxIndices = &mesh.polys[i * (iMaxNumVertInPoly * 2)];

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

        auto& line = lines.ExpandAndGetRef();
        line.m_start = GetNavMeshVertex(&mesh, polyVtxIndices[j - 1], vMeshOrigin, fCellSize, fCellHeight);
        line.m_end = GetNavMeshVertex(&mesh, polyVtxIndices[j], vMeshOrigin, fCellSize, fCellHeight);
      }

      // close the loop
      auto& line = lines.ExpandAndGetRef();
      line.m_start = GetNavMeshVertex(&mesh, polyVtxIndices[j - 1], vMeshOrigin, fCellSize, fCellHeight);
      line.m_end = GetNavMeshVertex(&mesh, polyVtxIndices[0], vMeshOrigin, fCellSize, fCellHeight);

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
    ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::CadetBlue);
  }
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
