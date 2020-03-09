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
  AddSubTest("Billboards", SubTests::Billboards);
  AddSubTest("BillboardRenderer", SubTests::BillboardRenderer);
  AddSubTest("ColorGradient", SubTests::ColorGradient);
  AddSubTest("Flies", SubTests::Flies);
  AddSubTest("Gravity", SubTests::Gravity);
  AddSubTest("LightRenderer", SubTests::LightRenderer);
  AddSubTest("MeshRenderer", SubTests::MeshRenderer);
  AddSubTest("Raycast", SubTests::Raycast);
  AddSubTest("SizeCurve", SubTests::SizeCurve);
  AddSubTest("TrailRenderer", SubTests::TrailRenderer);
  AddSubTest("Velocity", SubTests::Velocity);
}

ezResult ezGameEngineTestParticles::InitializeSubTest(ezInt32 iIdentifier)
{
  m_iFrame = -1;

  if (iIdentifier == SubTests::Billboards)
  {
    m_pOwnApplication->SubTestBillboardsSetup();
    return EZ_SUCCESS;
  }
  else
  {
    const char* szEffects[] = {
      "",
      "{ 08b3e790-2832-4083-93ec-133a93054c4c }",
      "{ f0959d22-6004-47e7-b167-af707d4d5cea }",
      "{ cb01c6d9-b8ff-4347-ab8b-e94403c68aad }",
      "{ ec64cef4-4936-4e44-8d47-9fedda2cc75b }",
      "{ 856007bd-6f03-4bc0-bb61-f7fcc5a1575b }",
      "{ 54f8f8f5-15e4-46c3-a80e-d5a6e2d6b693 }",
      "{ 4ff34107-3159-4357-b4eb-9652b0888a16 }",
      "{ 58bf4d72-aa09-404f-81b8-13965d3e2286 }",
      "{ ec85e634-b8ee-475f-bd86-7cbc2973de0a }",
      "{ ba82b712-3af7-430d-91a6-492aa836dffb }",
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

void ezGameEngineTestApplication_Particles::SubTestBillboardsSetup()
{
  LoadScene("Particles/AssetCache/Common/Billboards.ezObjectGraph");
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
