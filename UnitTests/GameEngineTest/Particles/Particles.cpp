#include <PCH.h>

#include "Particles.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

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
  AddSubTest("Billboards", SubTests::ST_Billboards);
}

ezResult ezGameEngineTestParticles::InitializeSubTest(ezInt32 iIdentifier)
{
  m_iFrame = -1;

  if (iIdentifier == SubTests::ST_Billboards)
  {
    m_pOwnApplication->SubTestBillboardsSetup();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestParticles::RunSubTest(ezInt32 iIdentifier)
{
  ++m_iFrame;

  if (iIdentifier == SubTests::ST_Billboards)
    return m_pOwnApplication->SubTestBillboardsExec(m_iFrame);

  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezTestAppRun::Quit;
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

ezTestAppRun ezGameEngineTestApplication_Particles::SubTestBillboardsExec(ezInt32 iCurFrame)
{
  if (Run() == ezApplication::Quit)
    return ezTestAppRun::Quit;

  switch (iCurFrame)
  {
    case 15:
    case 30:
      EZ_TEST_IMAGE(150);
      break;

    case 60:
      EZ_TEST_IMAGE(150);
      return ezTestAppRun::Quit;
  }

  return ezTestAppRun::Continue;
}
