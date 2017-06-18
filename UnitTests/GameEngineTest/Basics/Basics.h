#pragma once

#include <PCH.h>
#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

class ezWorld;

class ezGameEngineTestApplication_Basics : public ezGameEngineTestApplication
{
public:
  ezGameEngineTestApplication_Basics();

protected:
  virtual void SetupWorld() override;

};

class ezGameEngineTestBasics : public ezGameEngineTest
{
public:

  virtual const char* GetTestName() const override;
  virtual ezGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    ST_SimpleScene,
  };

  virtual void SetupSubTests() override;


  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;

  ezTestAppRun SubtestSimpleScene();

  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier) override;

  ezInt32 m_iFrame;
};


