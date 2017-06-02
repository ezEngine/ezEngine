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

void ezRcNavMeshComponent::Update()
{
  if (!IsActiveAndSimulating())
    return;

  if (m_uiDelay == 0)
  {
    if ((m_bShowNavMesh || g_AiShowNavMesh) && (m_NavMeshBuilder.m_polyMesh != nullptr))
    {
      const auto& mesh = *m_NavMeshBuilder.m_polyMesh;

      ezDynamicArray<ezDebugRenderer::Triangle> triangles;
      triangles.Reserve(mesh.npolys * 2);

      const int nvp = m_NavMeshBuilder.m_polyMesh->nvp;
      const float cs = m_NavMeshBuilder.m_polyMesh->cs;
      const float ch = m_NavMeshBuilder.m_polyMesh->ch;
      const float* orig = m_NavMeshBuilder.m_polyMesh->bmin;

      for (ezInt32 i = 0; i < mesh.npolys; ++i)
      {
        const unsigned short* poly = &mesh.polys[i*nvp * 2];
        const unsigned char area = mesh.areas[i];

        //unsigned int color;
        //if (area == RC_WALKABLE_AREA)
        //  color = duRGBA(0, 192, 255, 64);
        //else if (area == RC_NULL_AREA)
        //  color = duRGBA(0, 0, 0, 64);
        //else
        //  color = dd->areaToCol(area);

        unsigned short vi[3];
        for (int j = 2; j < nvp; ++j)
        {
          if (poly[j] == RC_MESH_NULL_IDX)
            break;

          vi[0] = poly[0];
          vi[1] = poly[j - 1];
          vi[2] = poly[j];

          auto& triangle = triangles.ExpandAndGetRef();

          for (int k = 0; k < 3; ++k)
          {
            const unsigned short* v = &mesh.verts[vi[k] * 3];
            const float x = orig[0] + v[0] * cs;
            const float z = orig[1] + (v[1] + 1)*ch;
            const float y = orig[2] + v[2] * cs;

            if (k == 0)
              triangle.m_p0 = ezVec3(x, y, z);
            else if (k == 1)
              triangle.m_p2 = ezVec3(x, y, z);
            else if (k == 2)
              triangle.m_p1 = ezVec3(x, y, z);
          }
        }
      }

      ezDebugRenderer::DrawSolidTriangles(GetWorld(), triangles, ezColor::CadetBlue);
    }

    return;
  }

  --m_uiDelay;

  if (m_uiDelay > 0)
    return;

  m_NavMeshBuilder.Build(m_NavMeshConfig, *GetWorld());

  GetManager()->GetRecastWorldModule()->SetNavMesh(m_NavMeshBuilder.m_pNavMesh);
}

void ezRcNavMeshComponent::OnSimulationStarted()
{
  m_uiDelay = 2;
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
