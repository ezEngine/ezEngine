#include <Core/ResourceManager/ResourceManager.h>
#include <CoreUtils/Geometry/GeomUtils.h>
#include <CoreUtils/Graphics/Camera.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include "Level.h"
#include "Application.h"
#include "ShipComponent.h"
#include "ProjectileComponent.h"
#include "AsteroidComponent.h"
#include "CollidableComponent.h"

static ezMeshResourceHandle CreateAsteroidMesh()
{
  ezMeshResourceHandle hMesh = ezResourceManager::GetResourceHandle<ezMeshResource>("AsteroidMesh");

  {
    ezResourceLock<ezMeshResource> mesh(hMesh, ezResourceAcquireMode::PointerOnly);
    if (mesh->GetLoadingState() == ezResourceLoadState::Loaded)
      return hMesh;
  }

  ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetResourceHandle<ezMeshBufferResource>("AsteroidMeshBuffer");

  {
    ezGeometry geom;
    geom.AddGeodesicSphere(1.0f, 1, ezColor::GetWhite());
    geom.ComputeFaceNormals();
    geom.ComputeSmoothVertexNormals();

    ezMeshBufferResourceDescriptor desc;
    desc.m_pDevice = SampleGameApp::GetDevice();
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
    desc.AddStream(ezGALVertexAttributeSemantic::Color, ezGALResourceFormat::RGBAByteNormalized);

    const ezDeque<ezGeometry::Vertex>& vertices = geom.GetVertices();
    const ezDeque<ezGeometry::Polygon>& polygons = geom.GetPolygons();

    desc.AllocateStreams(vertices.GetCount(), polygons.GetCount());

    for (ezUInt32 v = 0; v < vertices.GetCount(); ++v)
    {
      desc.SetVertexData<ezVec3>(0, v, vertices[v].m_vPosition);
      desc.SetVertexData<ezVec3>(1, v, vertices[v].m_vNormal);
      desc.SetVertexData<ezColor8UNorm>(2, v, vertices[v].m_Color);
    }

    for (ezUInt32 p = 0; p < polygons.GetCount(); ++p)
    {
      desc.SetTriangleIndices(p, polygons[p].m_Vertices[0], polygons[p].m_Vertices[1], polygons[p].m_Vertices[2]);
    }

    ezResourceManager::CreateResource(hMeshBuffer, desc);
  }

  {
    ezMeshResourceDescriptor desc;
    desc.hMeshBuffer = hMeshBuffer;

    ezResourceManager::CreateResource(hMesh, desc);
  }

  return hMesh;
}

Level::Level()
{
  m_pWorld = nullptr;
}

Level::~Level()
{
  m_pWorld->TransferThreadOwnership();
  EZ_DEFAULT_DELETE(m_pWorld);
}

void Level::SetupLevel(const char* szLevelName)
{
  m_pWorld = EZ_DEFAULT_NEW(ezWorld)(szLevelName);

  m_pWorld->CreateComponentManager<ezMeshComponentManager>();
  m_pWorld->CreateComponentManager<ShipComponentManager>();
  m_pWorld->CreateComponentManager<ProjectileComponentManager>();
  m_pWorld->CreateComponentManager<AsteroidComponentManager>();
  m_pWorld->CreateComponentManager<CollidableComponentManager>();

  for (ezInt32 iPlayer = 0; iPlayer < MaxPlayers; ++iPlayer)
    CreatePlayerShip(iPlayer);

  for (ezInt32 iAsteroid = 0; iAsteroid < MaxAsteroids; ++iAsteroid)
    CreateAsteroid();

  m_Camera.LookAt(ezVec3(0.0f, 0.0f, 100.0f), ezVec3(0.0f));
  m_Camera.SetCameraMode(ezCamera::OrthoFixedWidth, 45.0f, 0.0f, 500.0f);
}

void Level::CreatePlayerShip(ezInt32 iPlayer)
{
  // create one game object for the ship
  // then attach a ship component to that object

  ezGameObjectDesc desc;
  desc.m_LocalPosition.x = -15 + iPlayer * 5.0f;

  ezGameObject* pGameObject = nullptr;
  m_hPlayerShips[iPlayer] = m_pWorld->CreateObject(desc, pGameObject);

  {
    ShipComponent* pShipComponent = nullptr;
    ezComponentHandle hShipComponent = ShipComponent::CreateComponent(m_pWorld, pShipComponent);
  
    pShipComponent->m_iPlayerIndex = iPlayer;

    pGameObject->AddComponent(hShipComponent);
  }
  {
    CollidableComponent* pCollidableComponent = nullptr;
    ezComponentHandle hCollidableomponent = CollidableComponent::CreateComponent(m_pWorld, pCollidableComponent);

    pCollidableComponent->m_fCollisionRadius = 1.0f;

    pGameObject->AddComponent(hCollidableomponent);
  }
}

void Level::CreateAsteroid()
{
  ezGameObjectDesc desc;
  desc.m_LocalPosition.x = (((rand() % 1000) / 999.0f) * 40.0f) - 20.0f;
  desc.m_LocalPosition.y = (((rand() % 1000) / 999.0f) * 40.0f) - 20.0f;

  desc.m_LocalScaling = ezVec3(1.0f + ((rand() % 1000) / 999.0f));

  ezGameObject* pGameObject = nullptr;
  ezGameObjectHandle hAsteroid = m_pWorld->CreateObject(desc, pGameObject);

  {
    ezMeshComponent* pMeshComponent = nullptr;
    ezMeshComponent::CreateComponent(m_pWorld, pMeshComponent);

    pMeshComponent->SetMesh(CreateAsteroidMesh());

    pGameObject->AddComponent(pMeshComponent);
  }
  {
    AsteroidComponent* pAsteroidComponent = nullptr;
    AsteroidComponent::CreateComponent(m_pWorld, pAsteroidComponent);
     
    pGameObject->AddComponent(pAsteroidComponent);
  }
  {
    CollidableComponent* pCollidableComponent = nullptr;
    CollidableComponent::CreateComponent(m_pWorld, pCollidableComponent);
      
    pCollidableComponent->m_fCollisionRadius = desc.m_LocalScaling.x;
    
    pGameObject->AddComponent(pCollidableComponent);
  }
}

void Level::Update()
{
  m_pWorld->TransferThreadOwnership();

  m_pWorld->Update();

  for (ezInt32 iPlayer = 0; iPlayer < MaxPlayers; ++iPlayer)
    UpdatePlayerInput(iPlayer);
}



void SampleGameApp::CreateGameLevel()
{
  m_pLevel = EZ_DEFAULT_NEW(Level);

  m_pLevel->SetupLevel("Asteroids - World");

  m_View.SetWorld(m_pLevel->GetWorld());
  m_View.SetLogicCamera(m_pLevel->GetCamera());
}

void SampleGameApp::DestroyGameLevel()
{
  EZ_DEFAULT_DELETE(m_pLevel);
}

