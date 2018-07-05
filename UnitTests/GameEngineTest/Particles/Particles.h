#pragma once

#include <PCH.h>

#include "../TestClass/TestClass.h"

class ezGameEngineTestApplication_Particles : public ezGameEngineTestApplication
{
public:
  ezGameEngineTestApplication_Particles();

  void SubTestBillboardsSetup();
  ezTestAppRun SubTestBillboardsExec(ezInt32 iCurFrame);
};

class ezGameEngineTestParticles : public ezGameEngineTest
{
public:
  virtual const char* GetTestName() const override;
  virtual ezGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    ST_Billboards,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier) override;

  ezInt32 m_iFrame;
  ezGameEngineTestApplication_Particles* m_pOwnApplication;
};


