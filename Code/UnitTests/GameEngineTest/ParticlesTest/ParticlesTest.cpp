#include <GameEngineTestPCH.h>

#include "ParticlesTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ParticlePlugin/Components/ParticleComponent.h>

static ezGameEngineTestParticles s_GameEngineTestParticles;

const char* ezGameEngineTestParticles::GetTestName() const
{
  return "Particle Tests";
}

ezGameEngineTestApplication* ezGameEngineTestParticles::CreateApplication()
{
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication_Particles);
  return m_pOwnApplication;
}

void ezGameEngineTestParticles::SetupSubTests()
{
  AddSubTest("BillboardRenderer", SubTests::BillboardRenderer);
  AddSubTest("ColorGradientBehavior", SubTests::ColorGradientBehavior);
  AddSubTest("FliesBehavior", SubTests::FliesBehavior);
  AddSubTest("GravityBehavior", SubTests::GravityBehavior);
  AddSubTest("LightRenderer", SubTests::LightRenderer);
  AddSubTest("MeshRenderer", SubTests::MeshRenderer);
  AddSubTest("RaycastBehavior", SubTests::RaycastBehavior);
  AddSubTest("SizeCurveBehavior", SubTests::SizeCurveBehavior);
  AddSubTest("TrailRenderer", SubTests::TrailRenderer);
  AddSubTest("VelocityBehavior", SubTests::VelocityBehavior);
  //AddSubTest("EffectRenderer", SubTests::EffectRenderer);
  AddSubTest("BoxPosInitializer", SubTests::BoxPositionInitializer);
  AddSubTest("SpherePosInitializer", SubTests::SpherePositionInitializer);
  AddSubTest("CylinderPosInitializer", SubTests::CylinderPositionInitializer);
  AddSubTest("RandomColorInitializer", SubTests::RandomColorInitializer);
  AddSubTest("RandomSizeInitializer", SubTests::RandomSizeInitializer);
  AddSubTest("RotationSpeedInitializer", SubTests::RotationSpeedInitializer);
  AddSubTest("VelocityConeInitializer", SubTests::VelocityConeInitializer);

  AddSubTest("Billboards", SubTests::Billboards);
  //AddSubTest("PullAlongBehavior", SubTests::PullAlongBehavior);
}

ezResult ezGameEngineTestParticles::InitializeSubTest(ezInt32 iIdentifier)
{
  m_iFrame = -1;

  if (iIdentifier == SubTests::Billboards)
  {
    m_pOwnApplication->SetupSceneSubTest("Particles/AssetCache/Common/Billboards.ezObjectGraph");
    return EZ_SUCCESS;
  }
  else if (iIdentifier == SubTests::PullAlongBehavior)
  {
    m_pOwnApplication->SetupSceneSubTest("Particles/AssetCache/Common/Particles2.ezObjectGraph");
    return EZ_SUCCESS;
  }
  else
  {
    const char* szEffects[] = {
      "{ 08b3e790-2832-4083-93ec-133a93054c4c }", // BillboardRenderer
      "{ f0959d22-6004-47e7-b167-af707d4d5cea }", // ColorGradientBehavior
      "{ cb01c6d9-b8ff-4347-ab8b-e94403c68aad }", // FliesBehavior
      "{ ec64cef4-4936-4e44-8d47-9fedda2cc75b }", // GravityBehavior
      "{ 856007bd-6f03-4bc0-bb61-f7fcc5a1575b }", // LightRenderer
      "{ 54f8f8f5-15e4-46c3-a80e-d5a6e2d6b693 }", // MeshRenderer
      "{ 4ff34107-3159-4357-b4eb-9652b0888a16 }", // RaycastBehavior
      "{ 58bf4d72-aa09-404f-81b8-13965d3e2286 }", // SizeCurveBehavior
      "{ ec85e634-b8ee-475f-bd86-7cbc2973de0a }", // TrailRenderer
      "{ ba82b712-3af7-430d-91a6-492aa836dffb }", // VelocityBehavior
      //"{ 3673cc69-2ac0-463a-b9b3-207cc30b7f25 }", // EffectRenderer
      "{ e30bbbf2-9bda-45e0-8116-1ae8b998ce61 }", // BoxPositionInitializer
      "{ 536e7516-d811-4552-a3e9-5153dfdd5be1 }", // SpherePositionInitializer
      "{ 2fb51ce6-69fc-44ad-b453-2822e091916f }", // CylinderPositionInitializer
      "{ 8cfee0af-ac0e-452d-b13f-e67420497397 }", // RandomColorInitializer
      "{ b4a3fc51-60ac-48b2-abec-5b2b6728676c }", // RandomSizeInitializer
      "{ f51d9d7b-0ad9-4f61-acb4-745c2b91a311 }", // RotationSpeedInitializer
      "{ c5a48c20-efab-4af5-a86b-91cd2241682e }", // VelocityConeInitializer
    };

    m_pOwnApplication->SetupParticleSubTest(szEffects[iIdentifier]);
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestParticles::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  ++m_iFrame;

  return m_pOwnApplication->ExecParticleSubTest(m_iFrame);
}

//////////////////////////////////////////////////////////////////////////

ezGameEngineTestApplication_Particles::ezGameEngineTestApplication_Particles()
  : ezGameEngineTestApplication("Particles")
{
}

//////////////////////////////////////////////////////////////////////////

void ezGameEngineTestApplication_Particles::SetupSceneSubTest(const char* szFile)
{
  LoadScene(szFile);
}

void ezGameEngineTestApplication_Particles::SetupParticleSubTest(const char* szFile)
{
  LoadScene("Particles/AssetCache/Common/Particles1.ezObjectGraph");

  EZ_LOCK(m_pWorld->GetWriteMarker());

  ezGameObject* pObject;
  m_pWorld->TryGetObjectWithGlobalKey("Effect", pObject);

  ezParticleComponent* pEffect;
  m_pWorld->GetOrCreateComponentManager<ezParticleComponentManager>()->CreateComponent(pObject, pEffect);
  pEffect->SetParticleEffectFile(szFile);
  pEffect->m_uiRandomSeed = 42;
}

ezTestAppRun ezGameEngineTestApplication_Particles::ExecParticleSubTest(ezInt32 iCurFrame)
{
  if (Run() == ezApplication::Quit)
    return ezTestAppRun::Quit;

  switch (iCurFrame)
  {
    case 15:
      EZ_TEST_IMAGE(0, 50);
      break;

    case 30:
      EZ_TEST_IMAGE(1, 50);
      break;

    case 60:
      EZ_TEST_IMAGE(2, 50);
      return ezTestAppRun::Quit;
  }

  return ezTestAppRun::Continue;
}
