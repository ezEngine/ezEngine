#pragma once
#include <Foundation/Types/UniquePtr.h>
#include <TestFramework/Framework/TestBaseClass.h>

class ezTestFramework;

class ezEditorTests
{
public:
  ezEditorTests();
  ~ezEditorTests();

  void ShowTests();


private:

  ezUniquePtr<ezTestFramework> m_TestFramework;
};


class ezEditorTest : public ezTestBaseClass
{
public:
  ezEditorTest();

  virtual ezResult GetImage(ezImage& img) override;

protected:
  virtual void SetupSubTests() override { }
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier) override { return ezTestAppRun::Quit; }

  virtual ezResult InitializeTest() override { return EZ_SUCCESS; }
  virtual ezResult DeInitializeTest() override { return EZ_SUCCESS; }
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;

protected:

};



class ezBasicEditorTests : public ezEditorTest
{
public:

  virtual const char* GetTestName() const override { return "Basics"; }

private:
  enum SubTests
  {
    MyFirstTest
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("My First Test", SubTests::MyFirstTest);
  }


  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;

  ezTestAppRun SubtestMyFirstTest();

  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier) override;
};



