#include <Core/ResourceManager/ResourceManager.h>
#include <CoreUtils/Geometry/GeomUtils.h>
#include <CoreUtils/Graphics/Camera.h>
#include <Foundation/Logging/Log.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include "Level.h"
#include "Application.h"
#include "ShipComponent.h"
#include "ProjectileComponent.h"
#include "AsteroidComponent.h"
#include "CollidableComponent.h"
#include <Foundation/IO/FileSystem/FileSystem.h>

static ezMeshResourceHandle CreateAsteroidMesh()
{
  if (!ezFileSystem::ExistsFile("Meshes/Asteroid.ezm"))
  {
    ezGeometry geom;
    geom.AddGeodesicSphere(1.0f, 1, ezColor::White);
    geom.ComputeFaceNormals();
    geom.ComputeSmoothVertexNormals();

    ezMeshResourceDescriptor mfb;

    if (mfb.Load("Meshes/Asteroid.ezm").Failed())
    {
      mfb.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
      mfb.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
      mfb.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Color, ezGALResourceFormat::RGBAByteNormalized);

      const ezDeque<ezGeometry::Vertex>& vertices = geom.GetVertices();
      const ezDeque<ezGeometry::Polygon>& polygons = geom.GetPolygons();

      mfb.MeshBufferDesc().AllocateStreams(vertices.GetCount(), polygons.GetCount());

      for (ezUInt32 v = 0; v < vertices.GetCount(); ++v)
      {
        mfb.MeshBufferDesc().SetVertexData<ezVec3>(0, v, vertices[v].m_vPosition);
        mfb.MeshBufferDesc().SetVertexData<ezVec3>(1, v, vertices[v].m_vNormal);
        mfb.MeshBufferDesc().SetVertexData<ezColorLinearUB>(2, v, vertices[v].m_Color);
      }

      for (ezUInt32 p = 0; p < polygons.GetCount(); ++p)
      {
        mfb.MeshBufferDesc().SetTriangleIndices(p, polygons[p].m_Vertices[0], polygons[p].m_Vertices[1], polygons[p].m_Vertices[2]);
      }

      mfb.AddSubMesh(polygons.GetCount(), 0, 0);
      mfb.SetMaterial(0, "Materials/Asteroid.ezMaterial");
      mfb.Save("Meshes/Asteroid.ezm");
    }
  }

  return ezResourceManager::LoadResource<ezMeshResource>("Meshes/Asteroid.ezm");

  //ezMeshResourceHandle hMesh = ezResourceManager::CreateResource<ezMeshResource>("AsteroidMesh", mfb);
  //return hMesh;


}

static ezMaterialResourceHandle CreateAsteroidMaterial()
{
  ezMaterialResourceDescriptor desc;
  desc.m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Generic.ezShader");

  ezMaterialResourceHandle hMaterial = ezResourceManager::CreateResource<ezMaterialResource>("AsteroidMaterial", desc);

  return hMaterial;
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

    if (!m_hAsteroidMesh.IsValid())
      m_hAsteroidMesh = CreateAsteroidMesh();

    if (!m_hAsteroidMaterial.IsValid())
      m_hAsteroidMaterial = CreateAsteroidMaterial();

    pMeshComponent->SetMesh(m_hAsteroidMesh);
    pMeshComponent->SetMaterial(0, m_hAsteroidMaterial);

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

