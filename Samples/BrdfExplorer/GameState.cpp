#include <Core/ResourceManager/ResourceManager.h>
#include <Core/Graphics/Camera.h>

#include <RendererCore/Meshes/MeshComponent.h>

#include <GameEngine/Components/RotorComponent.h>
#include <GameEngine/GameApplication/GameApplication.h>

#include "GameState.h"
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(BrdfExplorerGameState, 1, ezRTTIDefaultAllocator<BrdfExplorerGameState>);
EZ_END_DYNAMIC_REFLECTED_TYPE

BrdfExplorerGameState::BrdfExplorerGameState()
{
}

BrdfExplorerGameState::~BrdfExplorerGameState()
{

}

float SpecularGGX(float roughness, float NdotH)
{
  // mad friendly reformulation of:
  //
  //              m^2
  // --------------------------------
  // PI * ((N.H)^2 * (m^2 - 1) + 1)^2

  float m = roughness * roughness;
  float m2 = m * m;
  float f = (NdotH * m2 - NdotH) * NdotH + 1.0f;
  return m2 / (ezMath::BasicType<float>::Pi() * f * f);
}

ezVec3 UniformSampleHemisphere(ezVec2 uv)
{
  float z = uv.x;
  float r = ezMath::Sqrt(ezMath::Max(0.0f, 1.0f - z*z));
  ezAngle phi = ezAngle::Radian(2.0f * ezMath::BasicType<float>::Pi() * uv.y);
  float x = r * ezMath::Cos(phi);
  float y = r * ezMath::Sin(phi);
  return ezVec3(x, y, z);
}

ezVec2 randomVec2(ezRandom& random)
{
  return ezVec2((float)random.DoubleZeroToOneInclusive(), (float)random.DoubleZeroToOneInclusive());
}

void BrdfExplorerGameState::OnActivation(ezWorld* pWorld)
{
  EZ_LOG_BLOCK("BrdfExplorerGameState::Activate");

  ezRandom random;
  random.Initialize(1);

  ezUInt32 numSamples = 0;
  double accu = 0.0f;
  //const float roughness = 0.2f;
  //ezVec3 Wo = ezVec3(0.3, 0.5, 0.7).GetNormalized();
  while (true)
  {
    ezVec2 randomVec = randomVec2(random);
    ezVec3 Wi = UniformSampleHemisphere(randomVec).GetNormalized();
    if (Wi.z < 0)
    {
      Wi.z *= -1;
    }
    float NdotL = Wi.z;
    accu += NdotL;
    numSamples++;

    if (numSamples % 10000 == 0 && numSamples > 0)
    {
      ezLog::Info("Value = {0}", (2 * accu) / (double)numSamples);
    }
  }

  ezGameState::OnActivation(pWorld);

  CreateGameLevel();
}

void BrdfExplorerGameState::OnDeactivation()
{
  EZ_LOG_BLOCK("BrdfExplorerGameState::Deactivate");

  DestroyGameLevel();

  ezGameState::OnDeactivation();
}

float BrdfExplorerGameState::CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const
{
  return 1.0f;
}

void BrdfExplorerGameState::CreateGameLevel()
{
  ezWorldDesc desc("Level");
  m_pMainWorld = GetApplication()->CreateWorld(desc);
  EZ_LOCK( m_pMainWorld->GetWriteMarker());

  ezMeshComponentManager* pMeshCompMan = m_pMainWorld->GetOrCreateComponentManager<ezMeshComponentManager>();

  ezGameObjectDesc obj;
  ezGameObject* pObj;
  ezMeshComponent* pMesh;

  ezMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezMeshResource>("Sponza/Meshes/Sponza.ezMesh");

  // World Mesh
  {
    obj.m_sName.Assign("Sponza");
    m_pMainWorld->CreateObject(obj, pObj);

    pMeshCompMan->CreateComponent(pObj, pMesh);
    pMesh->SetMesh(hMesh);
  }

  // Lights
  {
    obj.m_sName.Assign("DirLight");
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0.0f, 1.0f, 0.0f), ezAngle::Degree(60.0f));
    obj.m_LocalPosition.SetZero();
    obj.m_LocalRotation.SetIdentity();

    m_pMainWorld->CreateObject(obj, pObj);

    ezDirectionalLightComponent* pDirLight;
    ezDirectionalLightComponent::CreateComponent(pObj, pDirLight);

    ezAmbientLightComponent* pAmbLight;
    ezAmbientLightComponent::CreateComponent(pObj, pAmbLight);
  }

  ChangeMainWorld(m_pMainWorld);
}

void BrdfExplorerGameState::DestroyGameLevel()
{
  GetApplication()->DestroyWorld(m_pMainWorld);
}

EZ_APPLICATION_ENTRY_POINT(ezGameApplication, "BRDF Explorer", ezGameApplicationType::StandAlone, "Data/Samples/BrdfExplorer");


