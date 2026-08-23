#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

/// Verifies that SerializeComponent() and DeserializeComponent() of every known component type
/// agree about the amount of data they write and read.
///
/// Runs inside a game application because creating a component of an arbitrary type also creates its
/// component manager, and some managers allocate GPU resources, which requires a graphics device.
class ezGameEngineTestComponentSerialization : public ezGameEngineTest
{
  using SUPER = ezGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual ezGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    SerializeDeserializeRoundtrip,
  };

  virtual void SetupSubTests() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  ezGameEngineTestApplication* m_pOwnApplication = nullptr;
};
